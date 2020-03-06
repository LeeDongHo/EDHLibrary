#include "stdafx.h"

systemLog* systemLog::log = NULL;
//gameLog* gameLog::log = NULL;
//MYSQL* gameLog::connection = NULL;



systemLog::systemLog()
{
	log = NULL;
	type = Type_FILE;
	level = SYS_WARNING;
	folder = L"log";
	logCounter = 0;
}

void systemLog::setSys(Type _Type, Level _level)
{
	type = _Type;
	level = _level;

}

void systemLog::sysLog(Type _type, Level _level, WCHAR *_string, ...)
{
	systemLog* log = getInstance();
	if (_level > log->level)
		return;
	WCHAR parameter[512];
	WCHAR dummy[256];

	va_list vs;
	va_start(vs, _string);
	HRESULT result = StringCchVPrintf(dummy, 256, _string, vs);
	if (result != S_OK)
	{
		wprintf(L"VA_LIST ERROR [%s / %d] : %s\n", _type, _level, _string);
		return;
	}
	va_end(vs);

	wsprintf(parameter, L"[%d / %d / %06d] : %s\n", _type, _level,log->logCounter, dummy);
	log->saveLog(_type, parameter);
	log->logCounter++;
	return;
}

void systemLog::debugLog(WCHAR *_Type, WCHAR *_functionName, WCHAR *_String, ...)
{
	systemLog* log = getInstance();
	
	if (!_Type || !_functionName) return;

	WCHAR parameter[512];
	WCHAR dummy[256];

	va_list vs;
	va_start(vs, _String);
	HRESULT result = StringCchVPrintf(dummy, 256, _String, vs);
	if (result != S_OK)
	{
		wprintf(L"VA_LIST ERROR [%s / %s] : %s\n", _Type, _functionName, _String);
		return;
	}
	va_end(vs);

	wsprintf(parameter, L"%s] %s\n", _functionName, dummy);
	log->saveLog(_Type, parameter);
	log->logCounter++;
	return;
}

void systemLog::saveLog(Type _type, WCHAR *_string)
{
	int err;
	_wmkdir(folder);
	WCHAR fileName[256] = L"";
	char dummy[512];
	tm asdf;
	__time64_t time;
	_time64(&time);
	localtime_s(&asdf, &time);
	wsprintf(fileName, L"%s\\[%d]%04d%02d.txt", folder, _type, asdf.tm_year + 1900, asdf.tm_mon + 1);

	FILE *fp;
	fileOpen:
	_wfopen_s(&fp, fileName, L"a");
	if (!fp)
	{
		err = GetLastError();
		if (err == 32) goto fileOpen;
		printf("FILE_OPEN ERROR : %d\n", err);
		fclose(fp);
		logCounter--;
		return;
	}
	WideCharToMultiByte(CP_UTF8, 0, _string, 512, dummy, 512, 0, 0);
	//size_t result = fwrite(_string, 512, 512, fp);
	size_t result = fprintf(fp, "%s", dummy);
	if (result == 0)
	{
		err = GetLastError();
		printf("FILE_WRITE ERROR : %d\n", err);
		fclose(fp);
		logCounter--;
		return;
	}
	fclose(fp);
	return;
}

void systemLog::saveLog(WCHAR *_type, WCHAR *_string)
{
	int err;
	_wmkdir(folder);
	WCHAR fileName[256] = L"";
	char dummy[512];
	tm asdf;
	__time64_t time;
	_time64(&time);
	localtime_s(&asdf, &time);
	wsprintf(fileName, L"%s\\[%s]%04d%02d.txt", folder, _type, asdf.tm_year + 1900, asdf.tm_mon + 1);

	FILE *fp;
fileOpen:
	_wfopen_s(&fp, fileName, L"a");
	if (!fp)
	{
		err = GetLastError();
		if (err == 32) goto fileOpen;
		printf("FILE_OPEN ERROR : %d\n", err);
		fclose(fp);
		logCounter--;
		return;
	}
	WideCharToMultiByte(CP_UTF8, 0, _string, 512, dummy, 512, 0, 0);
	//size_t result = fwrite(_string, 512, 512, fp);
	size_t result = fprintf(fp, "%s", dummy);
	if (result == 0)
	{
		err = GetLastError();
		printf("FILE_WRITE ERROR : %d\n", err);
		fclose(fp);
		logCounter--;
		return;
	}
	fclose(fp);
	return;
}

void systemLog::hexSave(Type _type, Level _level, BYTE *_pointer, BYTE _len)
{
	if (_level > level)
		return;
	WCHAR dummy[256];
	WCHAR dummy1[10];
	wsprintf(dummy, L"[%s / %d / %06d] : ");

	for (int i = 0; i < _len; i++)
	{
		wsprintf(dummy1, L"%x", _pointer);
		wcscat_s(dummy, dummy1);
		_pointer++;
	}
	wcscat_s(dummy, L"\n");
	saveLog(_type, dummy);
}

void systemLog::sessionSave(Type _type, Level _level, BYTE *_session)
{
	if (_level > level)
		return;
	WCHAR dummy[256];
	WCHAR dummy1[10];
	wsprintf(dummy, L"[%s / %d / %06d] : ");

	for (int i = 0; i < 64; i++)
	{
		wsprintf(dummy1, L"%c", _session);
		wcscat_s(dummy, dummy1);
		_session++;
	}
	wcscat_s(dummy, L"\n");
	saveLog(_type, dummy);
}

/*
gameLog::gameLog()
{
	host = "127.0.0.1";
	user = "root";
	pass = "dongho";
	db = "sys";
	port = 3306;

	log = NULL;
	memset(dbName, 0, 128);
	mon = 0;
	dbConnect();
}

void gameLog::dbConnect()
{
	mysql_init(&conn);
	connection = mysql_real_connect(&conn, host, user, pass, db, port, (char*)NULL, 0);
	if (!connection)
	{
		printf("DB CONNECT ERROR \n");
		return;
	}

	mysql_set_character_set(connection, "utf8");
	return;
}

void gameLog::Log(int _accountNo, int _logType, int _logCode, int _Param1, int _Param2, int _Param3, int _Param4, WCHAR *_string, ...)
{
	WCHAR dummy[1024] = L"";
	WCHAR date[128] = L"";
	tm asdf;
	__time64_t time;
	_time64(&time);
	localtime_s(&asdf, &time);

	if (mon != asdf.tm_mon + 1)
	{
		sprintf_s(dbName, "game_log%04d%02d", asdf.tm_year + 1900, asdf.tm_mon + 1);
	}

	wsprintf(date, L"%04d-%02d-%02d %02d:%02d:%02d", asdf.tm_year + 1900, asdf.tm_mon + 1, asdf.tm_mday, asdf.tm_hour, asdf.tm_min, asdf.tm_sec);
	wsprintf(dummy, L"(`date`,`accountNo`,`LogType`,`LogCode`,`Param1`,`Param2`,`Param3`,`Param4`,`ParamString`) VALUES ('%s','%d','%d','%d','%d','%d','%d','%d','%s')",
		date,_accountNo, _logType, _logCode, _Param1, _Param2, _Param3, _Param4, _string);
	saveLog(dummy);

	return;
}

void gameLog::saveLog(WCHAR* _string)
{
	char dummy[1024] = "";
	char dummy1[1024] = "";
	WideCharToMultiByte(CP_UTF8, 0, _string, 1024, dummy, 1024, 0, 0);
	sprintf_s(dummy1, "INSERT INTO `%s`.`%s` %s", db, dbName, dummy);



	int queryState = mysql_query(connection, dummy1);
	if (queryState!= 0 && mysql_errno(connection) == 1146)
	{
		char makeTable[256] = "";
		sprintf_s(makeTable, "CREATE TABLE `%s`.`%s` LIKE `gamelog_template`",db, dbName);
		queryState = mysql_query(connection, makeTable);
		queryState = mysql_query(connection, dummy1);
	}
	else
	{
		printf("%d : %s", mysql_errno(connection), mysql_error(connection));
		return;
	}
	return;
}
*/