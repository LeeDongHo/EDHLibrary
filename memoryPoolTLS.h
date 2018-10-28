#pragma once

template <class T>
class memoryTLS
{
private:
	class chunk;
	struct node
	{
		chunk* chunkPtr;
		unsigned __int64 check;
		T data;
	};

	class chunk
	{
	private:
		node* chunkArr;
		unsigned short allocCount;
		LONG freeCount;
		memoryTLS<T> *tls;
	public:
		unsigned short chunkSize;

	public:
		chunk()
		{
			chunkSize = 0;
			allocCount = 0;
			freeCount = 0;
		}
		
		~chunk()
		{
			delete[] chunkArr;
		}

		void Constructor(memoryTLS<T> *_this,unsigned short _chunkSize, unsigned __int64 key)
		{
			tls = _this;
			chunkSize = _chunkSize;
			chunkArr = new node[chunkSize];
			for (int i = 0; i < chunkSize; i++)
			{
				chunkArr[i].chunkPtr = this;
				chunkArr[i].check = key;
			}
		}

		T* Alloc(void)
		{
			T* data = &chunkArr[allocCount].data;
			allocCount++;
			if (allocCount == chunkSize)
				tls->chunkAlloc();

			return data;
		}

		bool Free(void)
		{
			if (chunkSize == InterlockedIncrement(&freeCount))
			{
				InterlockedDecrement(&tls->usedCount);
				tls->chunkPool.Free(this);
				return false;
			}
			else
				return true;
		}

		void clear()
		{
			allocCount = 0;
			freeCount = 0;
		}
	};
	
private:
	friend class chunk;
	memoryPool<chunk> chunkPool;
	unsigned short chunkSize;
	LONG allocCount;
	LONG usedCount;

	DWORD poolIndex;
	unsigned __int64 key;

	chunk* chunkAlloc()
	{
		InterlockedIncrement(&usedCount);
		chunk* dummy = chunkPool.Alloc();
		if (dummy->chunkSize == 0)
		{
			InterlockedIncrement(&allocCount);
			dummy->Constructor(this,chunkSize,key);
		}
		dummy->clear();
		TlsSetValue(poolIndex, dummy);
		return dummy;
	}

public:
	memoryTLS(unsigned short _val, unsigned __int64 _key)
	{
		allocCount = 0;
		usedCount = 0;
		chunkSize = _val;
		poolIndex = TlsAlloc();
		key = _key;
	}

	~memoryTLS()
	{
		TlsFree(poolIndex);
	}

	T* Alloc()
	{
		chunk* dummy = (chunk*)TlsGetValue(poolIndex);
		if (!dummy)
			dummy = chunkAlloc();
		
		T* data = dummy->Alloc();

		return data;
	}

	void Free(T* _str)
	{
		node* dummy = (node*)((char*)_str - 16);
		chunk* dummyChunk = dummy->chunkPtr;
		dummyChunk->Free();

	}

	void clear()
	{
		chunk* dummy = (chunk*)TlsGetValue(poolIndex);
		if (dummy)
		{
			chunkPool.Free(dummy);
			InterlockedDecrement(&usedCount);
		}
		return;
	}

	int getAllocCount()
	{
		return allocCount;
	}

	int getUsedCount()
	{
		return usedCount;
	}
};

