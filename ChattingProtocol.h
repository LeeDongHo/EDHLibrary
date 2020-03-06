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
		// 클라이언트 접속 인증 요청
		// short		Type;
		//	short		clientType;		0 : Virtual Client(Console), 1 : Unity Client
		c2s_Login_Req,

		// Server to Client]
		// 클라이언트 접속 요청 응답
		// short		Type;
		//	char		Result;			1 : 허락, 0 : 거절
		s2c_Login_Res,

		//------------------------
		// About player 
		//------------------------
		About_Player,

		// Client to Server]
		// 클라이언트에서 서버로 캐릭터 정보 요청
		//	short			Type;
		c2s_playerData_Req,

		// Server to Client]
		// 클라이언트에 캐릭터 삭제 패킷 (같은 채널의 Unity client에게만 보냄)
		// short						Type
		//	unsigned __int64		playerCode
		s2c_playerData_Res,

		// Server to Client]
		// 클라이언트에 캐릭터 생성 패킷 (같은 채널의 Unity client에게만 보냄)
		//	short						Type;
		// unsigned __int64		playerCode			Player 구분 코드 (sessionKey)
		//	int							chNumber;				채널 번호
		// int							mapNo					맵 번호
		//	int							xPos, yPos;			Tile 좌표
		s2c_createPlayer,

		// Server to Client]
		// 클라이언트에 캐릭터 삭제 패킷 (같은 채널의 Unity client에게만 보냄)
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
		//	int			chNumber		이동된 CH 번호
		s2c_playerCHChange,

		//------------------------
		// About chatting 
		//------------------------
		Aboug_Chatting,

		// Client to Server
		// 클라이언트가 서버에게 채팅 데이터 전송
		//	short					Type
		// unsigned __int64 playerCode
		//	int						dataSize;
		//	WCHAR				Data[50]
		c2s_Chatting,

		// Server to Client
		// 서버가 클라이언트에게 채팅 데이터 전송
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