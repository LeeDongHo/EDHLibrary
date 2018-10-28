#include "stdafx.h"

__int64 netId = 0;

bool IOCPClient::Start(char *_ip, unsigned short _port, unsigned short _threadCount, bool _nagle, unsigned int _maxClient)
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		OnError(0, L"윈속 초기화 에러");
		return false;
	}

	hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (!hcp)
	{
		OnError(0, L"IOCP HANDLE CREATE ERROR");
		return false;
	}



	// bind()
	ZeroMemory(&addr, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(_ip);
	addr.sin_port = htons(_port);

	// Keep Alive option setting
	tcpKeep.onoff = 1;
	tcpKeep.keepalivetime = 5500;			// 5.5 sec
	tcpKeep.keepaliveinterval = 1000;	// 1 sec

	// 스택생성
	connectStack = new lockFreeStack<unsigned __int64>;

	// 배열 설정
	maxClient = _maxClient;
	sessionArr = new Session[_maxClient];
	threadArr = new HANDLE[_threadCount];
	indexStack = new lockFreeStack<unsigned __int64>;
	for (unsigned int i = 0; i < _maxClient; i++)
	{
		indexStack->push(i);
		connectStack->push(i);
	}

	// init
	connectTotal = 0;
	connectTPS = 0;
	sendTPS = 0;
	recvTPS = 0;
	clientCounter = 0;
	quit = true;
	ctQuit = true;
	nagleOpt = _nagle;

	// 스레드 생성
	threadCount = _threadCount;

	HANDLE hThread;

	hThread = (HANDLE)_beginthreadex(NULL, 0, connectThread, (LPVOID)this, 0, 0);
	CloseHandle(hThread);

	for (int i = 0; i < _threadCount; i++)
		threadArr[i] = (HANDLE)_beginthreadex(NULL, 0, workerThread, (LPVOID)this, 0, 0);

	return true;
}

bool IOCPClient::Stop(void)
{
	// accept 스레드 종료

	// print  스레드 종료
	quit = false;
	ctQuit = false;

	// 연결된 session 모두 disconnect;
	printf("session disconnet waiting...\n");
	for (int i = 0; i < maxClient; i++)
	{
		if (sessionArr[i].Sock != NULL)
		{
			shutdown(sessionArr[i].Sock, SD_BOTH);
			disconnect(&sessionArr[i]);
		}
	}
	printf("session disconnet success\n");

	// worker 스레드 종료
	for (int j = 0; j < threadCount; j++)
	{
		PostQueuedCompletionStatus(hcp, 0, 0, 0);
	}


	// 대기 : 모든 스레드가 종료 될 때 까지
	WaitForMultipleObjects(threadCount, threadArr, TRUE, INFINITE);
	printf("Net thread is closed\n");

	CloseHandle(hcp);
	delete[] sessionArr;
	delete[] threadArr;
	delete connectStack;
	delete indexStack;	// 소멸자에서 clear()
	WSACleanup();

	return true;
}

int IOCPClient::GetClientCount(void)
{
	return clientCounter;
}

void IOCPClient::SendPacket(unsigned __int64 _index, Sbuf *_buf, bool _type)
{
	Session *ss = acquirLock(_index);
	if (!ss)
		return;

	int retval;
	_buf->Encode();
	_buf->addRef();
	ss->sendQ.enqueue(_buf);

	if (_type == true)
	{
		InterlockedCompareExchange((LONG*)&ss->sendDisconnectFlag, true, false);
		if (ss->sendQ.getUsedSize() == 0)
			clientShutdown(ss->Index);
	}

	if (ss->sendFlag == false)
	{
		if (ss->sendPQCS == 0)
		{
			if( 0 == InterlockedCompareExchange((LONG*)&ss->sendPQCS, 1, 0))
			{
				PostQueuedCompletionStatus(hcp, 0, (ULONG_PTR)ss, (LPOVERLAPPED)1);
			}
		}
	}

	releaseLock(ss);
	return;
}

// private 

unsigned __stdcall IOCPClient::connectThread(LPVOID _data)
{
	IOCPClient *server = (IOCPClient*)_data;
	int retval = 0;
	int arrIndex = 0;
	Session *ss = NULL;
	bool Nagle = server->nagleOpt;
	unsigned __int64 Index = 0;
	SOCKADDR_IN addr = server->addr;
	tcp_keepalive keep = server->tcpKeep;
	int count = 0;
	int err = 0;
	lockFreeStack<unsigned __int64> *connectStack = server->connectStack;
	while (server->ctQuit)
	{
		count = 0;
			while (count < 300 && connectStack->getUsedSize() != 0)
			{
				count++;
				Index = 0;
				// stack pop
				connectStack->pop(&Index);
				arrIndex = server->getIndex(Index);
				//socket
				SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
				if (sock == INVALID_SOCKET)
				{
					_SYSLOG(L"ERROR", Level::SYS_ERROR, L"SOCKET ERROR CODE : %d", INVALID_SOCKET);
					break;
				}

				// nagle option
				if (Nagle == true)
				{
					bool optval = TRUE;
					setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));
				}

				// keepAlive option
				WSAIoctl(sock, SIO_KEEPALIVE_VALS, (tcp_keepalive*)&keep, sizeof(tcp_keepalive), NULL, 0, 0, NULL, NULL);

				// connect
				retval = connect(sock, (SOCKADDR*)&addr, sizeof(addr)); // WSACeept사용합시다.
				if (retval == SOCKET_ERROR)	// closed listensock : 종료
				{
					err = WSAGetLastError();
					_SYSLOG(L"ERROR", Level::SYS_ERROR, L"CONNECT ERROR CODE : %d", err);
					continue;
				}
				server->connectTPS++;
				server->connectTotal++;

				ss = server->insertSession(sock);
				server->sessionArr[arrIndex].Sock = sock;
				CreateIoCompletionPort((HANDLE)sock, server->hcp, (ULONG_PTR)ss, 0);	// iocp 등록
				InterlockedIncrement(&server->clientCounter);
				ss->recvFlag = true;
				server->recvPost(ss);
				server->OnClinetJoin(ss->Index);
			}
			Sleep(1000);
	}
	return 0;
}

unsigned __stdcall IOCPClient::3Thread(LPVOID _data)
{
	IOCPClient *server = (IOCPClient*)_data;

	int retval = 1;
	DWORD trans = 0;
	OVERLAPPED *over = NULL;
	Session *_ss = NULL;

	while (1)
	{
		trans = 0;
		over = NULL, _ss = NULL;
		retval = GetQueuedCompletionStatus(server->hcp, &trans, (PULONG_PTR)&_ss, (LPOVERLAPPED*)&over, INFINITE);
		if (!over)
		{
			if (retval == false)
			{
				server->OnError(WSAGetLastError(), L"GQCS error : overlapped is NULL and return false");
				break;
			}
			if (trans == 0 && !_ss)		// 종료 신호
			{
				break;
			}
		}
		else
		{
			if (1 == (int)over)
			{
				server->sendPost(_ss);
				if (_ss->sendFlag == false && _ss->sendQ.getUsedSize() > 0)
					server->sendPost(_ss);
				InterlockedDecrement((LONG*)&_ss->sendPQCS);
			}

			if (&(_ss->recvOver) == over)
				server->completeRecv(trans, _ss);

			if (&(_ss->sendOver) == over)
				server->completeSend(trans, _ss);

			if (0 == _ss->recvFlag)
			{
				if(_ss->sendFlag == false && _ss->usingFlag == false && _ss->sendPQCS == false)
					server->disconnect(_ss);
			}
		}

	}
	return 0;
}

Session* IOCPClient::insertSession(SOCKET _sock)
{
	unsigned __int64 index;
	indexStack->pop(&index);
	if (index == -1)
		return NULL;

	InterlockedExchange((LONG*)&sessionArr[index].disconnectFlag,0);
	sessionArr[index].Index = setID(index, netId);
	netId++;
	sessionArr[index].Sock = _sock;

	return &sessionArr[index];
}

int IOCPClient::getConnectTotal(void)
{
	return connectTotal;
}

int IOCPClient::getConnetTPS(void)
{
	return connectTPS;
}

int IOCPClient::getSendTPS(void)
{
	return sendTPS;
}

int IOCPClient::getRecvTPS(void)
{
	return recvTPS;
}

void IOCPClient::setTPS(void)
{
	connectTPS = 0;
	InterlockedExchange(&sendTPS, 0);
	InterlockedExchange(&recvTPS, 0);
	return;
}

__int64 IOCPClient::setID(int _index, int _clientID)
{
	__int64 result = (_index << 48) | _clientID;
	return result;
}

int IOCPClient::getID(int _clientID)
{
	int result = (_clientID << 16) >> 16;

	return result;
}

int IOCPClient::getIndex(int _id)
{
	int result = _id >> 48;
	return result;
}

void IOCPClient::recvPost(Session *_ss)
{

	DWORD recvVal, flag;
	int retval, err;
	winBuffer *_recv = &_ss->recvQ;
	ZeroMemory(&(_ss->recvOver), sizeof(_ss->recvOver));
	WSABUF wbuf[2];
	ZeroMemory(&wbuf, sizeof(WSABUF) * 2);
	wbuf[0].buf = _recv->getRearPosPtr();
	wbuf[0].len = _recv->getNotBrokenFreeSize();

	if (recvQ->getFreeSize() == 0)
		_ss->disconnect();

	if (_recv->getFreeSize() > _recv->getNotBrokenFreeSize())
	{
		wbuf[1].buf = _recv->getBufferPtr();
		wbuf[1].len = (_recv->getFreeSize() - _recv->getNotBrokenFreeSize());
	}
	// RECV 공간이 없는 경우 예외 처리 해주세요. 패킷자체가 문제가 있는것. 헤더의 길이가 잘못 됨
	// 클라가 깨진 패킷을 쐈거나 누군가 공격을 하는 것 이므로 연결을 끊으세요. 중대한 에러입니다. 
	recvVal = 0, flag = 0;
	retval = WSARecv(_ss->Sock, wbuf, 2, &recvVal, &flag, &(_ss->recvOver), NULL);

	if (retval == SOCKET_ERROR)
	{
		// PENDING 인 경우 나중에 처리된다는 의미
		err = WSAGetLastError();
		if (err != WSA_IO_PENDING)
		{
			_ss->recvFlag = false;
			if (err != WSAECONNRESET && err != WSAESHUTDOWN && err != WSAECONNABORTED)
			{
				_SYSLOG(L"ERROR", Level::SYS_ERROR, L"RECV POST ERROR CODE : %d", err);
				WCHAR errString[512] = L"";
				wsprintf(errString, L"RECV ERROR [SESSION_ID : %d] : %d", _ss->Index, err);
				OnError(0, errString);
			}
			clientShutdown(_ss);
		}
	}
	return;
}

void IOCPClient::sendPost(Session *_ss)
{
	DWORD sendVal = 0;

	int count = 0;
	int result = 0;
	int size = 0;
	WSABUF wbuf[maxWSABUF];
	wbuf[count].buf = 0;
	wbuf[count].len = 0;
	size = _ss->sendQ.getUsedSize();
	if (size <= 0)
		return;
	if (0 == InterlockedCompareExchange((LONG*)&(_ss->sendFlag), 1, 0))
	{
		_ss->sendCount = 0;
		count = 0;
		int retval = 0;
		int count = 0;
		Sbuf *buf;
		ZeroMemory(&_ss->sendOver, sizeof(_ss->sendOver));
		lockFreeQueue<Sbuf*> *_send = &_ss->sendQ;
		do
		{
			for (count; count < maxWSABUF; )
			{
				buf = NULL;
				retval = _send->peek(&buf, count);
				if (retval == -1 || !buf) break;
				wbuf[count].buf = buf->getHeaderPtr();
				wbuf[count].len = buf->getPacketSize();
				count++;
			}
			if (count >= maxWSABUF)
				break;
		} while (_send->getUsedSize() > count);

		_ss->sendCount = count;
		if (count == 0)
		{
			InterlockedExchange((LONG*)&(_ss->sendFlag), 0);
			return;
		}
		retval = WSASend(_ss->Sock, wbuf, _ss->sendCount, &sendVal, 0, &_ss->sendOver, NULL);
		if (retval == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err != WSA_IO_PENDING)
			{
				if (err != WSAECONNRESET && err != WSAESHUTDOWN && err != WSAECONNABORTED)
				{
					_SYSLOG(L"ERROR", Level::SYS_ERROR, L"SEND POST ERROR CODE : %d", err);
					WCHAR errString[512] = L"";
					wsprintf(errString, L"SEND ERROR [SESSION_ID : %d] : %d", _ss->Index, err);
					OnError(0, errString);
				}
				_ss->sendCount = 0;
				clientShutdown(_ss);
				InterlockedExchange((LONG*)&(_ss->sendFlag), 0);
				// 다른 스레드에서  sendPost를 호출하는 것을 막아보기 위해 shutdown함수 호출 다음에 sendFlag 변경
			}
		}
	}

	return;
}

void IOCPClient::completeRecv(LONG _trans, Session *_ss)
{
	int usedSize;
	int retval = 0;
	if (_trans == 0)
	{
		clientShutdown(_ss);
		_ss->recvFlag = false;
		return;
	}
	else
	{
		winBuffer *_recv = &_ss->recvQ;
		_recv->moveRearPos(_trans);
		while (usedSize = _recv->getUsedSize())
		{
			netHeader head;
			retval = _recv->peek((char*)&head, sizeof(netHeader));
			if (retval == 0 || (usedSize - sizeof(netHeader)) < head.len)
				break;
			InterlockedIncrement(&recvTPS);
			try
			{
				Sbuf *buf = Sbuf::Alloc();
				retval = _recv->dequeue(buf->getBufPtr(), sizeof(netHeader) + head.len);
				buf->moveRearPos(head.len);
				if (buf->Decode())
					OnRecv(_ss->Index, buf);
				else
					throw 4900;
				buf->Free();
			}
			catch (int num)
			{
				if (num != 4900)
					_SYSLOG(L"SYSTEM", Level::SYS_ERROR, L"에러코드 : %d", num);
				return;
			}
		}
		recvPost(_ss);
	}
	return;
}

void IOCPClient::completeSend(LONG _trans, Session *_ss)
{
	if (_trans == 0)
	{
		InterlockedDecrement((LONG*)&(_ss->sendFlag));
		clientShutdown(_ss);
		return;
	}
	else
	{
		Sbuf *buf;
		lockFreeQueue<Sbuf*> *_send = &_ss->sendQ;
		for (int i = 0; i < _ss->sendCount;)
		{
			buf = NULL;
			_send->dequeue(&buf);
			if (!buf) continue;
			buf->Free();
			i++;
			InterlockedIncrement(&sendTPS);
		}
		InterlockedDecrement((LONG*)&(_ss->sendFlag));
	}
	if (_ss->sendQ.getUsedSize() == 0)
	{
		if (1 == InterlockedCompareExchange((LONG*)&(_ss->sendDisconnectFlag), 1, 1))
			clientShutdown(_ss->Index);
	}
	sendPost(_ss);
}

void IOCPClient::clientShutdown(Session *_ss)
{
	InterlockedExchange((LONG*)&_ss->disconnectFlag, 1);
	shutdown(_ss->Sock, SD_BOTH);
	return;
}

void IOCPClient::disconnect(Session *_ss)
{
	ULONG64 dummyIndex = 0;
	if (_ss->disconnectFlag == true)
	{
		if (1 == InterlockedCompareExchange((LONG*)&_ss->disconnectFlag, 1, 1))
		{
			OnClientLeave(_ss->Index);
			Sbuf *buf = NULL;
			InterlockedDecrement(&clientCounter);
			while (1)
			{
				buf = NULL;
				_ss->sendQ.dequeue(&buf);
				if (!buf) break;
				buf->Free();
			}
			closesocket(_ss->Sock);
			_ss->Sock = INVALID_SOCKET;
			dummyIndex = _ss->Index;
			_ss->Index = 0;
			indexStack->push(getIndex(dummyIndex));
		}
	}
	return;
}

void IOCPClient::disconnect(SOCKET _sock)
{
	shutdown(_sock, SD_BOTH);
	closesocket(_sock);
	return;
}

Session* IOCPClient::acquirLock(unsigned __int64 _id)
{
	__int64 index = getIndex(_id);
	Session *ss = &sessionArr[index];

	if (1 == InterlockedCompareExchange((LONG*)&ss->usingFlag,0,1))
	{
		if (true == ss->disconnectFlag)
			disconnect(ss);
		return NULL;
	}

	if (ss->Index != _id)
	{
		if (true == ss->disconnectFlag)
			disconnect(ss);
		return NULL;
	}

	if (true == ss->disconnectFlag)
	{
		disconnect(ss);
		return NULL;
	}

	else
		return &sessionArr[index];
	return NULL;
}

void IOCPClient::releaseLock(Session *_ss)
{
	InterlockedExchange((LONG*)&_ss->usingFlag, 0);
	if (true == _ss->disconnectFlag)
		disconnect(_ss);
	return;
}

void IOCPClient::clientShutdown(unsigned __int64 _index)
{
	Session *_ss = acquirLock(_index);
	if (_ss)
	{
		InterlockedExchange((LONG*)&sessionArr[_index].disconnectFlag, 1);
		shutdown(_ss->Sock, SD_BOTH);
	}
	releaseLock(_ss);
	return;
}