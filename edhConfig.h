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
	#define setID(index, clientID) (index << 48) | clientID
	#define getID(clientID) (clientID<<16) >>16
	#define getIndex(clientID) clientID>>48

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

	char* loadFile(const char *_fileName)
	{
		FILE *fp;
		printf("Loding [%s]..\t", _fileName);
		fopen_s(&fp, _fileName, "rb");
		if (!fp)
		{
			printf("fail\n");
			printf("파일 이름 오류 [%s]\n", _fileName);
			fclose(fp);
			return NULL;
		}

		fseek(fp, 0, SEEK_END);
		int fileSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		char *buffer = new char[(fileSize + 1)];
		size_t test = fread(buffer, fileSize, 1, fp);
		if (test == 0)
		{
			printf("fail\n");
			printf("파일 읽기 오류 [%s]\n", _fileName);
			delete[] buffer;
			fclose(fp);
			return NULL;
		}
		printf("complete\n");
		buffer[fileSize] = '\0';
		fclose(fp);
		return buffer;
	}

}

#endif
