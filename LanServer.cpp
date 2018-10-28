#include "stdafx.h"

__int64 lanId = 0;

#define setID(index, clientID) (index << 48) | clientID
#define getID(clientID) (clientID<<16) >>16
#define getIndex(clientID) clientID>>48


bool LanServer::Start(char *_ip, unsigned short _port, unsigned short _threadCount, bool _nagle, unsigned int _maxSession)
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		OnError(0, L"윈속 초기화 에러");
		return false;
	}

	listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSock == INVALID_SOCKET)
	{
		OnError(0, L"LISTEN 에러");
		return false;
	}

	hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (!hcp)
	{
		OnError(0, L"IOCP HANDLE CREATE ERROR");
		return false;
	}

	// nagle check
	if (!_nagle)
	{
		bool optval = TRUE;
		setsockopt(listenSock, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));
	}

	// bind()
	SOCKADDR_IN serverAddr;
	IN_ADDR addr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));

	serverAddr.sin_family = AF_INET;
	//serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_addr.s_addr = inet_addr(_ip);
	serverAddr.sin_port = htons(_port);

	int retval = bind(listenSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (retval == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		OnError(err, L"BIND ERROR");
		return false;
	}

	// listen()
	retval = listen(listenSock, SOMAXCONN);
	if (retval == INVALID_SOCKET)
	{
		OnError(0, L"LISTEN ERROR");
		return false;
	}

	tcp_keepalive tcpKeep;
	tcpKeep.onoff = 1;
	tcpKeep.keepalivetime = 10000;
	tcpKeep.keepaliveinterval = 10;
	WSAIoctl(listenSock, SIO_KEEPALIVE_VALS, (tcp_keepalive*)&tcpKeep, sizeof(tcp_keepalive), NULL, 0, NULL, NULL, NULL);

	// 배열 설정
	maxCounter = _maxSession;
	sessionArry = new lanSession[_maxSession];
	indexStack = new lockFreeStack<__int64>;
	for (unsigned int i = 0; i < _maxSession; i++)
	{
		indexStack->push(i);
	}

	// init
	acceptTotal = 0;
	acceptTPS = 0;
	sendTPS = 0;
	recvTPS = 0;
	LANclientCounter = 0;

	// 스레드 생성
	workerCount = _threadCount;

	threadArr = new HANDLE[workerCount];

	HANDLE threadHandle = (HANDLE)_beginthreadex(NULL, 0, acceptThread, (LPVOID)this, 0, 0);
	CloseHandle(threadHandle);
	threadHandle = (HANDLE)_beginthreadex(NULL, 0, tpsThread, (LPVOID)this, 0, 0);
	CloseHandle(threadHandle);


	for (int i = 0; i < workerCount; i++)
		threadArr[i] = (HANDLE)_beginthreadex(NULL, 0, workerThread, (LPVOID)this, 0, 0);

	return true;
}

bool LanServer::Stop(void)
{
	// accept 스레드 종료
	closesocket(listenSock);

	// print  스레드 종료
	quit = true;


	// 연결된 session 모두 disconnect

	printf("session disconnet waiting...\n");
	for (unsigned int i = 0; i < maxCounter; i++)
	{
		if (sessionArry[i].Sock != NULL)
		{
			shutdown(sessionArry[i].Sock, SD_BOTH);
			disconnect(&sessionArry[i]);
		}
	}
	printf("session disconnet success\n");

	// worker 스레드 종료
	for (unsigned int j = 0; j < workerCount; j++)
		PostQueuedCompletionStatus(threadArr[j], 0, 0, 0);

	// 대기 : 모든 스레드가 종료 될 때 까지
	WaitForMultipleObjects(workerCount, threadArr, TRUE, INFINITE);
	printf("Net thread is closed\n");
	for (unsigned int a = 0; a < workerCount; a++)
		CloseHandle(threadArr[a]);

	delete[] threadArr;
	delete[] sessionArry;
	delete indexStack;	// 소멸자에서 clear()
	WSACleanup();

	return true;
}

int LanServer::GetClientCount(void)
{
	return LANclientCounter;
}

void LanServer::SendPacket(unsigned __int64 _id, Sbuf *_buf)
{
	lanSession *dummy = acquirLock(_id);
	if (!dummy)
		return;

	int retval;
	_buf->lanEncode();
	_buf->addRef();
	dummy->sendQ.enqueue(_buf);

	if (dummy->sendFlag == false)
		sendPost(dummy);


	releaseLock(dummy);
	return;
}

// private 



unsigned __stdcall LanServer::acceptThread(LPVOID _data)
{
	LanServer *dummy = (LanServer*)_data;
	SOCKET clientSock = 0;
	SOCKADDR_IN clientAddr;
	SOCKET listenSock = dummy->listenSock;
	lanSession* ss = NULL;
	int len;
	srand((unsigned int)_data);
	while (1)
	{
		len = sizeof(clientAddr);
		clientSock = accept(listenSock, (SOCKADDR*)&clientAddr, &len); // WSACeept사용합시다.
		if (clientSock == INVALID_SOCKET)	// closed listensock : 종료
		{
			break;
		}
		if (dummy->maxCounter == dummy->LANclientCounter)
		{
			dummy->disconnect(clientSock);
			continue;
		}
		dummy->acceptTPS++;
		dummy->acceptTotal++;
		WCHAR ip[16];
		InetNtop(AF_INET, &clientAddr.sin_addr, ip, 16);

		ss = dummy->insertSession(clientSock);
		CreateIoCompletionPort((HANDLE)clientSock, dummy->hcp, (ULONG_PTR)ss, 0);
		InterlockedIncrement(&dummy->LANclientCounter);
		dummy->OnClientJoin(ss->Index);
		dummy->recvPost(ss);
		if (0 == InterlockedDecrement(&(ss->iocpCount)))
			dummy->disconnect(ss);
	}
	return 0;
}

unsigned __stdcall LanServer::workerThread(LPVOID _data)
{
	LanServer *dummy = (LanServer*)_data;

	int retval = 1;
	DWORD trans = 0;
	OVERLAPPED *over = NULL;
	lanSession *_ss = NULL;

	srand(GetCurrentThreadId());
	while (1)
	{
		trans = 0;
		over = NULL, _ss = NULL;
		retval = GetQueuedCompletionStatus(dummy->hcp, &trans, (PULONG_PTR)&_ss, (LPOVERLAPPED*)&over, INFINITE);
		if (!over)
		{
			if (retval == false)
			{
				dummy->OnError(WSAGetLastError(), L"GQCS error : overlapped is NULL and return false");
				break;
			}
			if (trans == 0 && !_ss)		// 종료 신호
			{
				break;
			}
		}
		else
		{
			if (&(_ss->recvOver) == over)
				dummy->completeRecv(trans, _ss);

			if (&(_ss->sendOver) == over)
				dummy->completeSend(trans, _ss);

			// 멤버변수 하나 추가해줘서 값 받아놓자. 덤프 썼을 때 리턴값 확인용도. iocount<0 이면 crash
			if (0 == InterlockedDecrement(&(_ss->iocpCount)))
				dummy->disconnect(_ss);
		}

	}
	return 0;
}

unsigned __stdcall LanServer::tpsThread(LPVOID _data)
{
	LanServer *dummy = (LanServer*)_data;
	while (1)
	{
		if (dummy->quit) break;
		if (dummy->LANclientCounter < 0)
			CCrashDump::Crash();

		dummy->pacceptTPS = dummy->getAcceptTPS();
		dummy->psendTPS = dummy->getSendTPS();
		dummy->precvTPS = dummy->getRecvTPS();
		dummy->setTPS();
		Sleep(999);
	}
	return 0;
}

lanSession* LanServer::insertSession(SOCKET _sock)
{
	__int64 index;
	indexStack->pop(&index);
	if (index == -1)
		return NULL;

	sessionArry[index].Index = setID(index, lanId);
	lanId++;
	sessionArry[index].Sock = _sock;

	// clear session
	sessionArry[index].sendFlag = 0;
	InterlockedIncrement(&(sessionArry[index].iocpCount));
	sessionArry[index].disconnectFlag = false;
	return &sessionArry[index];
}

int LanServer::getAcceptTotal(void)
{
	return acceptTotal;
}

int LanServer::getAcceptTPS(void)
{
	return acceptTPS;
}

int LanServer::getSendTPS(void)
{
	return sendTPS;
}

int LanServer::getRecvTPS(void)
{
	return recvTPS;
}

void LanServer::setTPS(void)
{
	acceptTPS = 0;
	InterlockedExchange(&sendTPS, 0);
	InterlockedExchange(&recvTPS, 0);
	return;
}

void LanServer::recvPost(lanSession *_ss)
{

	DWORD recvVal, flag;
	int retval, err;
	winBuffer *_recv = &_ss->recvQ;
	ZeroMemory(&(_ss->recvOver), sizeof(_ss->recvOver));
	WSABUF buf[2];
	ZeroMemory(&buf, sizeof(WSABUF) * 2);
	buf[0].buf = _recv->getRearPosPtr();
	buf[0].len = _recv->getNotBrokenFreeSize();


	if (_recv->getFreeSize() > _recv->getNotBrokenFreeSize())
	{
		buf[1].buf = _recv->getBufferPtr();
		buf[1].len = (_recv->getFreeSize() - _recv->getNotBrokenFreeSize());
	}
	// RECV 공간이 없는 경우 예외 처리 해주세요. 패킷자체가 문제가 있는것. 헤더의 길이가 잘못 됨
	// 클라가 깨진 패킷을 쐈거나 누군가 공격을 하는 것 이므로 연결을 끊으세요. 중대한 에러입니다. 
	recvVal = 0, flag = 0;
	InterlockedIncrement(&(_ss->iocpCount));
	retval = WSARecv(_ss->Sock, buf, 2, &recvVal, &flag, &(_ss->recvOver), NULL);

	if (retval == SOCKET_ERROR)
	{
		// PENDING 인 경우 나중에 처리된다는 의미
		err = WSAGetLastError();
		if (err != WSA_IO_PENDING)
		{
			if (err != WSAECONNRESET && err != WSAESHUTDOWN && err != WSAECONNABORTED)
			{
				WCHAR errString[512] = L"";
				wsprintf(errString, L"RECV ERROR [SESSION_ID : %d] : %d", _ss->Index, err);
				OnError(0, errString);
			}
			clientShutdown(_ss);
			if (0 == InterlockedDecrement(&(_ss->iocpCount)))
				disconnect(_ss);
		}
	}
	return;
}

void LanServer::sendPost(lanSession *_ss)
{
	DWORD sendVal = 0;

	int count = 0;
	int result = 0;
	int size = 0;
	WSABUF buf[maxWSABUF];
	buf[count].buf = 0;
	buf[count].len = 0;
	size = _ss->sendQ.getUsedSize();
	if (size <= 0)
		return;
	if (FALSE == (BOOL)InterlockedCompareExchange(&(_ss->sendFlag), TRUE, FALSE))
	{
		_ss->sendCount = 0;
		count = 0;
		int retval = 0;
		int count = 0;
		Sbuf *dummy;
		ZeroMemory(&_ss->sendOver, sizeof(_ss->sendOver));
		lockFreeQueue<Sbuf*> *_send = &_ss->sendQ;
		do
		{
			for (count; count < maxWSABUF; )
			{
				dummy = NULL;
				retval = _send->peek(&dummy, count);
				if (retval == -1 || !dummy) break;
				buf[count].buf = dummy->getHeaderPtr();
				buf[count].len = dummy->getPacketSize();
				count++;
			}
			if (count >= maxWSABUF)
				break;
		} while (_send->getUsedSize() > count);

		_ss->sendCount = count;
		if (count == 0)
		{
			InterlockedCompareExchange(&(_ss->sendFlag), FALSE, TRUE);
			return;
		}
		InterlockedIncrement(&(_ss->iocpCount));
		retval = WSASend(_ss->Sock, buf, _ss->sendCount, &sendVal, 0, &_ss->sendOver, NULL);
		if (retval == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err != WSA_IO_PENDING)
			{
				if (err != WSAECONNRESET && err != WSAESHUTDOWN && err != WSAECONNABORTED)
				{
					WCHAR errString[512] = L"";
					wsprintf(errString, L"SEND ERROR [SESSION_ID : %d] : %d", _ss->Index, err);
					OnError(0, errString);
				}
				_ss->sendCount = 0;
				InterlockedCompareExchange(&(_ss->sendFlag), FALSE, TRUE);

				clientShutdown(_ss);
				if (0 == InterlockedDecrement(&(_ss->iocpCount)))
					disconnect(_ss);
			}
		}
	}

	return;
}

void LanServer::completeRecv(LONG _trans, lanSession *_ss)
{
	int usedSize;
	int retval = 0;
	if (_trans == 0)
	{
		clientShutdown(_ss);
		return;
	}
	else
	{
		winBuffer *_recv = &_ss->recvQ;
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
				retval = _recv->dequeue(buf->getDataPtr(), dataSize);
				buf->moveRearPos(dataSize);
				if (buf->lanDecode())
					OnRecv(_ss->Index, buf);
				buf->Free();
			}
			catch (int num)
			{
				if (num != 4900)
					OnError(num, L"ERROR");
				return;
			}
		}
		recvPost(_ss);
	}
	return;
}

void LanServer::completeSend(LONG _trans, lanSession *_ss)
{
	if (_trans == 0)
	{
		InterlockedDecrement(&(_ss->sendFlag));
		clientShutdown(_ss);
		return;
	}
	else
	{
		Sbuf *dummy;
		lockFreeQueue<Sbuf*> *_send = &_ss->sendQ;
		for (int i = 0; i < _ss->sendCount;)
		{
			dummy = NULL;
			_send->dequeue(&dummy);
			if (!dummy) continue;
			dummy->Free();
			i++;
			InterlockedIncrement(&sendTPS);
		}
		InterlockedDecrement(&(_ss->sendFlag));
	}
	sendPost(_ss);
}

void LanServer::clientShutdown(lanSession *_ss)
{
	InterlockedExchange((LONG*)&_ss->disconnectFlag, 1);
	shutdown(_ss->Sock, SD_BOTH);
	return;
}

void LanServer::disconnect(lanSession *_ss)
{
	unsigned __int64 dummyID = 0;
	if (_ss->disconnectFlag == true)
	{
		if (true == InterlockedCompareExchange(&(_ss->disconnectFlag), false, true))
		{
			shutdown(_ss->Sock, SD_BOTH);
			OnClientLeave(_ss->Index);
			Sbuf *buf = NULL;
			InterlockedDecrement(&LANclientCounter);
			while (1)
			{
				buf = NULL;
				_ss->sendQ.dequeue(&buf);
				if (!buf) break;
				buf->Free();
			}
			closesocket(_ss->Sock);
			_ss->Sock = INVALID_SOCKET;
			dummyID = _ss->Index;
			_ss->Index = 0;
			indexStack->push(getIndex(dummyID));
		}
	}
	return;
}

void LanServer::disconnect(SOCKET _sock)
{
	shutdown(_sock, SD_BOTH);
	closesocket(_sock);
	return;
}

lanSession* LanServer::acquirLock(unsigned __int64 _id)
{
	__int64 index = getIndex(_id);
	lanSession *ss = &sessionArry[index];

	if (1 == InterlockedIncrement(&(ss->iocpCount)))
	{
		if (0 == InterlockedDecrement(&(ss->iocpCount)))
			disconnect(ss);
		return NULL;
	}

	if (ss->Index != _id)
	{
		if (0 == InterlockedDecrement(&(ss->iocpCount)))
			disconnect(ss);
		return NULL;
	}

	if (ss->disconnectFlag == true)
	{
		disconnect(ss);
		return NULL;
	}

	else
		return &sessionArry[index];
	return NULL;
}

void LanServer::releaseLock(lanSession *_ss)
{
	if (0 == InterlockedDecrement(&(_ss->iocpCount)))
		disconnect(_ss);
	return;
}

void LanServer::clientShutdown(unsigned __int64 _id)
{
	lanSession *_ss = acquirLock(_id);
	if (_ss)
	{
		shutdown(_ss->Sock, SD_BOTH);
		releaseLock(_ss);
	}
	return;
}