#include "stdafx.h"
#include "winQueue.h"


winBuffer::winBuffer()
{
	buffer = new char[bufferSize];
	bufSize = bufferSize;
	init();
}

winBuffer::winBuffer(int size)
{
	buffer = new char[size];
	bufSize = size;
	init();
}

winBuffer::~winBuffer()
{
	delete[] buffer;
}

void winBuffer::init()
{
	memset(buffer, 0, sizeof(buffer));
	Rear = 0;
	Front = 0;
}

// 총 버퍼의 사이즈 리턴
int winBuffer::getBufferSize()
{
	return bufSize;
}

int winBuffer::getUsedSize()
{
	int dummyRear = Rear;
	int dummyFront = Front;
	if (dummyRear - dummyFront >= 0)
		return dummyRear - dummyFront;
	else
		return bufSize - dummyFront + dummyRear;
}

int winBuffer::getFreeSize()
{
	int usedSize = getUsedSize();
	return bufSize - usedSize - 1;
}

int winBuffer::getNotBrokenFreeSize()
{
	int dummyRear = Rear;
	int dummyFront = Front;
	if (dummyRear - dummyFront >= 0)
		return bufSize - dummyRear;
	else
		return dummyFront - dummyRear - 1;
}

int winBuffer::getNotBrokenUsedSize()
{
	int dummyRear = Rear;
	int dummyFront = Front;
	if (dummyRear - dummyFront >= 0)
		return dummyRear - dummyFront;
	else
		return bufSize - dummyFront;
}

int winBuffer::enqueue(char* dest, int size)
{
	int dummyRear = Rear;
	int dummyFront = Front;
	int freeSize = dummyFreeSize(dummyRear, dummyFront);
	if (size < 0 || !dest || freeSize == 0) return 0;

	if (size > freeSize)
		size = freeSize;

	int dummy = dummyRear;
	int count = 0;
	for (count; count < size; count++)
	{
		buffer[dummy] = *dest;
		dest++;
		dummy = (dummy + 1) % bufSize;
	}
	Rear = dummy;

	return count;
}

int winBuffer::dequeue(char* dest, int size)
{
	int dummyRear = Rear;
	int dummyFront = Front;
	int usedSize = dummyUsedSize(dummyRear, dummyFront);
	if (usedSize == 0) return 0;

	int count = 0;
	int dummy = dummyFront;

	if (usedSize < size)
		size = usedSize;

	for (count = 0; count < size; count++)
	{
		if (usedSize == 0) break;

		*dest = buffer[dummy];
		*dest++;
		dummy = (dummy + 1) % bufSize;
	}
	Front = dummy;
	return count;
}

int winBuffer::peek(char* dest, int size)
{
	int dummyRear = Rear;
	int dummyFront = Front;
	int usedSize = dummyUsedSize(dummyRear, dummyFront);
	if (usedSize == 0) return 0;

	int count = 0;
	int dummy = dummyFront;
	if (usedSize < size)
		size = usedSize;
	for (count = 0; count < size; count++)
	{
		*dest = buffer[dummy];
		*dest++;
		dummy = (dummy+1) % bufSize;
	}

	return count;
}

int winBuffer::peek(char *dest, int pos, int size)
{
	int dummyRear = Rear;
	int dummyFront = Front;
	int usedSize = dummyUsedSize(dummyRear, dummyFront);
	if (usedSize < pos + size) return 0;

	int dummy = (dummyFront + pos) % bufSize;

	for (int i = 0; i < size; i++)
	{
		*dest = buffer[dummy];
		*dest++;
		dummy = (dummy + 1) % bufSize;
	}

	return size;
}

void winBuffer::removeData(int size)
{
	int dummyFront = Front;
	if (dummyFront + size < bufSize)
		dummyFront += size;
	else
		dummyFront = (dummyFront + size) % bufSize;
	Front = dummyFront;
}

void winBuffer::moveFrontPos(int _pos)
{
	int dummyFront = Front;
	if (dummyFront + _pos > bufSize)
		dummyFront = (dummyFront + _pos) % bufSize;
	else
		dummyFront += _pos;
	Front = dummyFront;
	return;
}

void winBuffer::moveRearPos(int _pos)
{
	int dummyRear = Rear;
	if (dummyRear + _pos >= bufSize)
		dummyRear = (dummyRear + _pos) % bufSize;
	else
		dummyRear += _pos;
	Rear = dummyRear;
	return;
}

void winBuffer::clearBuffer()
{
	Front = 0;
	Rear = 0;
}

char* winBuffer::getBufferPtr()
{
	return buffer;
}

char* winBuffer::getFrontPosPtr()
{
	return &buffer[Front];
}

char* winBuffer::getRearPosPtr()
{
	return &buffer[Rear];
}

int winBuffer::dummyFreeSize(int _rear, int _front)
{
	if (_rear - _front >= 0)
		return bufSize - (_rear + _front) - 1;
	else
		return _front-_rear-1;
}

int winBuffer::dummyUsedSize(int _rear, int _front)
{
	if (_rear - _front >= 0)
		return _rear - _front;
	else
		return bufSize - _front + _rear;
}