#ifndef LANSERVER
#define LANSERVER

struct lanSession
{
	lanSession()
	{
		Sock = INVALID_SOCKET;
		Index = 0;

		recvFlag = 0;
		sendFlag = 0;
		usingFlag = 0;
		disconnectFlag = 0;
		sendCount = 0;
	}

	SOCKET  Sock;
	unsigned __int64 Index;

	// IOCP 작업 관련 변수들

	LONG recvFlag;				// Recv 중인지 체크. 
	LONG sendFlag;				// send 중인 지 체크.
	LONG usingFlag;				// 어떤 스레드에서 세션을 사용하고 있는지 체크.
	LONG afterSendingDisconnect;
	LONG disconnectFlag;		// disconnect 체크

	LONG sendCount;						// 전송한 패킷의 갯수
	LONG iocpCount;
	
	winBuffer recvQ;
	lockFreeQueue<Sbuf*> sendQ;
	OVERLAPPED recvOver, sendOver;
};

class LanServer
{
private:
	SOCKET listenSock;
	lanSession *sessionArry;		// session 관리 배열. 생성자에서 동적할당
	volatile LONG LANclientCounter;		// 접속자 수 counter 변수
	unsigned int maxCounter;
	unsigned int workerCount;
	HANDLE hcp;	// IOCP HANDLE
	SRWLOCK acceptLock;
	HANDLE *threadArr;

private:
	volatile LONG acceptTPS;
	volatile LONG sendTPS;
	volatile LONG recvTPS;

private:
	lockFreeStack<__int64> *indexStack;

	static unsigned __stdcall acceptThread(LPVOID _data);
	static unsigned __stdcall workerThread(LPVOID _data);
	static unsigned __stdcall tpsThread(LPVOID _data);

	lanSession* insertSession(SOCKET _sock);


private:
	void recvPost(lanSession *_ss);
	void sendPost(lanSession *_ss);
	void completeRecv(LONG _trnas, lanSession *_ss);
	void completeSend(LONG _trnas, lanSession *_ss);

	void clientShutdown(lanSession *_ss);
	void disconnect(lanSession *_ss);
	void disconnect(SOCKET _sock);

	lanSession* acquirLock(unsigned __int64 _id);
	void releaseLock(lanSession *_ss);

protected:
	bool quit;
	int acceptTotal;
	int pacceptTPS;
	int psendTPS;
	int precvTPS;



protected:
	bool	Start(char *_ip, unsigned short _port, unsigned short _threadCount, bool _nagle, unsigned int _maxSession);
	bool	Stop(void);					// 미정
	int		GetClientCount(void);	// 미정
	void	SendPacket(unsigned __int64 _id, Sbuf *_buf);

	void clientShutdown(unsigned __int64 _id);

	virtual void OnClientJoin(unsigned __int64 _id) = 0;	// accept -> 접속처리 완료 후 호출
	virtual void OnClientLeave(unsigned __int64 _id) = 0;		// disconnect 후 호출
	virtual void OnRecv(unsigned __int64 _id, Sbuf *_buf) = 0;		// 수신 완료 후
	virtual void OnSend(unsigned __int64 _id, int _sendSize) = 0;	// 송신 완료 후

	virtual void OnError(int _errorCode, WCHAR *_string) = 0;		// 오류메세지 전송

public:

	int getAcceptTotal(void);
	int getAcceptTPS(void);
	int getSendTPS(void);
	int getRecvTPS(void);
	void setTPS(void);
};

#endif // !LANSERVER



