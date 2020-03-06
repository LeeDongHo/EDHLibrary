#pragma once
//#pragma comment(lib,"mysqlclient.lib")

//#include <mysql.h>
//#include <errmsg.h>
#include <time.h>
#include <stdarg.h>
#include <strsafe.h>

#define _SYSLOG(_type, _level, _string, ...) systemLog::sysLog(_type,_level,_string, __VA_ARGS__)
#define _DEBUGLOG(_type, _funcName, _string, ...) systemLog::debugLog(_type,_funcName,_string, __VA_ARGS__)

	enum Level
	{
		SYS_SYSTEM =0, 
		SYS_ERROR, SYS_WARNING, SYS_DEBUG
	};

	enum Type
	{
		Type_DB=0, 
		Type_CONSOLE, Type_FILE, Type_WEB
	};

class systemLog
	{
	private:
		static systemLog *log;
		volatile int logCounter;
		WCHAR *folder;
		Type type;
		Level level;

	private:
		systemLog();
		~systemLog() {};
	public:
		static systemLog* getInstance(void)
		{
			if (log == NULL)
			{
				log = new systemLog;
				atexit(Destroy);
			}
			return log;
		}

		static void Destroy(void)
		{
			if(log)
				delete log;
		}

		void setSys(Type _Type, Level _level);
		static void sysLog(Type _type, Level _level, WCHAR *_string, ...);
		static void debugLog(WCHAR *_Type, WCHAR *_functionName, WCHAR *_string, ...);
		void saveLog(Type _type, WCHAR *_string);
		void saveLog(WCHAR *_type, WCHAR *_string);

		void hexSave(Type _type, Level _level, BYTE *_pointer, BYTE _len);
		void sessionSave(Type _type, Level _level, BYTE *_session);

	};

/*
class gameLog
{
private:
	static gameLog *log;
	
	char* host;
	char* user;
	char* pass;
	char* db;
	unsigned int port;

	MYSQL conn;
	static MYSQL *connection;

	char dbName[128];
	int mon;

private:
	gameLog();
	~gameLog() {};
	void dbConnect();

public:
	static gameLog* getInstance()
	{
		if (!log)
		{
			log = new gameLog();
			atexit(Destroy);
		}
		return log;
	}

	static void Destroy(void)
	{
		delete log;
		if (connection)
			mysql_close(connection);
	}

	void Log(int _accountNo, int _logType, int _logCode, int _Param1, int _Param2, int _Param3, int _Param4,WCHAR *_string,...);
	void saveLog(WCHAR* _string);
	
};
*/