#pragma once

#include "memoryPool.h"

template <class T>
class lockFreeQueue
{
private:
	struct node
	{
		node()
		{
			next = NULL;
		}
		node* next;
		T data;
	};

	struct TOP
	{
		TOP()
		{
			node = NULL;
			uniqueIndex = 0;
		}
		node *node;
		unsigned __int64 uniqueIndex;
	};

private:
	LONG usedSize;
	memoryPool<node> *pool;

	TOP *headTop;
	TOP *tailTop;

	volatile LONG64 uniqueIndex;
	
public:
	lockFreeQueue();
	~lockFreeQueue();

	void clear();

	void dequeue(T *data);
	void enqueue(T data);
	int peek(T *data, int _pos);

	int getUsedSize(void);
};

template <class T>
lockFreeQueue<T>::lockFreeQueue()
{
	usedSize = 0;
	uniqueIndex = 0;
	headTop = new TOP;
	tailTop = new TOP;
	pool = new memoryPool<node>;
	headTop->node = pool->Alloc();
	tailTop->node = headTop->node;
}

template <class T>
lockFreeQueue<T>::~lockFreeQueue()
{
	clear();
	delete headTop;
	delete tailTop;
	delete pool;
}

template <class T>
void lockFreeQueue<T>::clear()
{
	node *_node = headTop->node;
	node *nodeNext = NULL;

	while (_node)
	{
		nodeNext = _node->next;
		pool->Free(_node);
		_node = nodeNext;
		InterlockedDecrement(&usedSize);
	}
}

template <class T>
void lockFreeQueue<T>::enqueue(T data)
{
	node *_node = pool->Alloc();
	_node->data = data;
	_node->next = NULL;
	TOP tail;
	volatile	LONG64 unq = InterlockedIncrement64(&uniqueIndex);
	while (1)
	{
		tail.uniqueIndex = tailTop->uniqueIndex;
		tail.node = tailTop->node;

		if (tail.node->next == NULL)
		{
			if (NULL == InterlockedCompareExchangePointer((PVOID*)&tail.node->next, _node, nullptr))
			{
				InterlockedIncrement(&usedSize);
				InterlockedCompareExchange128((long long*)tailTop, unq, (long long)tail.node->next, (long long*)&tail);
				break;
			}
		}
		else
			InterlockedCompareExchange128((long long*)tailTop, unq, (long long)tail.node->next, (long long*)&tail);
	}
}

template <class T>
void lockFreeQueue<T>::dequeue(T *data)
{
	TOP hTop;
	TOP tTop;
	node* head;
	
	if (InterlockedDecrement(&usedSize) < 0)
	{
		InterlockedIncrement(&usedSize);
		*data = NULL;
		return;
	}

	volatile LONG64 unq = InterlockedIncrement64(&uniqueIndex);
	while (1)
	{
		hTop.uniqueIndex = headTop->uniqueIndex;
		hTop.node = headTop->node;
		tTop.uniqueIndex = tailTop->uniqueIndex;
		tTop.node = tailTop->node;
		
		if (tTop.node->next != NULL)
		{
			InterlockedCompareExchange128((long long*)tailTop, unq, (long long)tTop.node->next, (long long*)&tTop);
		}
		else
		{
			*data = hTop.node->next->data;
			head = hTop.node;
			if (1 == InterlockedCompareExchange128((long long*)headTop,unq, (long long)hTop.node->next, (long long*)&hTop))
			{
				pool->Free(head); 
				break;
			}
		}

	}
}

template <class T>
int lockFreeQueue<T>::getUsedSize(void)
{
	return usedSize;
}

template <class T>
int lockFreeQueue<T>::peek(T* data, int _pos)
{
	if (usedSize < _pos|| !data) return -1;
	TOP head;
	head.uniqueIndex = headTop->uniqueIndex;
	head.node = headTop->node;

	node *tail = tailTop->node;
	node *iter = head.node->next;
	for (int i = 0; i < _pos; i++)
	{
		if (iter == NULL || iter == tail->next)
			return -1;
		iter = iter->next;
	}
	if (iter == NULL)
		return -1;
	if (head.uniqueIndex == headTop->uniqueIndex)
		*data = iter->data;
	return _pos;	
}