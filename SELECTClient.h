#pragma once

struct client
{
	SOCKET sock;

};

class SELECTClient
{
private:
	SOCKET sock;
	SOCKADDR_IN addr;

	client user;

	unsigned short allThreadCount;
	bool nagleOpt;

	char ip[16];
	unsigned short port;

	LONG sendTPS;
	LONG recvTPS;

public:
	LONG pSendTPS;
	LONG pRecvTPS;

private:
	static unsigned __stdcall connectThread(LPVOID _Data);
	static unsigned __stdcall workerThread(LPVOID _Data);

public:
	bool	Start(char *_Ip, unsigned short _Port, unsigned short _threadCount, bool _Nalge);
	bool	Stop();

	LONG getSendTPS();
	LONG getRecvTPS();

	void sendPost(Sbuf *_buf);
	void serverdisconnect(unsigned int _Index);

	virtual void onClientJoin(void) = 0;
	virtual void onClientLeave(void) = 0;
	virtual void onRecv(Sbuf *_buf);
	

};