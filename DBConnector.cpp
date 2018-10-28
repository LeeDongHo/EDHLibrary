#include "stdafx.h"

mysqlConnector::mysqlConnector()
{
	con = NULL;
	setFlag = false;
}

mysqlConnector :: ~mysqlConnector()
{
	if (!con)
		disconnect();
}

void mysqlConnector::setDB(const char *_host, const  char *_user, const  char *_pass, const char *_dbName, const int _port)
{
	strcpy_s(host, 16, _host);
	strcpy_s(user, 32, _user);
	strcpy_s(pass, 32,_pass);
	strcpy_s(dbName,32, _dbName);
	port = _port;
	setFlag = true;
}

bool mysqlConnector::connect()
{
	if (con)
		return true;

	mysql_init(&sql);
	con = mysql_real_connect(&sql, host, user, pass, dbName, port, (char*)NULL, 0);

	if (!con)
	{
		errSave();
		return false;
	}
	mysql_set_character_set(con, "utf8");

	return true;
}

void mysqlConnector::disconnect()
{
	if (!con) 	
		return;

	mysql_close(con);
	con = NULL;
}

bool mysqlConnector::updateQuery(const std::string	_string)
{
	int count = 0;
	unsigned int lastError = 0;
	queryString = _string;
	if (!con)
		connect();
	int retval = mysql_query(con, _string.c_str());
	if (retval != 0)
	{
		while (count < 10)
		{
			lastError = mysql_errno(con);
			if (lastError == CR_CONNECTION_ERROR ||
				lastError == CR_CONN_HOST_ERROR ||
				lastError == CR_SERVER_GONE_ERROR ||
				lastError == CR_TCP_CONNECTION ||
				lastError == CR_SERVER_HANDSHAKE_ERR ||
				lastError == CR_SERVER_LOST ||
				lastError == CR_INVALID_CONN_HANDLE)
			{
				count++;
				connect();
				retval = mysql_query(con, _string.c_str());
				if (retval != 0) continue;
				return true;
			}
				break;
		}
		errSave();
		return false;
	}
	return true;
}

bool mysqlConnector::selectQuery(const std::string _string)
{
	int count = 0;
	int lastError = 0;
	queryString = _string;
	if(!con)
		connect();
	int retval = mysql_query(con, _string.c_str());
	if (retval != 0)
	{
		while (count < 10)
		{
			lastError = mysql_errno(con);
			if (lastError == CR_CONNECTION_ERROR ||
				lastError == CR_CONN_HOST_ERROR ||
				lastError == CR_SERVER_GONE_ERROR ||
				lastError == CR_TCP_CONNECTION ||
				lastError == CR_SERVER_HANDSHAKE_ERR ||
				lastError == CR_SERVER_LOST ||
				lastError == CR_INVALID_CONN_HANDLE)
			{
				count++;
				connect();
				retval = mysql_query(con, _string.c_str());
				if (retval != 0) continue;
				sqlResult = mysql_store_result(con);
				return true;
			}
			break;
		}
		errSave();
		return false;
	}
	sqlResult = mysql_store_result(con);
	return true;
}

MYSQL_ROW mysqlConnector::fetchRow()
{
	MYSQL_ROW row = mysql_fetch_row(sqlResult);
	return row;
}

void mysqlConnector::freeResult()
{
	mysql_free_result(sqlResult);
	sqlResult = NULL;
}

void mysqlConnector::errSave()
{
	errNo = mysql_errno(&sql);
	strcpy_s(errMsg,mysql_error(&sql));
}

mongoConnector::mongoConnector()
{
	mongoc_init();
	setFlag = false;
}

mongoConnector::~mongoConnector()
{
	disconnect();
	mongoc_cleanup();
}

void mongoConnector::setDB(const char *_Uri, const char *_Document, const char *_Collection, const char *_serverName)
{
	strcpy_s(uri, 64,_Uri);
	strcpy_s(document, 32,_Document);
	strcpy_s(collection, 32,_Collection);
	strcpy_s(serverName, 16, _serverName);
	setFlag = true;
}

bool mongoConnector::connect()
{
	if (connectFlag)
		return true;
	client = mongoc_client_new(uri);
	mongoc_client_set_appname(client, serverName);
	// application name
	coll = mongoc_client_get_collection(client, document, collection);

	return true;
}

void mongoConnector::disconnect()
{
	mongoc_cursor_destroy(cursor);
	mongoc_collection_destroy(coll);
	mongoc_client_destroy(client);
	connectFlag = false;
}

bool mongoConnector::upsertQuery(const bson_t *_Selector, const bson_t *_Query)
{
	if (!connectFlag)
		connect();

	bson_t *opt = bson_new();

	if (!mongoc_collection_update_one(coll, _Selector, _Query, opt, NULL, &error)) 
	{
		errSave();
		bson_free(opt);
		return false;
	}
	bson_free(opt);
	return true;
}

bool mongoConnector::insertQuery(const bson_t *_Query)
{
	if (!connectFlag)
		connect();
	bson_t reply;
	if (!mongoc_collection_insert_one(coll, _Query, NULL,&reply, &error))
	{
		errSave();
		return false;
	}
	return true;
}

bool mongoConnector::findQuery(const bson_t *_Selector)
{
	if (!connectFlag)
		connect();

	cursor = mongoc_collection_find(coll, MONGOC_QUERY_NONE,0,0,0,_Selector, NULL, NULL);
	return true;
}

bool mongoConnector::deleteQuery(const bson_t *_Selector)
{
	if (!connectFlag)
		connect();

	if (!mongoc_collection_delete_one(coll, _Selector, NULL, NULL, &error))
	{
		errSave();
		return false;
	}
	return true;
}

char* mongoConnector::cursorNext(const bson_t *_doc)
{
	if (!mongoc_cursor_next(cursor, &_doc))
		return NULL;
	str = bson_as_json(_doc, NULL);
	return str;
}

void mongoConnector::strFree()
{
	if (!str) return;
	bson_free(str);
	str = NULL;
}

void mongoConnector::errSave()
{
	fprintf_s(stderr, "ERROR : %s\n", error.message);
}