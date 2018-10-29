#pragma once

// 2018.03.20  로그인서버 -> 게임서버 패킷 내용 수정
//   - character object id 전송 추가

// 2018.03.23 게임서버 프로토콜 추가

// 2018.03.28 캐릭터 생성 & 삭제 프로토콜 추가

// 2018.09.17	클라이언트 로그인 & 캐릭터 정보 전송 프로토콜 분리
//					패킷 타입 자료형 변경 : unsigned short	->	short
//					요청 결과 자료형 변경 : unsigned char		->	char
//					캐릭터 삭제 패킷데이터 변경 : mongoDB object id -> index

// 2018.09.18	캐릭터 선택 패킷 변경 : Index & character object id 추가
//					캐릭터 생성 및 삭제 패킷 변경 : character object id 추가 & charArr 전송 삭제

// 2018.10.28  회원가입, 회원탈퇴, 공격, 이동처리 등 통합 수정

enum protocol
{
	// 로그인서버
	loginProtocol = 100,

	// 유저 회원가입
	// short		Type
	// char		id[10]
	// char		pass[15]
	login_signUp_req,

	// 유저 회원가입 응답
	// short		Type
	// char		Result
	login_signUp_res,

	// 유저 회원탈퇴
	// short		Type
	// char		id[10]
	// char		pass[15]
	login_withDrawal_req,

	// 유저 회원탈퇴 응답
	// short		Type
	// char		Result
	login_withDrawal_res,

	// 유저 로그인 요청
	// short		Type
	// char		id[10]
	// char		pass[15]
	login_Login_req,

	// 유저 로그인 응답
	// short		Type
	// char		Result			
	login_Login_res,

	// 테스트 유저 로그인 요청
	// short		Type
	// char		id[10]
	// char		pass[15]
	login_test_Login_req,

	// 유저 캐릭터 생성 요쳥
	// short		Type
	// short		charType
	// WCHAR	nickName[10]
	login_createChar_req,

	// 캐릭터 생성 응답
	// short		Type
	// char		Result
	login_createChar_res,

	// 유저 캐릭터 삭제 요청
	// short Type
	// char  charIndex
	// char[25]	objectId
	login_deleteChar_req,

	// 캐릭터 삭제 응답
	// short		Type
	// char		Result
	login_deleteChar_res,

	// 유저가 캐릭터 정보 요청
	// short	Type
	login_charList_req,

	// 유저 캐릭터 정보 응답
	// short		Type
	// char		Result
	// charArr	Data
	login_charList_res,

	// 유저가 캐릭터 선택 
	// short		Type
	// char		charIndex
	login_userSelect_req,

	// 테스트 유저 캐릭터 선택
	// short		Type
	// int			testOID
	login_testSelect_req,

	// 유저 캐릭터 선택 결과
	// short		Type
	//	char		Result
	login_userSelect_res,



	// 로그인서버가 클라이언트(유저)에게 게임서버와 채팅서버의 주소를 보낸다.

	// unsigned __int64		sessionKey
	// char	                    gameIp[16]
	// short		                gamePort
	login_userGameConnect_res,




	// login & server connection and authentication protocol


	// 게임/채팅서버 로그인 요청
	// short		Type
	// char		serverNumber
	// char		serverType
	// char		ip[16]
	// short		port
	login_serverLogin_req,

	// 게임/채팅서버 로그인 응답
	// short		Type
	// char		Result
	login_serverLogin_res,

	// 게임/채팅서버에 캐릭터 접속 알림
	// short						Type
	// unsigned __int64		sessionKey
	// char						charOid[25]
	//	unsigned __int64		accountNo
login_userAuth_req,

// 게임/채팅서버 캐릭터 접속 알림 응답
// short		Type
// char		serverType			: 게임서버, 채팅서버 구분 타입
// unsigned __int64 sessionKey
login_userAuth_res,





gameProtocol = 200,

// 클라이언트(유저)가 게임서버로 접속 시도
// unsigned short		Type;
// unsigned __int64 sessionKey;
// int		version;			// 클라이언트 버전
game_loginUser_req,

// Virutal Client가 게임서버로 접속 시도
// unsigned short		Type;
// unsigned __int64 sessionKey;
// int		version;			// 클라이언트 버전
// int		characterOID
game_loginTestUser_req,

// 게임서버가 클라이언트(유저)에게 접속 시도 결과 전송
// unsigned short		Type;
// unsigned char		Result;
// unsigned __int64		Index;
game_loginUser_res,

// 클라이언트(유저)가 이동요청 전송
// unsigned short	Type;
// unsigned __int64 Index;
// float	xDir;
// float	zDir;
// float	xForward;
// float	zForward;
// float	xPos;
// float	zPos;
game_move_req,

// 서버가 클라이언트(유저)에게 이동 패킷 전송
// unsigned short	Type;
// unsigned __int64	Index;
// float					xDir;
// float					zDir
// float					xForward;
// float					zForward;
game_move_res,

// 클라이언트(유저)가 정지요청 전송
// unsigned short	Type;
// unsigned __int64 Index;
// float					xForward;
// float					zForward;
// float					xPos;
// float					zPos
game_stop_req,

// 서버가 클라이언트(유저)에게 정지 패킷 전송
// unsigned short		Type;
// unsigned __int64		Index;
// float					xForward;
// float					zForward;
// float					xPos;
// float					zPos
game_stop_res,

// 서버와 클라이언트 위치 동기화
// unsigned short	Type;
// unsigned __int64	Index;
// float					xPos;
// float					yPos;
// float					xForward;
// float					zForward;

game_sync_res,

// 서버가 클라이언트(유저) 생성 패킷 전송 (본인)
// unsigned short			Type;
// unsigned __int64			Index;
// unsigned char			charType;
// WCHAR						nickName[NICKNAME_SIZE]
// float							xPos
// float							zPos
// float							xForward
// float							zForward
//	int								Level
// int								maxHP
// int								currentHP
game_newChar_res, game_newUser_res,

// 서버가 클라이언트(유저) 삭제 패킷 전송
// unsigned short			Type;
// unsigned __int64		Index;
game_deleteUser_res,

// 서버가 클라이언트(유저) 삭제 패킷 전송
// unsigned short			Type;
// unsigned __int64		Index;
game_deathUser_res,

// 클라이언트(유저)가 공격 패킷 전송
// unsigned short	Type
// unsigned __int64	Index
// float					xRotation
// float					zRotation
game_attack_req,

// 서버가 클라이언트(유저) 공격 패킷 전송
// unsigned short   Type
// unsigned __int64 Index
// float					xRotation
// float                  zRotation
game_attack_res,


// 클라이언트가 서버에게 타격 패킷 전송
// unsigned short Type
// unsigned __int64 Index
// unsigned __int64 targetIndex
// float                    xPosition
// float                    zPosition
// int                      damage
game_hit_req,

// 서버가 클라이언트에게 타격 패킷 전송
// unsigned short   Type
// unsigned __int64 damagerIndex
// float                    xPosition
// float                    zPosition
// int                      damage
// int                      damagerHP;
game_hit_res,

// 서버가 클라이언트에게 자동 회복 패킷 전송
//	unsigned short	Type
// unsigned __int64	Index
//	int						recoveryAmount
// int						currentHP
game_Recovery_res,

// 게임서버가 로그인서버에게 플레이어 로그아웃을 알림 (login status 수정)
// short Type;
// unsigned __int64 accountNo;
// char		oid[25]
// int			Level
// char	nickName[20]
game_userLogout_req,
};

enum login_Result {
	Failed = 0,
	Overalpped,
	Success,
	accountSuspension,
};

enum login_serverType {
	GAME = 0,
	CHAT,
};

// Game
enum moveDirection
{
	Top, Down, Left, Right,
	TL, TR, DL, DR, None
};

enum playerStatus
{
	NONE, MOVE, ATTACK,
};

enum Game_Login_Result
{
	GAME_Failed = 0,
	GAME_versionMiss,
	GAME_SUCCESS,
};

enum EDHServerType
{
	gameServer = 0, chatServer,
};