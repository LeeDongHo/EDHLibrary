#ifndef LANSERVER
#define LANSERVER

struct lanSession
{
	lanSession()
	{
		Sock = INVALID_SOCKET;
		Index = 0;

		sendFlag = 0;
		disconnectFlag = 0;
		sendCount = 0;
	}

	SOCKET  Sock;
	unsigned __int64 Index;

	// IOCP �۾� ���� ������

	LONG sendFlag;				// send ���� �� üũ.
	LONG afterSendingDisconnect;
	LONG disconnectFlag;		// disconnect üũ

	LONG sendCount;						// ������ ��Ŷ�� ����
	LONG iocpCount;
	
	winBuffer recvQ;
	lockFreeQueue<Sbuf*> sendQ;
	OVERLAPPED recvOver, sendOver;
};

class LanServer
{
private:
	SOCKET listenSock;
	lanSession *sessionArry;		// session ���� �迭. �����ڿ��� �����Ҵ�
	volatile LONG LANclientCounter;		// ������ �� counter ����
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
	bool	Stop(void);					// ����
	int		GetClientCount(void);	// ����
	void	SendPacket(unsigned __int64 _id, Sbuf *_buf);

	void clientShutdown(unsigned __int64 _id);

	virtual void onClientJoin(unsigned __int64 _id) = 0;	// accept -> ����ó�� �Ϸ� �� ȣ��
	virtual void onClientLeave(unsigned __int64 _id) = 0;		// disconnect �� ȣ��
	virtual void onRecv(unsigned __int64 _id, Sbuf *_buf) = 0;		// ���� �Ϸ� ��
	virtual void onSend(unsigned __int64 _id, int _sendSize) = 0;	// �۽� �Ϸ� ��

	virtual void onError(int _errorCode, WCHAR *_string) = 0;		// �����޼��� ����

public:

	int getAcceptTotal(void);
	int getAcceptTPS(void);
	int getSendTPS(void);
	int getRecvTPS(void);
	void setTPS(void);
};

#endif // !LANSERVER



