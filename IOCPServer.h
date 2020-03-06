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

		sendFlag = 0;
		disconnectFlag = 0;
		sendCount = 0;
		sendDisconnectFlag = 0;
		sendPQCS = 0;

		iocpCount = 0;
	}
	SOCKET  Sock;
	unsigned __int64 Index;

	// IOCP �۾� ���� ������
	
	LONG sendFlag;				// send ���� �� üũ.
	volatile LONG disconnectFlag;	// disconnect üũ

	LONG sendCount;				// send�� ����ȭ ������ �� 
	LONG sendDisconnectFlag;	// ������ ���� üũ
	LONG sendPQCS;				// PQCS�� �̿��ؼ� SEND �Լ� ȣ��� ȣ�⿩�� üũ

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
	Session *sessionArr;		// session ���� �迭. �����ڿ��� �����Ҵ�
	
	LONG clientCounter;						// ������ �� counter ����
	int maxClient;					// �ִ� ������ ���� 
	int threadCount;				// worker������ ����
	int allThreadCount;		// worker�����带 ������ �� ������ ���� (main ����)
	
	HANDLE hcp;						// IOCP HANDLE
	HANDLE *threadArr;			// Thread ���� �� HANDLE ���� �迭

	tcp_keepalive tcpKeep;

	BYTE Code;
	BYTE Key1, Key2;

private:
	LONG acceptTPS;				// accept ��� ����
	LONG sendTPS;					// send ��� ����
	LONG recvTPS;					// recv ��� ����

private:
	boost::lockfree::stack<unsigned __int64> *indexStack;		// socket �迭 index�������ִ� ������ ����

	static unsigned __stdcall acceptThread(LPVOID _data);			// ACCEPT �۾��� �ϴ� ������
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
	bool	Stop(void);					// ����
	int		GetClientCount(void);	// ����
	void	SendPacket(unsigned __int64 _index, Sbuf *_buf, bool _type = false);

	void clientShutdown(unsigned __int64 _index);		// �ڽ� Ŭ�������� ȣ���ϴ� �Լ� 
	void setEncodeKey(BYTE _code, BYTE _key1, BYTE _key2);

	virtual void OnClientJoin(unsigned __int64 _index) = 0;	// accept -> ����ó�� �Ϸ� �� ȣ��
	virtual void OnClientLeave(unsigned __int64 _index) = 0;		// disconnect �� ȣ��
	virtual bool OnConnectionRequest(char *_ip, unsigned int _port) = 0; // accept �� [false : Ŭ���̾�Ʈ �ź� / true : ���� ���]
	virtual void OnRecv(unsigned __int64 _index, Sbuf *_buf) = 0;		// ���� �Ϸ� ��
	virtual void OnError(int _errorCode, WCHAR *_string) = 0;		// �����޼��� ����

};

