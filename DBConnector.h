// MongoDB lib setting
// 1. 디버깅 - 환경 : PATH=c:/mongo-c-driver/bin
// 2. 추가포함 디렉터리 : C:\mongo-c-driver\include\libmongoc-1.0 &  C:\mongo - c - driver\include\libbson - 1.0
// 3. 링커 - 입력 - 추가종속성 : C:\mongo-c-driver\lib\bson-1.0.lib &  C:\mongo - c - driver\lib\mongoc - 1.0.lib
// 4. 헤더  :  #include <mongoc.h>
// dll file check directory

// Mysql lib setting
// 1. 추가포함 디렉터리 : mysql\include
// 2. 추가 라이브 디렉터리 : C:\Program Files\MySQL\MySQL Server 5.7\lib;
#pragma comment(lib, "libmysql")

#include <mysql.h>
#include <errmsg.h>
#include <string>

#include <mongoc.h>

class mysqlConnector
{
private:
	char host[16];
	char user[32];
	char pass[32];
	char dbName[32];
	int port;

	MYSQL sql;
	MYSQL *con;
	MYSQL_RES *sqlResult;	// RESULT

	int errNo;
	char errMsg[256];
	std::string queryString;

	bool setFlag;

protected:
	bool connect();
	void disconnect();
	void errSave();

public:
	mysqlConnector();
	~mysqlConnector();

	void setDB(const char *_host, const  char *_user, const  char *_pass, const char *_dbName, const int _port);

	bool Query(const std::string _string);
	bool selectQuery(const std::string _string);

	MYSQL_ROW fetchRow();
	void freeResult();
};

class mongoConnector
{
private:
	char uri[64];
	char document[32];
	char collection[32];
	char serverName[16];

	mongoc_client_t *client;
	mongoc_collection_t *coll;

	mongoc_cursor_t *cursor;
	bson_error_t error;
	char *str;
	bool retval;

	bool setFlag;
	bool connectFlag;

protected:
	bool connect();
	void disconnect();
	void errSave();

public:
	mongoConnector();
	~mongoConnector();

	void setDB(const char *_Uri, const char *_Document, const char *_Collection, const char *_serverName);

	bool upsertQuery(const bson_t *_Selector, const bson_t *_Query);
	bool insertQuery(const bson_t *_Query);
	bool findQuery(const bson_t *_Selector);
	bool deleteQuery(const bson_t *_Selector);

	char* cursorNext(const bson_t *_doc);
	void strFree();
};
