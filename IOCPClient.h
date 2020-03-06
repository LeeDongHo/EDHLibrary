#pragma pack(push,1)
struct netHeader
{
	BYTE code;					// ���� �ڵ�
	WORD len;					// ��Ŷ�� ���� (��� ����)
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

		recvFlag = 0;
		sendFlag = 0;
		usingFlag = 0;
		disconnectFlag = 0;
		sendCount = 0;
		sendDisconnectFlag = 0;
		sendPQCS = 0;

	}
	SOCKET  Sock;
	unsigned __int64 Index;

	// IOCP �۾� ���� ������
	
	volatile LONG recvFlag;				// Recv ������ üũ. 
	volatile LONG sendFlag;				// send ���� �� üũ.
	volatile LONG usingFlag;				// � �����忡�� ������ ����ϰ� �ִ��� üũ.
	volatile LONG disconnectFlag;	// disconnect üũ

	LONG sendCount;				// send�� ����ȭ ������ �� 
	volatile LONG sendDisconnectFlag;	// ������ ���� üũ
	volatile LONG sendPQCS;				// PQCS�� �̿��ؼ� SEND �Լ� ȣ��� ȣ�⿩�� üũ

	winBuffer recvQ;
	lockFreeQueue<Sbuf*> sendQ;
	OVERLAPPED recvOver, sendOver;
};


class IOCPClient
{
private:
	Session *sessionArr;		// session ���� �迭. �����ڿ��� �����Ҵ�
	
	LONG clientCounter;						// ������ �� counter ����
	int maxClient;					// �ִ� ������ ���� 
	int threadCount;				// worker������ ����
	
	HANDLE hcp;						// IOCP HANDLE
	HANDLE *threadArr;
	HANDLE handleArr[2];		// connect thread waitformultiple objects's variable
	SOCKADDR_IN addr;

	tcp_keepalive tcpKeep;

	bool nagleOpt;

private:
	LONG connectTPS;				// accept ��� ����
	LONG sendTPS;					// send ��� ����
	LONG recvTPS;					// recv ��� ����

private:
	lockFreeStack<unsigned __int64> *indexStack;		// socket �迭 index�������ִ� ������ ����
	lockFreeStack<unsigned __int64> *connectStack;
	
	static unsigned __stdcall connectThread(LPVOID _data);			// ACCEPT �۾��� �ϴ� ������
	static unsigned __stdcall workerThread(LPVOID _data);		// IOCP ������

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
	bool ctQuit;
	int connectTotal;
	int pconnectTPS;
	int psendTPS;
	int precvTPS;

protected:
	int getConnectTotal(void);
	int getConnetTPS(void);
	int getSendTPS(void);
	int getRecvTPS(void);
	void setTPS(void);

protected:
	bool	Start(char *_ip, unsigned short _port, unsigned short _threadCount, bool _nagle, unsigned int _maxClient);
	bool	Stop(void);					// ����
	int		GetClientCount(void);	// ����
	void	SendPacket(unsigned __int64 _index, Sbuf *_buf, bool _type = false);

	void clientShutdown(unsigned __int64 _Index);		// �ڽ� Ŭ�������� ȣ���ϴ� �Լ� 

	virtual void OnClientJoin(unsigned __int64 _Index) = 0;	// accept -> ����ó�� �Ϸ� �� ȣ��
	virtual void OnClientLeave(unsigned __int64 _Index) = 0;		// disconnect �� ȣ��
	virtual bool OnConnectionRequest(char *_ip, unsigned int _port) = 0; // accept �� [false : Ŭ���̾�Ʈ �ź� / true : ���� ���]
	virtual void OnRecv(unsigned __int64 _Index, Sbuf *_buf) = 0;		// ���� �Ϸ� ��

	virtual void OnError(int _errorCode, WCHAR *_string) = 0;		// �����޼��� ����

};

