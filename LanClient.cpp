#include "stdafx.h"

#define setID(index, clientID) (index << 48) | clientID
#define getID(clientID) (clientID<<16) >>16
#define getIndex(clientID) clientID>>48


bool LanClient::Start(char *_ip, unsigned short _port, unsigned short _workerCount, bool _nagleOpt)
{
	user = new connectedClient;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		onError(0, L"���� �ʱ�ȭ ����");
		return false;
	}

	user->Sock = socket(AF_INET, SOCK_STREAM, 0);
	if (user->Sock == INVALID_SOCKET)
	{
		onError(0, L"SOCKET ����");
		return false;
	}

	hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (!hcp)
	{
		onError(0, L"IOCP HANDLE CREATE ERROR");
		return false;
	}

	// nagle check
	if (!nagleOpt)
	{
		bool optval = TRUE;
		setsockopt(user->Sock, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));
	}


	// keep alive option
	tcp_keepalive tcpKeep;
	tcpKeep.onoff = 1;
	tcpKeep.keepalivetime = 10000;
	tcpKeep.keepaliveinterval = 10;
	WSAIoctl(user->Sock, SIO_KEEPALIVE_VALS, (tcp_keepalive*)&tcpKeep, sizeof(tcp_keepalive), NULL, 0, NULL, NULL, NULL);

	// init
	strcpy_s(ip, 16, _ip);
	inet_pton(AF_INET,ip,&addr);
	port = _port;
	workerCount = _workerCount;
	nagleOpt = _nagleOpt;
	
	connectTotal = 0;
	sendTPS = 0;
	recvTPS = 0;
	connectFlag = false;


	// ������ ����
	threadArr = new HANDLE[workerCount];

	HANDLE threadHandle = (HANDLE)_beginthreadex(NULL, 0, connectThread, (LPVOID)this, 0, 0);
	CloseHandle(threadHandle);
	threadHandle = (HANDLE)_beginthreadex(NULL, 0, tpsThread, (LPVOID)this, 0, 0);
	CloseHandle(threadHandle);

	int i = 0;
	for (i; i < workerCount; i++)
		threadArr[i] = (HANDLE)_beginthreadex(NULL, 0, workerThread, (LPVOID)this, 0, 0);

	return true;
}

bool LanClient::Stop(void)
{
	// print  ������ ����
	quit = true;


	// disconnect;
	printf("Lan session disconnet waiting...\n");
	disconnect();
	printf("Lan session disconnet success\n");

	// worker ������ ����
	int j = 0;
	for (j; j < workerCount; j++)
		PostQueuedCompletionStatus(hcp, 0, 0, 0);


	// ��� : ��� �����尡 ���� �� �� ����
	WaitForMultipleObjects(workerCount, threadArr, TRUE, INFINITE);
	printf("Lan worker thread is closed\n");

	int z = 0;
	for (z; z < workerCount; z++)
		CloseHandle(threadArr[z]);

	delete[] threadArr;
	delete user;
	WSACleanup();

	return true;
}

void LanClient::SendPacket(Sbuf *_buf)
{
	_buf->lanEncode();
	_buf->addRef();
	user->sendQ.push(_buf);

	if (user->sendFlag == false)
		sendPost(user);
}

unsigned __stdcall LanClient::connectThread(LPVOID _data)
{
	LanClient *client = (LanClient*)_data;
	SOCKADDR_IN serverAddr = client->server;
	int retval = 0;
	int errorCode = 0;
	connectedClient *User = client->user;
	SOCKET sock = User->Sock;
	while (1)
	{
		if (!client->connectFlag)
		{
			ZeroMemory(&serverAddr, sizeof(serverAddr));
			serverAddr.sin_family = AF_INET;
			serverAddr.sin_addr = client->addr;
			serverAddr.sin_port = htons(client->port);
			retval = connect(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
			if (retval == SOCKET_ERROR)
			{
				errorCode = WSAGetLastError();
				_SYSLOG(Type::Type_CONSOLE, Level::SYS_ERROR, L"CONNECT ERROR CODE : %d", errorCode);
				continue;
			}
			client->connectFlag = true;
			User->Sock = sock;
			CreateIoCompletionPort((HANDLE)sock, client->hcp, (ULONG_PTR)client->user, 0);
			InterlockedIncrement(&(User->iocpCount));
			client->onClientJoin();
			client->recvPost(User);

			if (0 == InterlockedDecrement(&(User->iocpCount)))
				client->disconnect();
		}
		Sleep(999);
	}
	
	return 0;
}

unsigned __stdcall LanClient::workerThread(LPVOID _data)
{
	LanClient *client = (LanClient*)_data;

	int retval = 1;
	DWORD trans = 0;
	OVERLAPPED *over = NULL;
	connectedClient *User = NULL;

	while (1)
	{
		trans = 0;
		over = NULL, User = NULL;
		retval = GetQueuedCompletionStatus(client->hcp, &trans, (PULONG_PTR)&User, (LPOVERLAPPED*)&over, INFINITE);
		if (!over)
		{
			if (retval == false)
			{
				client->onError(WSAGetLastError(), L"GQCS error : overlapped is NULL and return false");
				break;
			}
			if (trans == 0 && !User)		// ���� ��ȣ
				break;
		}
		else
		{
			if (&(User->recvOver) == over)
				client->completeRecv(trans, User);

			if (&(User->sendOver) == over)
				client->completeSend(trans, User);

			// ������� �ϳ� �߰����༭ �� �޾Ƴ���. ���� ���� �� ���ϰ� Ȯ�ο뵵. iocount<0 �̸� crash
			if (0 == InterlockedDecrement(&(User->iocpCount)))
				client->disconnect();
		}

	}
	return 0;
}

unsigned __stdcall LanClient::tpsThread(LPVOID _data)
{
	LanClient *client = (LanClient*)_data;
	while (1)
	{
		if (client->quit) break;
		client->onTPS();
		client->psendTPS = client->getSendTPS();
		client->precvTPS = client->getRecvTPS();
		client->setTPS();
		Sleep(999);
	}
	return 0;
}

int LanClient::getSendTPS(void)
{	
	return sendTPS;
}

int LanClient::getRecvTPS(void)
{
	return recvTPS;
}

void LanClient::setTPS(void)
{
	InterlockedExchange(&sendTPS, 0);
	InterlockedExchange(&recvTPS, 0);
}

void LanClient::recvPost(connectedClient *_User)
{

	DWORD recvVal, flag;
	int retval, err;
	winBuffer *_recv = &_User->recvQ;
	ZeroMemory(&(_User->recvOver), sizeof(_User->recvOver));
	WSABUF buf[2];
	ZeroMemory(&buf, sizeof(WSABUF) * 2);
	buf[0].buf = _recv->getRearPosPtr();
	buf[0].len = _recv->getNotBrokenFreeSize();


	if (_recv->getFreeSize() > _recv->getNotBrokenFreeSize())
	{
		buf[1].buf = _recv->getBufferPtr();
		buf[1].len = (_recv->getFreeSize() - _recv->getNotBrokenFreeSize());
	}
	// RECV ������ ���� ��� ���� ó�� ���ּ���. ��Ŷ��ü�� ������ �ִ°�. ����� ���̰� �߸� ��
	// Ŭ�� ���� ��Ŷ�� ���ų� ������ ������ �ϴ� �� �̹Ƿ� ������ ��������. �ߴ��� �����Դϴ�. 
	recvVal = 0, flag = 0;
	InterlockedIncrement(&(_User->iocpCount));
	retval = WSARecv(_User->Sock, buf, 2, &recvVal, &flag, &(_User->recvOver), NULL);

	if (retval == SOCKET_ERROR)
	{
		// PENDING �� ��� ���߿� ó���ȴٴ� �ǹ�
		err = WSAGetLastError();
		if (err != WSA_IO_PENDING)
		{
			if (err != WSAECONNRESET && err != WSAESHUTDOWN && err != WSAECONNABORTED)
			{
				onError(err, L"RECV ERROR");
			}
			clientShutdown();
			if (0 == InterlockedDecrement(&(_User->iocpCount)))
				disconnect();
		}
	}
}

void LanClient::sendPost(connectedClient *_User)
{
	DWORD sendVal = 0;

	int count = 0;
	int result = 0;
	int size = 0;
	WSABUF buf[maxWSABUF];
	buf[count].buf = 0;
	buf[count].len = 0;
	boost::lockfree::queue<Sbuf*> *Send = &_User->sendQ;
	boost::lockfree::queue<Sbuf*> *completeSend = &_User->completeSendQ;
	if (Send->empty()) return;
	if (FALSE == (BOOL)InterlockedCompareExchange(&(_User->sendFlag), TRUE, FALSE))
	{
		_User->sendCount = 0;
		count = 0;
		int retval = 0;
		int count = 0;
		Sbuf *storedBuf;
		ZeroMemory(&_User->sendOver, sizeof(_User->sendOver));
		do
		{
			for (count; count < maxWSABUF; )
			{
				storedBuf = NULL;
				retval = Send->pop(storedBuf);
				if (retval == -1 || !storedBuf) break;
				buf[count].buf = storedBuf->getHeaderPtr();
				buf[count].len = storedBuf->getPacketSize();
				completeSend->push(storedBuf);
				count++;
			}
			if (count >= maxWSABUF)
				break;
		} while (!Send->empty());

		_User->sendCount = count;
		if (count == 0)
		{
			InterlockedCompareExchange(&(_User->sendFlag), FALSE, TRUE);
			return;
		}
		InterlockedIncrement(&(_User->iocpCount));
		retval = WSASend(_User->Sock, buf, _User->sendCount, &sendVal, 0, &_User->sendOver, NULL);
		if (retval == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err != WSA_IO_PENDING)
			{
				if (err != WSAECONNRESET && err != WSAESHUTDOWN && err != WSAECONNABORTED)
					onError(err, L"SEND ERROR");
				
				_User->sendCount = 0;
				InterlockedCompareExchange(&(_User->sendFlag), FALSE, TRUE);

				clientShutdown();
				if (0 == InterlockedDecrement(&(_User->iocpCount)))
					disconnect();
			}
		}
	}
}

void LanClient::completeRecv(LONG _trans, connectedClient *_User)
{
	int usedSize;
	int retval = 0;
	if (_trans == 0)
	{
		clientShutdown();
		return;
	}
	else
	{
		winBuffer *_recv = &_User->recvQ;
		_recv->moveRearPos(_trans);
		while (usedSize = _recv->getUsedSize())
		{
			short dataSize;
			retval = _recv->peek((char*)&dataSize, sizeof(short));
			if (retval == 0 || (usedSize - sizeof(short)) < dataSize)
				break;
			InterlockedIncrement(&recvTPS);
			try
			{
				_recv->removeData(sizeof(short));
				Sbuf *buf = Sbuf::lanAlloc();
				retval = _recv->dequeue(buf->getDataPtr(),dataSize);
				buf->moveRearPos(dataSize);
				if(buf->lanDecode())
					onRecv(buf);
				buf->Free();
			}
			catch (int num)
			{
				if (num != 4900)
				return;
			}
		}
		recvPost(_User);
	}
}

void LanClient::completeSend(LONG _trans, connectedClient *_User)
{
	if (_trans == 0)
	{
		InterlockedDecrement(&(_User->sendFlag));
		clientShutdown();
		return;
	}
	else
	{
		Sbuf *storedBuf;
		boost::lockfree::queue<Sbuf*> *completeSend = &_User->completeSendQ;
		for (int i = 0; i < _User->sendCount;)
		{
			storedBuf = NULL;
			completeSend->pop(storedBuf);
			if (!storedBuf) continue;
			storedBuf->Free();
			i++;
			InterlockedIncrement(&sendTPS);
		}
		InterlockedDecrement(&(_User->sendFlag));
	}
	sendPost(_User);
}

void LanClient::clientShutdown()
{
	InterlockedExchange(&(user->disconnectFlag), 1);
	shutdown(user->Sock, SD_BOTH);
}

void LanClient::disconnect()
{
	if (user->disconnectFlag == true)
	{
		if (InterlockedCompareExchange(&(user)->disconnectFlag, 0, 1))
		{
			onClientLeave();
			Sbuf *buf = NULL;
			while (1)
			{
				buf = NULL;
				user->sendQ.pop(buf);
				if (!buf) break;
				buf->Free();
			}
			closesocket(user->Sock);
			user->Sock = socket(AF_INET, SOCK_STREAM, 0);
			connectFlag = false;
		}
	}
}

void LanClient::disconnect(SOCKET _sock)
{
	closesocket(_sock);
}
