#pragma once

enum monitorProtocol
{
	//	short					protocolType
	// short					clientType
	//	unsigned char	clientNameSize
	// char*					clientName
	//	unsigned char	dataSize
	// monitorData		Data[]
	requestClientLogin = 0,

	//	short					protocolType
	//	unsigned char	loginResut
	//	unsigned char	authorizedClientCode
	//	unsigned char	authorizedDataSize
	responseClientLogin,

	// short					protocolType
	//	unsigned char	authorizedClientCode
	//	unsigned char	dataSize
	//	ULONGLONG		Data[]
	requestSetMonitorData,
};

enum monitorClientType
{
	Null = 0, Server, Client
};