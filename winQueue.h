#pragma once


class winBuffer
{
private:
	//	int Rear, Front;
	char* buffer;

	int bufSize;

	enum {
		bufferSize = 2048
	};

public:
	int Rear, Front;
	winBuffer();
	winBuffer(int _size);
	~winBuffer();

	void init();

	int getBufferSize();
	int getUsedSize();
	int getFreeSize();
	int getNotBrokenFreeSize();
	int getNotBrokenUsedSize();

	int enqueue(char* dest, int size);
	int dequeue(char* dest, int size);
	int peek(char* dest, int size);
	int peek(char* dest, int pos, int size);

	void removeData(int size);
	void moveFrontPos(int pos);
	void moveRearPos(int pos);

	void clearBuffer();

	char* getBufferPtr();
	char* getFrontPosPtr();
	char* getRearPosPtr();

private:
	int dummyFreeSize(int _rear, int _front);
	int dummyUsedSize(int _rear, int _front);
};