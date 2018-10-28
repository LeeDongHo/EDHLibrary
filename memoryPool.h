#pragma once

template <class T>
class memoryPool
{
private:
	struct memoryBuffer
	{
		memoryBuffer()
		{
			next = nullptr;
		}
		memoryBuffer *next;
		T data;
	};

	struct TOP
	{
		TOP()
		{
			tailNode = NULL;
			uniqueIndex = 0;
		}
		memoryBuffer* tailNode;
		__int64			 uniqueIndex;
	};

public:
	long usedCount;
	long allocCount;

	TOP* topNode;
	__int64 uniqueIndex;

private:
	void clear();

public:
	memoryPool();
	~memoryPool();

	T* Alloc(void);
	void Free(T* _data);

	long getUsedCount(void) { return usedCount; }
	long getAllocCount(void) { return allocCount; }
};

template <class T>
memoryPool<T>::memoryPool()
{
	usedCount = 0;
	allocCount = 0;
	topNode = new TOP;
	uniqueIndex = 0;

}

template <class T>
memoryPool<T>::~memoryPool()
{
	clear();
	delete topNode;
}

template <class T>
void memoryPool<T>::clear()
{
	memoryBuffer *node;
	node = topNode->tailNode;
	while (node)
	{
		topNode->tailNode = topNode->tailNode->next;
		delete node;
		node = topNode->tailNode;
		InterlockedDecrement(&allocCount);
	}
}

template <class T>
T* memoryPool<T>::Alloc(void)	
{
	memoryBuffer* node = NULL;
	TOP dummyTop;
	T* returnData = NULL;
	InterlockedIncrement(&usedCount);
	__int64 unq = InterlockedIncrement64(&uniqueIndex);
	if (!topNode->tailNode)
	{
	alloc:
		node = new memoryBuffer();
		returnData = &(node->data);
		InterlockedIncrement(&allocCount);
	}
	else
	{
		do
		{
			dummyTop.uniqueIndex = topNode->uniqueIndex;
			dummyTop.tailNode = topNode->tailNode;
			if (!dummyTop.tailNode)
				goto alloc;
		} while (InterlockedCompareExchange128((long long*)topNode, unq, (long long)dummyTop.tailNode->next, (long long*)&dummyTop)==0);
		returnData = &(dummyTop.tailNode->data);
	}
	return returnData;
}

template <class T>
void memoryPool<T>::Free(T* _data)
{
	memoryBuffer* dummyBuffer = (memoryBuffer*)((char*)_data - sizeof(char*));
	TOP dummyTop;
	__int64 unq = InterlockedIncrement64(&uniqueIndex);

	do
	{
		dummyTop.uniqueIndex = topNode->uniqueIndex;
		dummyTop.tailNode = topNode->tailNode;
		dummyBuffer->next = dummyTop.tailNode;
	} while (InterlockedCompareExchange128((long long*)topNode, unq, (long long)dummyBuffer, (long long*)&dummyTop) == 0);
	InterlockedDecrement(&usedCount);
}