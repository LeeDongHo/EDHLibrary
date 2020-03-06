#include "stdafx.h"
#include "SBuf.h"

memoryPool<Sbuf>* Sbuf::pool = new memoryPool<Sbuf>;

Sbuf::Sbuf() { init(); }

Sbuf::~Sbuf() { release(); }

void Sbuf::init()
{
	buffer = new char[defaultBuffer];
	memset(buffer, 0, sizeof(char)*defaultBuffer);
	dataPos = buffer + defaultHeader;
	frontPos = dataPos;
	rearPos = dataPos;


	bufferSize = defaultBuffer;
	header = defaultHeader;
	dataSize = 0;
	headerSize = 0;
	_refCount = 0;

	encodeFlag = false;
}

void Sbuf::release()
{
	delete[]buffer;
}

void Sbuf::clear()
{
	dataSize = 0;
	headerSize = defaultHeader;
	dataPos = buffer + defaultHeader;
	frontPos = dataPos;
	rearPos = dataPos;
	encodeFlag = false;
	buffer[0] = 0;
}

void Sbuf::lanClear()
{
	dataSize = 0;
	headerSize = lanHeader;
	dataPos = buffer + lanHeader;
	frontPos = dataPos;
	rearPos = dataPos;
	encodeFlag = false;
}

int Sbuf::getBufSize()
{
	return bufferSize;
}

int Sbuf::getDataSize()
{
	return dataSize;
}

int Sbuf::getPacketSize()
{
	return dataSize + headerSize;
}

char* Sbuf::getBufPtr()
{
	return buffer;
}

char* Sbuf::getHeaderPtr()
{
	return dataPos - headerSize;
}

char* Sbuf::getDataPtr()
{
	return dataPos;
}

char* Sbuf::getFrontPtr()
{
	return frontPos;
}

// 이동은 조금 더 생각해보자 (예외처리와 경우의 수)
int Sbuf::moveFrontPos(int _pos)
{
	if (_pos < 0) throw 4960;
	if (dataSize < _pos)throw 4960;

	int dummy = frontPos - &buffer[_pos];
	frontPos = frontPos + _pos;
	dataSize -= _pos;
	return -dummy;
}

int Sbuf::moveRearPos(int _pos)
{
	if (_pos < 0) throw 4960;
	if (bufferSize - dataSize < _pos) throw 4960;

	int dummy = rearPos - &buffer[_pos];
	rearPos = rearPos + _pos;
	dataSize += _pos;
	return -dummy;
}

int Sbuf::push(char* dest, int _size)
{
	if (bufferSize - dataSize < _size)
		throw 4960;
	memcpy(rearPos, dest, _size);
	rearPos += _size;
	dataSize += _size;
	return _size;
}

int Sbuf::push(const char* dest, int _size)
{
	if (bufferSize - dataSize < _size)
		throw 4960;
	memcpy(rearPos, dest, _size);
	rearPos += _size;
	dataSize += _size;
	return _size;
}

int Sbuf::pop(char* dest, int _size)
{
	if (_size > dataSize)
		throw 4960;
	memcpy(dest, frontPos, _size);
	frontPos += _size;
	dataSize -= _size;
	return _size;
}

bool Sbuf::setHeader(char* dest)
{
	if (!dest) return false;
	memcpy(buffer, dest, header);
	headerSize = header;
	return true;
}

bool Sbuf::setHeaderCustom(char *dest, int _size)
{
	if (_size > header || !dest) return false;
	char *pos = dataPos - _size;
	memcpy(pos, dest, _size);
	headerSize = _size;
	return true;
}

bool Sbuf::setHeaderShort(void)
{
	char *pos = dataPos - headerSize;
	buffer[0] = dataSize;
	buffer[1] = dataSize >> 8;
	headerSize = header;
	return true;
}

Sbuf& Sbuf:: operator << (BYTE _value)
{
	push((char*)&_value, sizeof(BYTE));
	return *this;
}

Sbuf& Sbuf::operator<<(char _value)
{
	push(&_value, sizeof(char));
	return *this;
}

Sbuf& Sbuf::operator<<(short _value)
{
	push((char*)&_value, sizeof(short));
	return *this;
}

Sbuf& Sbuf::operator<<(WORD _value)
{
	push((char*)&_value, sizeof(WORD));
	return *this;
}

Sbuf& Sbuf::operator<<(bool _value)
{
	push((char*)&_value, sizeof(bool));
	return *this;
}

Sbuf& Sbuf::operator<<(int _value)
{
	push((char*)&_value, sizeof(int));
	return *this;
}

Sbuf& Sbuf::operator<<(DWORD _value)
{
	push((char*)&_value, sizeof(DWORD));
	return *this;
}

Sbuf& Sbuf::operator<<(float _value)
{
	push((char*)&_value, sizeof(float));
	return *this;
}

Sbuf& Sbuf::operator<<(unsigned int _value)
{
	push((char*)&_value, sizeof(unsigned int));
	return *this;
}

Sbuf& Sbuf::operator<<(__int64 _value)
{
	push((char*)&_value, sizeof(__int64));
	return *this;
}

Sbuf& Sbuf::operator<<(unsigned __int64 _value)
{
	push((char*)&_value, sizeof(unsigned __int64));
	return *this;
}

Sbuf& Sbuf::operator<<(double _value)
{
	push((char*)&_value, sizeof(double));
	return *this;
}

// 시프트 연산자 오버로딩 OUT
// PARAMETERS : 값을 저장시킬 변수, 사이즈

Sbuf& Sbuf::operator >> (BYTE &_value)
{
	pop((char*)&_value, sizeof(BYTE));
	return *this;
}

Sbuf& Sbuf::operator >> (char &_value)
{
	pop((char*)&_value, sizeof(char));
	return *this;
}

Sbuf& Sbuf::operator >> (short &_value)
{
	pop((char*)&_value, sizeof(short));
	return *this;
}

Sbuf& Sbuf::operator >> (WORD &_value)
{
	pop((char*)&_value, sizeof(WORD));
	return *this;
}

Sbuf& Sbuf::operator >> (bool &_value)
{
	pop((char*)&_value, sizeof(bool));
	return *this;
}

Sbuf& Sbuf::operator >> (int &_value)
{
	pop((char*)&_value, sizeof(int));
	return *this;
}

Sbuf& Sbuf::operator >> (DWORD &_value)
{
	pop((char*)&_value, sizeof(DWORD));
	return *this;
}

Sbuf& Sbuf::operator >> (float &_value)
{
	pop((char*)&_value, sizeof(float));
	return *this;
}

Sbuf& Sbuf::operator >> (unsigned int &_value)
{
	pop((char*)&_value, sizeof(unsigned int));
	return *this;
}

Sbuf& Sbuf::operator >> (__int64 &_value)
{
	pop((char*)&_value, sizeof(__int64));
	return *this;
}

Sbuf& Sbuf::operator >> (unsigned __int64 &_value)
{
	pop((char*)&_value, sizeof(unsigned __int64));
	return *this;
}

Sbuf& Sbuf::operator >> (double &_value)
{
	pop((char*)&_value, sizeof(double));
	return *this;
}

LONG Sbuf::addRef(void)
{
	return InterlockedIncrement(&_refCount);
}

Sbuf* Sbuf::Alloc()
{
	Sbuf *buf = pool->Alloc(); 
	buf->clear();
	buf->addRef();			// 시작 refCount = 1;
	return buf;
}

Sbuf* Sbuf::lanAlloc()
{
	Sbuf *buf = pool->Alloc();
	buf->lanClear();
	buf->addRef();
	return buf;
}
void Sbuf::Free()
{
	if (0 == InterlockedDecrement(&_refCount))
		pool->Free(this);
	return;
}

void Sbuf::Encode(BYTE _code, BYTE _key1, BYTE _key2)
{
	if (encodeFlag == false)
	{
		if (false == InterlockedCompareExchange(&encodeFlag, true, false))
		{
			BYTE key = _key1 ^ _key2;
			BYTE randCode = rand() % 256;
			int size = dataSize + defaultHeader;

			// 체크썸
			BYTE checkSum;
			int check=0;
			for (int i = defaultHeader; i < size; i++)
			{
				check += buffer[i];
				buffer[i] ^= randCode;
			}
			checkSum = check % 256;
			checkSum ^= randCode;

			buffer[0] = _code;
			buffer[1] = dataSize;
			buffer[2] = dataSize >> 8;
			buffer[3] = randCode ^ key;
			buffer[4] = checkSum;
		}
	}
}

void Sbuf::lanEncode(void)
{
	buffer[0] = dataSize;
	buffer[1] = dataSize >> 8;
}

bool Sbuf::Decode(BYTE _code, BYTE _key1, BYTE _key2)
{
	BYTE key = _key1 ^ _key2;
	short size = dataSize + defaultHeader;
	int check = 0;
	// 1차 복호화
	buffer[3] ^= key;
	BYTE randCode = buffer[3];
	buffer[4] ^= randCode;
	for (int a = defaultHeader; a < size; a++)
	{
		buffer[a] ^= randCode;
		check += buffer[a];
	}

	// 체크썸
	BYTE checkSum = check % 256;

	if ((BYTE)buffer[0] != _code)
		return false;
	if (checkSum == (BYTE)buffer[4])
		return true;
	return false;
}

bool Sbuf::lanDecode()
{
	return true;
}