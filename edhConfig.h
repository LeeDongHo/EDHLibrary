#ifndef EDH
#define EDH

#include <atlsocket.h>
#include <string.h>

#define maxWSABUF 100


#include "rapidjson\document.h"
#include "rapidjson\writer.h"
#include "rapidjson\stringbuffer.h"

// 서버 공통 구조체 (IOCPServer / LAN_IOCPServer)

namespace EDH
{
	bool domainToIP(WCHAR *_ip, IN_ADDR *_addr)
	{
		ADDRINFOW *addrInfo;
		SOCKADDR_IN *sockAddr;
		if (GetAddrInfo(_ip, L"0", NULL, &addrInfo) != 0)
		{
			return false;
		}
		sockAddr = (SOCKADDR_IN*)addrInfo->ai_addr;
		*_addr = sockAddr->sin_addr;
		FreeAddrInfo(addrInfo);

		return true;
	}
}

#endif
