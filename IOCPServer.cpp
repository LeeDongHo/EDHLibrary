#include "stdafx.h"

__int64 netId = 0;

bool IOCPServer::Start(char *_ip, unsigned short _port, unsigned short _threadCount, bool _nagle, unsigned int _maxClient)
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

	// bind()
	SOCKADDR_IN serverAddr;
	IN_ADDR addr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));

	serverAddr.sin_family = AF_INET;

	if (strcmp("0.0.0.0", _ip) == 0)
		serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	else
		inet_pton(AF_INET, _ip, &addr);

	serverAddr.sin_port = htons(_port);

	bool optval = TRUE;
	// nagle check
	if (_nagle == true)
		setsockopt(listenSock, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));
	// Set Reuseaddr option.
	//setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));

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

	// 배열 설정
	maxClient = _maxClient;
	sessionArr = new Session[_maxClient];
	indexStack = new boost::lockfree::stack<unsigned __int64>;
	for (unsigned int i = 0; i < _maxClient; i++)
	{
		indexStack->push(i);
	}

	// init
	acceptTotal = 0;
	acceptTPS = 0;
	sendTPS = 0;
	recvTPS = 0;
	clientCounter = 0;

	// 스레드 생성
	allThreadCount = _threadCount + 1;
	threadCount = _threadCount;

	threadArr = new HANDLE[allThreadCount];

	threadArr[0] = (HANDLE)_beginthreadex(NULL, 0, acceptThread, (LPVOID)this, 0, 0);

	for (int i = 1; i < allThreadCount; i++)
		threadArr[i] = (HANDLE)_beginthreadex(NULL, 0, workerThread, (LPVOID)this, 0, 0);

	return true;
}

bool IOCPServer::Stop(void)
{
	// accept 스레드 종료
	closesocket(listenSock);

	// print  스레드 종료
	quit = true;


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
	WaitForMultipleObjects(allThreadCount, threadArr, TRUE, INFINITE);
	printf("Net thread is closed\n");
	for (int a = 0; a < allThreadCount; a++)
		CloseHandle(threadArr[a]);

	CloseHandle(hcp);
	delete[] threadArr;
	delete[] sessionArr;
	delete indexStack;	// 소멸자에서 clear()
	WSACleanup();

	return true;
}

int IOCPServer::GetClientCount(void)
{
	return clientCounter;
}

void IOCPServer::setEncodeKey(BYTE _code, BYTE _key1, BYTE _key2)
{
	Code = _code;
	Key1 = _key1;
	Key2 = _key2;
}

void IOCPServer::SendPacket(unsigned __int64 _index, Sbuf *_buf, bool _type)
{
	Session *ss = acquirLock(_index);
	if (!ss)
		return;

	int retval;
	_buf->Encode(Code,Key1,Key2);
	_buf->addRef();
	ss->sendQ.push(_buf);

	if (_type == true)
	{
		InterlockedCompareExchange((LONG*)&ss->sendDisconnectFlag, true, false);
		if (ss->sendQ.empty())
			clientShutdown(ss->Index);
	}

	if (ss->sendFlag == false && ss->sendPQCS == false)
	{
			if (0 == InterlockedCompareExchange((LONG*)&ss->sendPQCS, 1, 0))
			{
				InterlockedIncrement(&(ss->iocpCount));
				PostQueuedCompletionStatus(hcp, 0, (ULONG_PTR)ss, (LPOVERLAPPED)1);
			}
	}

	releaseLock(ss);
}

// private 

unsigned __stdcall IOCPServer::acceptThread(LPVOID _data)
{
	IOCPServer *server = (IOCPServer*)_data;
	SOCKET clientSock = 0;
	SOCKADDR_IN clientAddr;
	SOCKET listenSock = server->listenSock;
	Session* ss = NULL;
	int len;

	int loopCount = 0;
	while (1)
	{
		len = sizeof(clientAddr);
		clientSock = accept(listenSock, (SOCKADDR*)&clientAddr, &len); // WSACeept사용합시다.
		if (clientSock == INVALID_SOCKET)	// closed listensock : 종료
			break;
		if (server->maxClient == server->clientCounter)
		{
			// accept는 받았으므로 보내고 끊기 기능을 사용할 수 있을까?
			server->disconnect(clientSock);
			continue;
		}
		server->acceptTPS++;
		server->acceptTotal++;
		char ip[16];
		inet_ntop(AF_INET, &clientAddr.sin_addr, ip, 16);
		if (server->OnConnectionRequest(ip, clientAddr.sin_port))	// white ip check
		{
			ss = server->insertSession(clientSock);
			CreateIoCompletionPort((HANDLE)clientSock, server->hcp, (ULONG_PTR)ss, 0);	// iocp 등록
			InterlockedIncrement(&server->clientCounter);
			server->OnClientJoin(ss->Index);
			server->recvPost(ss);
			if (0 == InterlockedDecrement(&(ss->iocpCount)))
				server->disconnect(ss);
		}
		else
			server->disconnect(clientSock);
	}
	return 0;
}

unsigned __stdcall IOCPServer::workerThread(LPVOID _data)
{
	IOCPServer *server = (IOCPServer*)_data;

	int retval = 1;
	DWORD trans = 0;
	OVERLAPPED *over = NULL;
	Session *_ss = NULL;

	srand(GetCurrentThreadId());

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
				break;
		}
		else
		{
			if (1 == (int)over)
			{
				server->sendPost(_ss);
				InterlockedDecrement((LONG*)&_ss->sendPQCS);
				if (_ss->sendFlag == false && _ss->sendQ.empty())
					server->sendPost(_ss);
			}

			if (&(_ss->recvOver) == over)
				server->completeRecv(trans, _ss);

			if (&(_ss->sendOver) == over)
				server->completeSend(trans, _ss);
			if (0 == InterlockedDecrement(&(_ss->iocpCount)))
				server->disconnect(_ss);
		}

	}
	return 0;
}

Session* IOCPServer::insertSession(SOCKET _sock)
{
	unsigned __int64 index;
	indexStack->pop(index);
	if (index == -1)
		return NULL;
	if(index == 0)
		CCrashDump::Crash();

	sessionArr[index].Index = setID(index, netId);
	netId++;
	sessionArr[index].Sock = _sock;

	InterlockedExchange(&sessionArr[index].sendFlag, false);
	InterlockedExchange(&sessionArr[index].disconnectFlag, false);
	InterlockedIncrement(&(sessionArr[index].iocpCount));

	return &sessionArr[index];
}

int IOCPServer::getAcceptTotal(void)
{
	return acceptTotal;
}

int IOCPServer::getAcceptTPS(void)
{
	return acceptTPS;
}

int IOCPServer::getSendTPS(void)
{
	return sendTPS;
}

int IOCPServer::getRecvTPS(void)
{
	return recvTPS;
}

void IOCPServer::setTPS(void)
{
	acceptTPS = 0;
	InterlockedExchange(&sendTPS, 0);
	InterlockedExchange(&recvTPS, 0);
}

void IOCPServer::recvPost(Session *_ss)
{

	DWORD recvVal, flag;
	int retval, err;
	winBuffer *_recv = &_ss->recvQ;
	ZeroMemory(&(_ss->recvOver), sizeof(_ss->recvOver));
	WSABUF wbuf[2];
	ZeroMemory(&wbuf, sizeof(WSABUF) * 2);
	wbuf[0].buf = _recv->getRearPosPtr();
	wbuf[0].len = _recv->getNotBrokenFreeSize();


	if (_recv->getFreeSize() > _recv->getNotBrokenFreeSize())
	{
		wbuf[1].buf = _recv->getBufferPtr();
		wbuf[1].len = (_recv->getFreeSize() - _recv->getNotBrokenFreeSize());
	}
	// RECV 공간이 없는 경우 예외 처리 해주세요. 패킷자체가 문제가 있는것. 헤더의 길이가 잘못 됨
	// 클라가 깨진 패킷을 쐈거나 누군가 공격을 하는 것 이므로 연결을 끊으세요. 중대한 에러입니다. 
	recvVal = 0, flag = 0;
	InterlockedIncrement(&(_ss->iocpCount));
	retval = WSARecv(_ss->Sock, wbuf, 2, &recvVal, &flag, &(_ss->recvOver), NULL);

	if (retval == SOCKET_ERROR)
	{
		// PENDING 인 경우 나중에 처리된다는 의미
		err = WSAGetLastError();
		if (err != WSA_IO_PENDING)
		{
			if (err != WSAECONNRESET && err != WSAESHUTDOWN && err != WSAECONNABORTED)
			{
				_SYSLOG(Type::Type_CONSOLE, Level::SYS_ERROR, L"RECV POST ERROR CODE : %d", err);
				WCHAR errString[512] = L"";
				wsprintf(errString, L"RECV ERROR [SESSION_ID : %d] : %d", _ss->Index, err);
				OnError(0, errString);
			}
			clientShutdown(_ss);
			if (0 == InterlockedDecrement(&(_ss->iocpCount)))
				disconnect(_ss);
		}
	}
}

void IOCPServer::sendPost(Session *_ss)
{
	DWORD sendVal = 0;

	int count = 0;
	int result = 0;
	int size = 0;
	WSABUF wbuf[maxWSABUF];
	wbuf[count].buf = 0;
	wbuf[count].len = 0;
	boost::lockfree::queue<Sbuf*> *_send = &_ss->sendQ;
	boost::lockfree::queue<Sbuf*> *completeSend = &_ss->completeSendQ;
	if (_send->empty()) return;
	if (0 == InterlockedCompareExchange(&(_ss->sendFlag), 1, 0))
	{
		_ss->sendCount = 0;
		count = 0;
		int retval = 0;
		int count = 0;
		Sbuf *buf;
		ZeroMemory(&_ss->sendOver, sizeof(_ss->sendOver));
		do
		{
			for (count; count < maxWSABUF; )
			{
				buf = NULL;
				retval = _send->pop(buf);
				if (retval == false || !buf) break;
				wbuf[count].buf = buf->getHeaderPtr();
				wbuf[count].len = buf->getPacketSize();
				completeSend->push(buf);
				count++;
			}
			if (count >= maxWSABUF)
				break;
		} while (!_send->empty());

		_ss->sendCount = count;
		if (count == 0)
		{
			InterlockedExchange(&(_ss->sendFlag), false);
			return;
		}
		InterlockedIncrement(&(_ss->iocpCount));
		retval = WSASend(_ss->Sock, wbuf, _ss->sendCount, &sendVal, 0, &_ss->sendOver, NULL);
		if (retval == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err != WSA_IO_PENDING)
			{
				if (err != WSAECONNRESET && err != WSAESHUTDOWN && err != WSAECONNABORTED)
				{
					_SYSLOG(Type::Type_CONSOLE, Level::SYS_ERROR, L"SEND POST ERROR CODE : %d", err);
					WCHAR errString[512] = L"";
					wsprintf(errString, L"SEND ERROR [SESSION_ID : %d] : %d", _ss->Index, err);
					OnError(0, errString);
				}
				_ss->sendCount = 0;
				InterlockedExchange(&(_ss->sendFlag), false);
				clientShutdown(_ss);
				if (0 == InterlockedDecrement(&(_ss->iocpCount)))
					disconnect(_ss);
				// 다른 스레드에서  sendPost를 호출하는 것을 막아보기 위해 shutdown함수 호출 다음에 sendFlag 변경
			}
		}
	}
}

void IOCPServer::completeRecv(LONG _trans, Session *_ss)
{
	int usedSize;
	int retval = 0;
	if (_trans == 0)
	{
		clientShutdown(_ss);
		return;
	}
	winBuffer *_recv = &_ss->recvQ;
	_recv->moveRearPos(_trans);
	while (usedSize = _recv->getUsedSize())
	{
		netHeader head;
		retval = _recv->peek((char*)&head, sizeof(netHeader));
		if ((usedSize - sizeof(netHeader)) < head.len || retval == 0 )
			break;
		InterlockedIncrement(&recvTPS);
		try
		{
			Sbuf *buf = Sbuf::Alloc();
			retval = _recv->dequeue(buf->getBufPtr(), sizeof(netHeader) + head.len);
			buf->moveRearPos(head.len);
			if (buf->Decode(Code, Key1, Key2))
				OnRecv(_ss->Index, buf);
			else
				CCrashDump::Crash();
			buf->Free();
		}
		catch (int num)
		{
			if (num != 4900)
				_SYSLOG(Type::Type_CONSOLE, Level::SYS_ERROR, L"에러코드 : %d", num);
		}
	}
	recvPost(_ss);
}

void IOCPServer::completeSend(LONG _trans, Session *_ss)
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
		boost::lockfree::queue<Sbuf*> *completeSend = &_ss->completeSendQ;
		for (int i = 0; i < _ss->sendCount;)
		{
			buf = NULL;
			completeSend->pop(buf);
			if (!buf) continue;
			buf->Free();
			i++;
			InterlockedIncrement(&sendTPS);
		}
		InterlockedDecrement((LONG*)&(_ss->sendFlag));
		_ss->sendCount = 0;
	}
	if (_ss->sendQ.empty() && _ss->sendDisconnectFlag)
	{
		if (false == InterlockedCompareExchange(&(_ss->disconnectFlag), true, false))
			clientShutdown(_ss->Index);
	}
	sendPost(_ss);
}

void IOCPServer::clientShutdown(Session *_ss)
{
	InterlockedExchange(&_ss->disconnectFlag, 1);
	shutdown(_ss->Sock, SD_BOTH);
}

void IOCPServer::disconnect(Session *_ss)
{
	ULONG64 dummyIndex = 0;
	if (true == InterlockedCompareExchange((LONG*)&_ss->disconnectFlag, false, true))
	{
		if (0 == InterlockedCompareExchange((LONG*)&_ss->iocpCount, 0, 0))
		{
			OnClientLeave(_ss->Index);
			Sbuf *buf = NULL;
			InterlockedDecrement(&clientCounter);
			_ss->sendCount = 0;
			while (1)
			{
				buf = NULL;
				_ss->completeSendQ.pop(buf);
				if (!buf) break;
				buf->Free();
			}
			while (1)
			{
				buf = NULL;
				_ss->sendQ.pop(buf);
				if (!buf) break;
				buf->Free();
			}
			dummyIndex = _ss->Index;
			if (dummyIndex == 0 || getIndex(dummyIndex) == 0)
				CCrashDump::Crash();
			closesocket(_ss->Sock);
			_ss->Sock = INVALID_SOCKET;
			_ss->Index = 0;
			indexStack->push(getIndex(dummyIndex));
		}
	}
}

void IOCPServer::disconnect(SOCKET _sock)
{
	shutdown(_sock, SD_BOTH);
	closesocket(_sock);
}

Session* IOCPServer::acquirLock(unsigned __int64 _Index)
{
	__int64 index = getIndex(_Index);
	Session *ss = &sessionArr[index];
	if (1 == InterlockedIncrement(&(ss->iocpCount)))
	{
		if (0 == InterlockedDecrement(&(ss->iocpCount)))
			disconnect(ss);
		return NULL;
	}

	if (ss->Index != _Index)
	{
		if (0 == InterlockedDecrement(&(ss->iocpCount)))
			disconnect(ss);
		return NULL;
	}

	if (true == ss->disconnectFlag)
	{
		if (0 == InterlockedDecrement(&(ss->iocpCount)))
			disconnect(ss);
		return NULL;
	}

	else
		return &sessionArr[index];
	return NULL;
}

void IOCPServer::releaseLock(Session *_ss)
{
	if (0 == InterlockedDecrement(&(_ss->iocpCount)))
		disconnect(_ss);
}

void IOCPServer::clientShutdown(unsigned __int64 _index)
{
	Session *_ss = acquirLock(_index);
	if (_ss)
	{
		InterlockedExchange((LONG*)&(_ss->disconnectFlag), 1);
		shutdown(_ss->Sock, SD_BOTH);
		releaseLock(_ss);
	}
}