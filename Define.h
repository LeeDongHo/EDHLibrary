#pragma once

// EDH MMORPG game server graduate project

// define

// Common
#define setID(index, clientID) (index << 48) | clientID
#define getID(clientID) (clientID<<16) >>16
#define getIndex(clientID) clientID>>48

#define NICKNAME_SIZE 10		// User nickname size

// Game
#define AUTH_ACCEPT_DELAY  5
#define AUTH_AUTH_DELAY 5
#define GAME_ACCEPT_DEALY 5
#define GAME_RELEASE_DEALY 5
#define GAME_MODE_DEALY 5
#define GAME_LOGOUT_DEALY 3

#define GAME_TILE_ERROR_RANGE 2

#define SPEED_FORWARD	155
#define SPEED_DIAGONAL	235


//---------------------------------------------
// 클라이언트 좌표를 서버 타일 좌표로 변경
// TILE to Pos 는 클라좌표로 변경 후 랜덤하게 소수점을 추가한다.
//---------------------------------------------
#define POS_to_TILE_X(Pos) ((int)((Pos) * 2.0f))
#define POS_to_TILE_Y(Pos) (((int)((Pos) * 2.0f)))

#define TILE_to_POS_X(Tile) ((double)((Tile) / 2.0f) + 0.25f)
#define TILE_to_POS_Y(Tile) ((double)((Tile) / 2.0f) + 0.25f)


//---------------------------------------------
// 타일 좌표를 섹터 좌표로 변경
//---------------------------------------------
#define TILE_to_SECTOR_X(xTile,wideSize) (xTile / wideSize)
#define TILE_to_SECTOR_Y(yTile,heightSize) (yTile / heightSize)


//---------------------------------------------
// 타음타일 이동 시간.
//---------------------------------------------
inline ULONGLONG getSpeed(moveDirection _dir)
{
	ULONGLONG speed;
	switch (_dir)
	{
	case moveDirection::Left:
	case moveDirection::Right:
	case moveDirection::Top:
	case moveDirection::Down:
		speed = SPEED_FORWARD;	// 130
		break;
	case moveDirection::TR:
	case moveDirection::TL:
	case moveDirection::DL:
	case moveDirection::DR:
		speed = SPEED_DIAGONAL;	// 210
		break;
	}
	return speed;
}

//--------------------------------------------
// 타일 이동.
//--------------------------------------------

inline void getMoveReslt(moveDirection _dir, int _sxTile, int _syTile, int *_dxTile, int *_dyTile)
{
	switch (_dir)
	{
	case moveDirection::Left:
		*_dxTile = _sxTile - 1;
		*_dyTile = _syTile;
		break;
	case moveDirection::Right:
		*_dxTile = _sxTile + 1;
		*_dyTile = _syTile;
		break;
	case moveDirection::Top:
		*_dxTile = _sxTile;
		*_dyTile = _syTile - 1;
		break;
	case moveDirection::Down:
		*_dxTile = _sxTile;
		*_dyTile = _syTile + 1;
		break;
	case moveDirection::TR:
		*_dxTile = _sxTile + 1;
		*_dyTile = _syTile - 1;
		break;
	case moveDirection::TL:
		*_dxTile = _sxTile - 1;
		*_dyTile = _syTile - 1;
		break;
	case moveDirection::DL:
		*_dxTile = _sxTile - 1;
		*_dyTile = _syTile + 1;
		break;
	case moveDirection::DR:
		*_dxTile = _sxTile + 1;
		*_dyTile = _syTile + 1;
		break;
	}

	return;
}

//--------------------------------------------
// 이동시간에 따른 타일좌표.
//--------------------------------------------

inline bool getTileByMovingtime(moveDirection _dir, ULONGLONG _speed, ULONGLONG _time, int _sxTile, int _syTile, int *_dxTile, int *_dyTile,int _wideSize, int _heightSize, int _overTime = 0)
{
	if (!_dxTile || !_dxTile)
		return false;

	ULONGLONG distance = _time / _speed;			// 이동시간만큼 이동해야하는 횟수 계산

	int count = 0;
	for (count; count < distance; count++)
	{
		getMoveReslt(_dir, _sxTile, _syTile, _dxTile, _dyTile);
	}
	
	if (*_dxTile < 0 || *_dyTile < 0 || *_dxTile >= _wideSize || *_dyTile >= _heightSize)
	{
		*_dxTile = -1;
		*_dyTile = -1;
		return false;
	}

	return true;
}


inline bool checkOverTime(moveDirection _oDir, moveDirection _cDir)
{
	switch (_oDir)
	{
	case moveDirection::Left:
	case moveDirection::TL:
	case moveDirection::DL:
		if (_cDir == moveDirection::Right || _cDir == moveDirection::TR || _cDir == moveDirection::DR)
			return true;
	case moveDirection::Right:
	case moveDirection::TR:
	case moveDirection::DR:
		if (_cDir == moveDirection::Left || _cDir == moveDirection::TL || _cDir == moveDirection::DL)
			return true;
	}


	return false;
}
