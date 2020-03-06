#pragma once

#define Virtual_maxWSABUF 100

struct netHeader
{
	BYTE code;					// 검증 코드
	WORD len;					// 패킷의 길이 (헤더 제외)
	BYTE randCode;		// XOR code
	BYTE checkSum;		// checkSum
};

class Session
{
	friend class VirtualClient;

private:
	SOCKET Sock;

	SOCKADDR_IN Addr;

	LONG recvFlag;
	LONG sendFlag;
	LONG lockFlag;
	LONG disconnectFlag;

	LONG sendCount;
	LONG disconnectAfterSend;
	LONG calledPQCSSend;

	winBuffer recvQ;
	lockFreeQueue<Sbuf*> *sendQ;

	OVERLAPPED recvOver, sendOver;

protected:
	int Index;

public:
	Session(void);
	~Session(void);

	void setAddr(SOCKADDR_IN _addr);
	void sessionShutdown(void);
};

class virtualIOCP
{
private:
	bool nagleOpt;
	bool addrOpt;
	tcp_keepalive keepaliveOpt;

	BYTE Code, Key1, Key2;
	SOCKADDR_IN Addr;

	unsigned int maxClient;
	unsigned int workerThreadCount;
	
	HANDLE hcp;
	HANDLE *workerThreadArray;
	
	bool configSettingFlag;
	bool clientShutdownFlag;
	bool arrayAccessFlag;

	unsigned int connectTPS;
	unsigned int sendTPS;
	unsigned int recvTPS;

	lockFreeQueue<int> *indexQueue;
	Session** sessionArray;

protected:
	unsigned int connectedClientCount;

	unsigned int totalConnected;

	unsigned int pConnectTPS;
	unsigned int pSendTPS;
	unsigned int pRecvTPS;

	unsigned int CPSCount;		// Connect Per Seconds

private:

	static unsigned __stdcall connectThread(LPVOID _data);
	static unsigned __stdcall workerThread(LPVOID _data);

	void recvPost(Session *_ss);
	void sendPost(Session *_ss);
	void completedRecv(LONG _trans, Session *_ss);
	void completedSend(LONG _trans, Session *_ss);		

	void sessionShutdown(Session *_ss);			// 세션과 서버의 연결을 끊음
	void disconnect(Session *_ss);						// 연결 끊긴 세션 후 처리
	void disconnect(SOCKET _sock);					// 소켓 연결 끊기 (세션 할당 전)

	Session* acquireLockedSession(int _index);		// 세션에 락을 걸고, 세션 포인터 반환
	void releaseLockedSession(Session *_ss);				// 세션의 락을 해제

	void checkAllClientDisconnect(void);

protected:
	void setTPS(void);		// 1초마다 모니터링 변수 갱신 (recvTPS -> pRecvTPS)

public:
	VirtualClient(void);
	~VirtualClient(void) {};

	bool setConfigData(bool _arrayAccessFlag ,unsigned int _maxClientCount, unsigned int _workerThreadCount, bool _nagleOpt = false, char *_ip = NULL, unsigned short _port = 0);
	void setEncodeKey(BYTE _code, BYTE _key1, BYTE _key2);
	bool setArrayAccessPTR(Session *_ss, int _count);
	void setCPS(unsigned int _CPS);
	void setAccessFlag(bool _val);

	bool Start(void);
	bool Stop(void);

	void sendPacket(int _index, Sbuf *_buf, bool disconnectAfterSend = false);
	void clientShutdown(int _index);

	virtual void printSettingData()=0;

	virtual void OnConnect(int _index)=0;
	virtual void OnConnectFail(int _index) = 0;
	virtual void OnDisconnect(int _index)=0;
	virtual void OnRecv(int _index, Sbuf *_buf)=0;
	virtual void OnSend()=0;
};