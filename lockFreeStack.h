#pragma once

#include "memoryPool.h"

template <class T>
class lockFreeStack
{
private:
	struct node
	{
		node()
		{
			next = NULL;
		}
		node *next;
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
		__int64 uniqueIndex;
	};

	TOP *topNode;
	LONG usedSize;

	memoryPool<node> *pool;
	volatile LONG64 uniqueIndex;

private:
	void clear();

public:
	lockFreeStack();
	~lockFreeStack();

	void pop(T *data);
	void push(T data);

	int getUsedSize(void);
};

template <class T>
lockFreeStack<T>::lockFreeStack()
{
	usedSize = 0;
	topNode = new TOP;
	pool = new memoryPool<node>;
	uniqueIndex = 0;
}

template <class T>
lockFreeStack<T>::~lockFreeStack()
{
	clear();
	delete topNode;
	delete pool;
}

template <class T>
void lockFreeStack<T>::clear()
{
	node *head = topNode->node;
	node *next;
	while (head)
	{
		next = head->next;
		pool->Free(head);
		head = next;
	
		InterlockedDecrement(&usedSize);
	}
}

template <class T>
void lockFreeStack<T>::push(T data)
{
	InterlockedIncrement64(&uniqueIndex);
	node *node = pool->Alloc();
	node->data = data;
	TOP top;

	do
	{
		top.uniqueIndex = topNode->uniqueIndex;
		top.node = topNode->node;
		node->next = top.node;
	} while (0 == InterlockedCompareExchange128((long long*)topNode, uniqueIndex, (long long)node, (long long*)&top));
	InterlockedIncrement(&usedSize);
}

template <class T>
void lockFreeStack<T>::pop(T *data)
{
	node *nodeNext;
	TOP top;
	node *node;
	InterlockedIncrement64(&uniqueIndex);
	if (InterlockedDecrement(&usedSize) < 0)
	{
	alloc:
		InterlockedIncrement(&usedSize);
		data = NULL;
		return;
	}
	else
	{
		do
		{
			top.uniqueIndex = topNode->uniqueIndex;
			top.node = topNode->node;
			nodeNext = top.node->next;
			if (top.node == nullptr)
				goto alloc;
		} while (0 == InterlockedCompareExchange128((long long*)topNode, uniqueIndex, (long long)nodeNext, (long long*)&top));
		*data = (top.node->data);
		node = top.node;
		pool->Free(node);	
	}
}

template <class T>
int lockFreeStack<T>::getUsedSize(void)
{
	return usedSize;
}