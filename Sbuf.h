#pragma once

#include "memoryPool.h"

class Sbuf
{
private:
	enum SbufPacket
	{
		defaultBuffer = 10000,
		defaultHeader = 5,
		lanHeader = 2,
		// 이 사이즈는 적용 시 클라이언트 혹은 서버의 최대 패킷의 수에 맞춰도 무방
		// 후에 안전성을 위해서 이 범위를 초과하는 패킷사이즈가 있으면 패킷 사이즈를 조정하도록? 해볼 생각
	};

	void	init();				// 메모리할당 함수 함부로 호출 X
	void	release();		// 메모리 해제 함수

public:
	Sbuf();
	~Sbuf();

	void	clear();			// 버퍼를 비운다. (버퍼를 삭제하는것은 아님)
	void	lanClear();

	int		getBufSize();	// 버퍼의 최대 사이즈를 얻는다. (버퍼사이즈 반환)
	int		getDataSize();// 버퍼에 저장된 데이터의 사이즈값 (헤더제외)을 얻는다.
	int		getPacketSize(); // 헤더 포함 사이즈값을 얻는다.

	char* getBufPtr();	// 버퍼의 포인터 반환
	char* getHeaderPtr(); // 헤더 시작부의 포인터 반환
	char* getDataPtr();
	char* getFrontPtr();

	int		moveFrontPos(int _pos);	// 버퍼의 front(삭제) 위치를 변경한다.
	int		moveRearPos(int _pos);	// 버퍼의 Rear(삽입) 위치를 변경한다.

	bool		setHeader(char *dest);	// 5바이트 헤더 삽입.
	bool		setHeaderCustom(char *dest, int _size);		// _size 크기만큼의 헤더 삽입
	bool		setHeaderShort(void);		// unsigned short 크기만큼 헤더 삽입.

	void Encode(BYTE _code, BYTE _key1, BYTE _key2);
	void lanEncode(void);
	bool Decode(BYTE _code, BYTE _key1, BYTE _key2);
	bool lanDecode(void);

	LONG	addRef();
	
	int		push(char *dest, int _size);
	int		pop(char* dest, int _size);
	static Sbuf*Alloc();
	static Sbuf*lanAlloc();
	void		Free();

private:

	int		bufferSize;
	int		dataSize;
	int		header;
	int		headerSize;

	char*		frontPos;		// 삭제
	char*		rearPos;			// 삽입
	char*		dataPos;			// 데이터 영역

									
	volatile LONG _refCount;

	LONG encodeFlag;

public:
	char	*buffer;

public:
	static memoryPool<Sbuf> *pool;

public:
	// 시프트연산자 오버로딩 IN
	// 1byte
	Sbuf &operator << (BYTE _value);
	Sbuf &operator << (char _value);

	// 2byte
	Sbuf &operator << (short _value);
	Sbuf &operator << (WORD _value);

	// 4byte
	Sbuf &operator << (int _value);
	Sbuf &operator << (DWORD _value);
	Sbuf &operator << (float _value);

	// 8byte
	Sbuf &operator << (__int64 _value);
	Sbuf &operator << (unsigned __int64 _value);
	Sbuf &operator << (double _value);

	// 시프트 연산자 오버로딩 OUT
	// 1byte
	Sbuf &operator >> (BYTE &_value);
	Sbuf &operator >> (char &_value);

	// 2byte
	Sbuf &operator >> (short &_value);
	Sbuf &operator >> (WORD &_value);

	// 4byte
	Sbuf &operator >> (int &_value);
	Sbuf &operator >> (DWORD &_value);
	Sbuf &operator >> (float &_value);

	// 8byte
	Sbuf &operator >> (__int64 &_value);
	Sbuf &operator >> (unsigned __int64 &_value);
	Sbuf &operator >> (double &_value);
};
