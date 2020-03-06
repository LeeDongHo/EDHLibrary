#pragma pack(push,1)
struct netHeader
{
	BYTE code;					// 검증 코드
	WORD len;					// 패킷의 길이 (헤더 제외)
	BYTE randCode;		// XOR code
	BYTE checkSum;		// checkSum
};
#pragma pack(pop)

struct Session
{
	Session()
	{
		Sock = INVALID_SOCKET;
		Index = 0;

		sendFlag = 0;
		disconnectFlag = 0;
		sendCount = 0;
		sendDisconnectFlag = 0;
		sendPQCS = 0;

		iocpCount = 0;
	}
	SOCKET  Sock;
	unsigned __int64 Index;

	// IOCP 작업 관련 변수들
	
	LONG sendFlag;				// send 중인 지 체크.
	volatile LONG disconnectFlag;	// disconnect 체크

	LONG sendCount;				// send한 직렬화 버퍼의 수 
	LONG sendDisconnectFlag;	// 보내고 끊기 체크
	LONG sendPQCS;				// PQCS를 이용해서 SEND 함수 호출시 호출여부 체크

	volatile LONG iocpCount;

	winBuffer recvQ;
	boost::lockfree::queue<Sbuf*> sendQ;
	boost::lockfree::queue<Sbuf*> completeSendQ;
	OVERLAPPED recvOver, sendOver;
};


class IOCPServer
{
private:
	SOCKET listenSock;
	Session *sessionArr;		// session 관리 배열. 생성자에서 동적할당
	
	LONG clientCounter;						// 접속자 수 counter 변수
	int maxClient;					// 최대 접속자 제한 
	int threadCount;				// worker스레드 개수
	int allThreadCount;		// worker스레드를 포함한 총 스레드 개수 (main 제외)
	
	HANDLE hcp;						// IOCP HANDLE
	HANDLE *threadArr;			// Thread 생성 후 HANDLE 보관 배열

	tcp_keepalive tcpKeep;

	BYTE Code;
	BYTE Key1, Key2;

private:
	LONG acceptTPS;				// accept 기록 변수
	LONG sendTPS;					// send 기록 변수
	LONG recvTPS;					// recv 기록 변수

private:
	boost::lockfree::stack<unsigned __int64> *indexStack;		// socket 배열 index관리해주는 락프리 스택

	static unsigned __stdcall acceptThread(LPVOID _data);			// ACCEPT 작업만 하는 스레드
	static unsigned __stdcall workerThread(LPVOID _data);		// IOCP 스레드

	Session* insertSession(SOCKET _sock);


private:
	void recvPost(Session *_ss);
	void sendPost(Session *_ss);
	void completeRecv(LONG _trnas, Session *_ss);
	void completeSend(LONG _trnas, Session *_ss);

	void clientShutdown(Session *_ss);
	void disconnect(Session *_ss);
	void disconnect(SOCKET _sock);

	Session* acquirLock(unsigned __int64 _index);
	void releaseLock(Session *_ss);

protected:
	bool quit;
	int acceptTotal;
	int pacceptTPS;
	int psendTPS;
	int precvTPS;

protected:
	int getAcceptTotal(void);
	int getAcceptTPS(void);
	int getSendTPS(void);
	int getRecvTPS(void);
	void setTPS(void);

protected:
	bool	Start(char *_ip, unsigned short _port, unsigned short _threadCount, bool _nagle, unsigned int _maxClient);
	bool	Stop(void);					// 미정
	int		GetClientCount(void);	// 미정
	void	SendPacket(unsigned __int64 _index, Sbuf *_buf, bool _type = false);

	void clientShutdown(unsigned __int64 _index);		// 자식 클래스에서 호출하는 함수 
	void setEncodeKey(BYTE _code, BYTE _key1, BYTE _key2);

	virtual void OnClientJoin(unsigned __int64 _index) = 0;	// accept -> 접속처리 완료 후 호출
	virtual void OnClientLeave(unsigned __int64 _index) = 0;		// disconnect 후 호출
	virtual bool OnConnectionRequest(char *_ip, unsigned int _port) = 0; // accept 후 [false : 클라이언트 거부 / true : 접속 허용]
	virtual void OnRecv(unsigned __int64 _index, Sbuf *_buf) = 0;		// 수신 완료 후
	virtual void OnError(int _errorCode, WCHAR *_string) = 0;		// 오류메세지 전송

};

