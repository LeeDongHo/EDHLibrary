#include "..\MMORPG\MMORPG_VirtualClient\MMORPG_VirtualClient\stdafx.h"
#include	"stdafx.h"
#include "Virtual_IOCPClient.h"

Session::Session(void)
{
	Sock = INVALID_SOCKET;
	Index = -1;

	recvFlag = false;
	sendFlag = false;
	lockFlag = false;
	disconnectFlag = false;

	sendCount = 0;
	disconnectAfterSend = 0;
	calledPQCSSend = 0;

	sendQ = new lockFreeQueue<Sbuf*>;
}

Session::~Session(void)
{
	Sock = INVALID_SOCKET;
	Index = 0;

	recvFlag = false;
	sendFlag = false;
	lockFlag = false;
	disconnectFlag = false;

	sendCount = 0;
	disconnectAfterSend = 0;
	calledPQCSSend = 0;

	delete sendQ;
}

void Session::setAddr(SOCKADDR_IN _addr)
{
	Addr = _addr;
}

void Session::sessionShutdown()
{
	InterlockedExchange((LONG*)&disconnectFlag, 1);
	shutdown(Sock, SD_BOTH);
}

VirtualClient::VirtualClient(void)
{
	nagleOpt = false;

	configSettingFlag = false;
	clientShutdownFlag = false;

	connectTPS = 0;
	sendTPS = 0;
	recvTPS = 0;

	connectedClientCount = 0;

	totalConnected = 0;

	pConnectTPS = 0;
	pSendTPS = 0;
	pRecvTPS = 0;

	Code = 0x77;
	Key1 = 0x32;
	Key2 = 0x84;
}

unsigned __stdcall VirtualClient::connectThread(LPVOID _data)
{
	VirtualClient *Client = (VirtualClient*)_data;
	
	lockFreeQueue<int>* indexQueue = Client->indexQueue;

	Session *ss = NULL;
	bool clientNagleOpt = Client->nagleOpt;
	bool addrOpt = Client->addrOpt;
	tcp_keepalive keepaliveOpt = Client->keepaliveOpt;
	SOCKADDR_IN  Addr;

	int Index = 0;
	int Count = 0;
	int Error = 0;
	int retVal = 0;

	while (!Client->clientShutdownFlag)
	{
		Count = 0;
		while (Count < Client->CPSCount && indexQueue->getUsedSize() != 0)
		{
			Index = 0;
			indexQueue->dequeue(&Index);
			if (!&Index) break;
			SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
			if (sock == INVALID_SOCKET)
			{
				_SYSLOG(Type::Type_CONSOLE, Level::SYS_ERROR, L"socket function error %d", INVALID_SOCKET);
				break;
			}

			if (clientNagleOpt == true)
			{
				bool optVal = TRUE;
				setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&optVal, sizeof(optVal));
			}

			WSAIoctl(sock, SIO_KEEPALIVE_VALS, (tcp_keepalive*)&keepaliveOpt, sizeof(tcp_keepalive), NULL, 0, 0, NULL, NULL);

			ss = Client->sessionArray[Index];
			ss->Index = Index;

			if (addrOpt)
				Addr = Client->Addr;
			else
				Addr = ss->Addr;
			retVal = connect(sock, (SOCKADDR*)&Addr, sizeof(Addr));
			if (retVal == SOCKET_ERROR)
			{
				Error = WSAGetLastError();
				_SYSLOG(Type::Type_CONSOLE, Level::SYS_DEBUG, L"Connect funtion error :%d", Error);
				ss->Index = -1;
				Client->OnConnectFail(Index);
				indexQueue->enqueue(Index);
				continue;
			}
			Client->connectTPS++;
			Client->totalConnected++;

			InterlockedIncrement(&(Client->connectedClientCount));
			ss->Sock = sock;
			CreateIoCompletionPort((HANDLE)sock, Client->hcp, (ULONG_PTR)ss, 0);
			ss->recvFlag = true;
			Client->recvPost(ss);
			Client->OnConnect(Index);
		}
		Sleep(1000);
	}

	return 0;	
}

unsigned __stdcall VirtualClient::workerThread(LPVOID _data)
{
	VirtualClient *Client = (VirtualClient*)_data;

	int retVal = 1;
	DWORD Trans = 0;
	OVERLAPPED *Over = NULL;
	Session *ss = NULL;

	while (1)
	{
		Trans = 0;
		Over = NULL, ss = NULL;
		retVal = GetQueuedCompletionStatus(Client->hcp, &Trans, (PULONG_PTR)&ss, (LPOVERLAPPED*)&Over, INFINITE);
		if (!Over)
		{
			if (retVal == false)
			{
				_SYSLOG(Type::Type_CONSOLE, Level::SYS_SYSTEM, L"GQCS function error. Overlapped is  NULL and return value return false");
				break;
			}
			if (Trans == 0 && !ss)
				break;
		}
		else
		{
			if ((int)Over == 1)
			{
				Client->sendPost(ss);
				if (ss->sendFlag == false && ss->sendQ->getUsedSize() > 0)
					Client->sendPost(ss);
				InterlockedDecrement((LONG*)&ss->calledPQCSSend);
			}
			if (&(ss->recvOver) == Over)
				Client->completedRecv(Trans, ss);
			if (&(ss->sendOver) == Over)
				Client->completedSend(Trans, ss);

			if (ss->recvFlag == false)
			{
				if (ss->sendFlag == false && ss->lockFlag == false && ss->calledPQCSSend == false)
					Client->disconnect(ss);
			}
		}
	}

	return 0;
}

void VirtualClient::recvPost(Session *_ss)
{
	DWORD recvVal, Flag;
	int retVal, Error;

	winBuffer* recvQ = &_ss->recvQ;
	ZeroMemory(&(_ss->recvOver), sizeof(_ss->recvOver));
	
	WSABUF wBuf[2];
	ZeroMemory(&wBuf, sizeof(WSABUF) * 2);
	wBuf[0].buf = recvQ->getRearPosPtr();
	wBuf[0].len = recvQ->getNotBrokenFreeSize();

	if (recvQ->getFreeSize() == 0)
	{
		disconnect(_ss);
		_SYSLOG(Type::Type_CONSOLE, Level::SYS_SYSTEM, L"[Index : %d] Receive Queue's memory is full!",_ss->Index);
		return;
	}

	if (recvQ->getFreeSize() > recvQ->getNotBrokenFreeSize())
	{
		wBuf[1].buf = recvQ->getBufferPtr();
		wBuf[1].len = (recvQ->getFreeSize() - recvQ->getNotBrokenFreeSize());
	}

	recvVal = 0, Flag = 0;
	retVal = WSARecv(_ss->Sock, wBuf, 2, &recvVal, &Flag, &(_ss->recvOver), NULL);
	if (retVal == SOCKET_ERROR)
	{
		Error = WSAGetLastError();
		if (Error != WSA_IO_PENDING)
		{
			_ss->recvFlag = false;
			if (Error != WSAECONNRESET && Error != WSAESHUTDOWN && Error != WSAECONNABORTED)
				_SYSLOG(Type::Type_CONSOLE, Level::SYS_ERROR, L"[Index : %d] WSARecv funtion error! Error code : %d",_ss->Index, Error);
			sessionShutdown(_ss);
		}
	}
}

void VirtualClient::sendPost(Session *_ss)
{
	DWORD sendVal = 0;

	int Count = 0, retVal = 0, Size = 0, Error = 0;

	WSABUF wBuf[Virtual_maxWSABUF];
	wBuf[Count].buf = 0;
	wBuf[Count].len = 0;
	
	Size = _ss->sendQ->getUsedSize();
	if (Size <= 0)
		return;

	lockFreeQueue<Sbuf*> *sendQ = _ss->sendQ;
	Sbuf *buf = NULL;

	if (0 == InterlockedCompareExchange((LONG*)&(_ss->sendFlag), 1, 0))
	{
		_ss->sendCount = 0;
		Count = 0;

		ZeroMemory(&_ss->sendOver, sizeof(_ss->sendOver));
		
		do
		{
			for (Count; Count < Virtual_maxWSABUF; Count++)
			{
				buf = NULL;
				retVal = sendQ->peek(&buf, Count);
				if (retVal == -1 || !buf)
					break;
				wBuf[Count].buf = buf->getHeaderPtr();
				wBuf[Count].len = buf->getPacketSize();
			}
			if (Count >= Virtual_maxWSABUF)
				break;
		} while (sendQ->getUsedSize() > Count);

		_ss->sendCount = Count;
		if (Count == 0)
		{
			InterlockedExchange((LONG*)&(_ss->sendFlag), 0);
			return;
		}

		retVal = WSASend(_ss->Sock, wBuf, Count, &sendVal, 0, &_ss->sendOver, NULL);
		if (retVal == SOCKET_ERROR)
		{
			Error = WSAGetLastError();
			if (Error != WSA_IO_PENDING)
			{
				if (Error != WSAECONNRESET && Error != WSAESHUTDOWN && Error != WSAECONNABORTED)
					_SYSLOG(Type::Type_CONSOLE, Level::SYS_ERROR, L"[Index : %d] WSASend function error. Error code : %d", _ss->Index, Error);
				_ss->sendCount = 0;
				sessionShutdown(_ss);
				InterlockedExchange((LONG*)&(_ss->sendFlag), 0);
			}
		}
	}
}

void VirtualClient::completedRecv(LONG _trans, Session *_ss)
{
	int usedSize = 0, retVal = 0;

	if (_trans == 0)
	{
		sessionShutdown(_ss);
		_ss->recvFlag = false;
		return;
	}
	winBuffer *recvQ = &(_ss->recvQ);
	recvQ->moveRearPos(_trans);
	while (1)
	{
		usedSize = recvQ->getUsedSize();
		netHeader head;
		retVal = recvQ->peek((char*)&head, sizeof(netHeader));
		if (retVal == 0 || (usedSize - sizeof(netHeader)) < head.len)
			break;
		InterlockedIncrement(&recvTPS);
		
		try
		{
			Sbuf *buf = Sbuf::Alloc();
			retVal = recvQ->dequeue(buf->getBufPtr(), sizeof(netHeader) + head.len);
			buf->moveRearPos(head.len);
			if (buf->Decode(Code, Key1, Key2))
				OnRecv(_ss->Index, buf);
			else
				throw 4900;
			buf->Free();
		}
		catch(int num)
		{
			if (num != 4900)
				_SYSLOG(Type::Type_CONSOLE, Level::SYS_ERROR, L"Read error. error code : %d", num);
			sessionShutdown(_ss);
			return;
		}

	}
	recvPost(_ss);
}

void VirtualClient::completedSend(LONG _trans, Session *_ss)
{
	if (_trans == 0)
	{
		InterlockedDecrement((LONG*)&(_ss->sendFlag));
		sessionShutdown(_ss);
		return;
	}

	Sbuf *buf = NULL;
	lockFreeQueue<Sbuf*> *sendQ = _ss->sendQ;
	int Count = 0;
	for (Count; Count < _ss->sendCount; Count++)
	{
		buf = NULL;
		sendQ->dequeue(&buf);
		if (!buf)	break;
		buf->Free();
		InterlockedIncrement(&sendTPS);
	}
	InterlockedDecrement((LONG*)&(_ss->sendFlag));

	if (sendQ->getUsedSize() == 0)
	{
		if (1 == InterlockedCompareExchange((LONG*)&(_ss->disconnectAfterSend), 1, 1))
			sessionShutdown(_ss);
	}
//	sendPost(_ss);					// Virtual version에서는 응답이 오지 않으면 보내지 않습니다. 
}

void VirtualClient::sessionShutdown(Session *_ss)
{
	InterlockedExchange((LONG*)&(_ss->disconnectFlag), 1);
	shutdown(_ss->Sock, SD_BOTH);
}

void VirtualClient::disconnect(Session *_ss)
{
	unsigned int dummyIndex = 0;
	if (_ss->disconnectFlag == true)
	{
		if (1 == InterlockedCompareExchange((LONG*)&(_ss->disconnectFlag), 1, 1))
		{
			OnDisconnect(_ss->Index);
			Sbuf *buf = NULL;
			while (1)
			{
				buf = NULL;
				_ss->sendQ->dequeue(&buf);
				if (!buf)	break;
				buf->Free();
			}
			closesocket(_ss->Sock);
			InterlockedDecrement(&connectedClientCount);
			_ss->Sock = INVALID_SOCKET;
			dummyIndex = _ss->Index;
			_ss->Index = -1;
			indexQueue->enqueue(dummyIndex);
		}
	}
}

void VirtualClient::disconnect(SOCKET _sock)
{
	shutdown(_sock, SD_BOTH);
	closesocket(_sock);
}

Session* VirtualClient::acquireLockedSession(int _index)
{
	Session *ss = sessionArray[_index];

	if (ss->Index != _index)
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

	if (1 == InterlockedCompareExchange((LONG*)&(ss->lockFlag), 0, 1))
	{
		if (true == ss->disconnectFlag)
			disconnect(ss);
		return NULL;
	}

	return ss;
}

void VirtualClient::releaseLockedSession(Session *_ss)
{
	InterlockedExchange((LONG*)&(_ss->lockFlag), 0);
	if (true == _ss->disconnectFlag)
		disconnect(_ss);
}

void VirtualClient::checkAllClientDisconnect(void)
{
	while (1)
	{
		if (connectedClientCount == 0)
			break;
		Sleep(100);
	}
}
void VirtualClient::setTPS(void)
{
	pConnectTPS = connectTPS;
	pSendTPS = sendTPS;
	pRecvTPS = recvTPS;

	connectTPS = 0;
	sendTPS = 0;
	recvTPS = 0;
}

void VirtualClient::setCPS(unsigned int _CPS)
{
	CPSCount = _CPS;
}

void VirtualClient::setAccessFlag(bool _val)
{
	arrayAccessFlag = _val;
}

bool VirtualClient::setConfigData(bool _arrayAccessFlag, unsigned int _maxClientCount, unsigned int _workerThreadCount, bool _nagleOpt, char* _ip, unsigned short _port)
{
	maxClient = _maxClientCount;
	workerThreadCount = _workerThreadCount;
	nagleOpt = _nagleOpt;
	arrayAccessFlag = _arrayAccessFlag;

	if (_ip)
	{
		ZeroMemory(&Addr, sizeof(Addr));

		Addr.sin_family = AF_INET;
		Addr.sin_addr.s_addr = inet_addr(_ip);
		Addr.sin_port = htons(_port);
		addrOpt = true;
	}
	else
		Addr = { 0 };

	sessionArray = (Session**)malloc(sizeof(Session) * maxClient);
	if (arrayAccessFlag)
	{
		int Count = 0;
		for (Count; Count < maxClient; Count++)
			sessionArray[Count] = new Session();
	}
	configSettingFlag = true;

	return true;
}

void VirtualClient::setEncodeKey(BYTE _code, BYTE _key1, BYTE _key2)
{
	Code = _code;	
	Key1 = _key1;
	Key2 = _key2;
}

bool VirtualClient::setArrayAccessPTR(Session *_ss, int _count)
{
	if (!configSettingFlag || arrayAccessFlag)
		return false;

	sessionArray[_count] = _ss;

	return true;
}

bool VirtualClient::Start(void)
{
	if (configSettingFlag == false || arrayAccessFlag == false)
		return false;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		_SYSLOG(Type::Type_CONSOLE, Level::SYS_ERROR, L"WSAStartup function error.");
		return false;
	}

	hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (!hcp)
	{
		_SYSLOG(Type::Type_CONSOLE, Level::SYS_ERROR, L"CreateIoCompletionPort function error.");
		return false;
	}

	// Keep Alive option setting
	keepaliveOpt.onoff = 1;
	keepaliveOpt.keepalivetime = 5500;			// 5.5 sec
	keepaliveOpt.keepaliveinterval = 1000;	// 1 sec

	indexQueue = new lockFreeQueue<int>;
	workerThreadArray = new HANDLE[workerThreadCount];

	for (unsigned int i = 0; i < maxClient; i++)
		indexQueue->enqueue(i);

	// 스레드 생성
	HANDLE hThread;

	hThread = (HANDLE)_beginthreadex(NULL, 0, connectThread, (LPVOID)this, 0, 0);
	CloseHandle(hThread);

	for (int i = 0; i < workerThreadCount; i++)
		workerThreadArray[i] = (HANDLE)_beginthreadex(NULL, 0, workerThread, (LPVOID)this, 0, 0);

	return true;
}

bool VirtualClient::Stop(void)
{
	clientShutdownFlag = true;
	printf("Session disconnecting. please wait...\t");

	int Count = 0;
	for (Count = 0; Count < maxClient; Count++)
	{
		if (sessionArray[Count]->Sock != INVALID_SOCKET)
			sessionShutdown(sessionArray[Count]);
	}
	checkAllClientDisconnect();
	printf("Success!\n\n");

	printf("Worker thread deleting. please wait... \t");
	for (Count = 0; Count < workerThreadCount; Count++)
		PostQueuedCompletionStatus(hcp, 0, 0, 0);
	WaitForMultipleObjects(workerThreadCount, workerThreadArray, TRUE, INFINITE);
	printf("Success!\n\n");

	CloseHandle(hcp);
	delete[] sessionArray;
	delete[] workerThreadArray;
	delete indexQueue;
	WSACleanup();

	return true;
}

void VirtualClient::sendPacket(int _index, Sbuf *_buf, bool _disconnectAfterSend)
{
	Session *ss = acquireLockedSession(_index);
	if (!ss)
		return;

	int retVal;
	_buf->Encode(Code, Key1, Key2);
	_buf->addRef();
	ss->sendQ->enqueue(_buf);

	if (_disconnectAfterSend == true)
	{
		InterlockedCompareExchange((LONG*)&ss->disconnectAfterSend, true, false);
		if (ss->sendQ->getUsedSize() == 0)
			sessionShutdown(ss);
	}

	if (ss->sendFlag == false)
	{
		if (ss->calledPQCSSend == 0)
		{
			if (0 == InterlockedCompareExchange((LONG*)&(ss->calledPQCSSend), 1, 0))
				PostQueuedCompletionStatus(hcp, 0, (ULONG_PTR)ss, (LPOVERLAPPED)1);
		}
	}

	releaseLockedSession(ss);
}

void VirtualClient::clientShutdown(int _index)
{
	shutdown(sessionArray[_index]->Sock, SD_BOTH);
}