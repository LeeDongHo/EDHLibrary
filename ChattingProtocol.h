#pragma once

namespace chatProtocol
{

	enum Protocol
	{
		//------------------------
		// Authentication
		//------------------------
		Authentication = 0,

		// Client to Server] 
		// Ŭ���̾�Ʈ ���� ���� ��û
		// short		Type;
		//	short		clientType;		0 : Virtual Client(Console), 1 : Unity Client
		c2s_Login_Req,

		// Server to Client]
		// Ŭ���̾�Ʈ ���� ��û ����
		// short		Type;
		//	char		Result;			1 : ���, 0 : ����
		s2c_Login_Res,

		//------------------------
		// About player 
		//------------------------
		About_Player,

		// Client to Server]
		// Ŭ���̾�Ʈ���� ������ ĳ���� ���� ��û
		//	short			Type;
		c2s_playerData_Req,

		// Server to Client]
		// Ŭ���̾�Ʈ�� ĳ���� ���� ��Ŷ (���� ä���� Unity client���Ը� ����)
		// short						Type
		//	unsigned __int64		playerCode
		s2c_playerData_Res,

		// Server to Client]
		// Ŭ���̾�Ʈ�� ĳ���� ���� ��Ŷ (���� ä���� Unity client���Ը� ����)
		//	short						Type;
		// unsigned __int64		playerCode			Player ���� �ڵ� (sessionKey)
		//	int							chNumber;				ä�� ��ȣ
		// int							mapNo					�� ��ȣ
		//	int							xPos, yPos;			Tile ��ǥ
		s2c_createPlayer,

		// Server to Client]
		// Ŭ���̾�Ʈ�� ĳ���� ���� ��Ŷ (���� ä���� Unity client���Ը� ����)
		// short						Type
		//	unsigned __int64		playerCode
		s2c_deletePlayer,

		// Client to Server]		
		// short		Type
		// int		destXpos, destYpos
		c2s_playerMove,

		// Client to Server
		//	short		Type
		//	int			mapNo
		c2s_playerMapChange,

		// Client to Server]
		// short		Type
		// int		destCHNumber
		c2s_playerCHChange,

		// Server to Client]
		// short						Type
		//	unsigned __int64		playerCode
		// int							xPos, yPos
		s2c_playerMove,

		// Server to Client]
		// short						Type
		//	unsigned __int64		playerCode
		// bool						Result
		// int							xPos, yPos
		s2c_playerMapChange,

		// Server to Client]
		//	short		Type
		//	unsigned __int64		playerCode
		//	int			chNumber		�̵��� CH ��ȣ
		s2c_playerCHChange,

		//------------------------
		// About chatting 
		//------------------------
		Aboug_Chatting,

		// Client to Server
		// Ŭ���̾�Ʈ�� �������� ä�� ������ ����
		//	short					Type
		// unsigned __int64 playerCode
		//	int						dataSize;
		//	WCHAR				Data[50]
		c2s_Chatting,

		// Server to Client
		// ������ Ŭ���̾�Ʈ���� ä�� ������ ����
		//	short						Type
		// unsigned __int64		playerCode
		//	int							dataSize;
		//	WCHAR					Data[50]
		s2c_Chatting,
	};

	enum chatClientType
	{
		None = 0, virtualClient, unityClient
	};
}