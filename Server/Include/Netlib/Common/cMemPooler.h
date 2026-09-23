#pragma once

BEGIN_NETLIB

template <class Type>
class cMemPooler
{
private:
	int	m_iNumberofBlock;
	int	m_iMaximumBlock;
	int	m_iNumberofBlock_32K;
	int m_iMaximum32kBlock;
	BOOL m_bThreadSafe;

#if defined(_list_mempool_)
	std::list<Type*>	m_MemQueue;
	std::list<Type*>	m_MemQueue_Over_32K;
#else
	std::deque<Type*>	m_MemQueue;
	std::deque<Type*>	m_MemQueue_Over_32K;
#endif

	cCriticalSection m_cs;
	cCriticalSection m_cs32k;

private:
	void Create(int nNumofBlock)
	{
		for (int i = 0; i < nNumofBlock; ++i)
		{
			Type* _type = new Type;

			Push(_type);
		}

		m_iNumberofBlock_32K = 0;
	}

	void Destroy()
	{
		Type* pBlock = NULL;
		while ((pBlock = Remove()) != NULL)
		{
			delete pBlock;
		}

		while ((pBlock = Remove_32K()) != nullptr)
		{
			delete pBlock;
		}
	}

	/*void Lock()
	{
		if(m_bThreadSafe)
		{
			m_cs.Lock();
		}
	}

	void Unlock()
	{
		if(m_bThreadSafe)
		{
			m_cs.Unlock();
		}
	}*/

public:
	cMemPooler(int iNumofBlock = 0, int iMaximumBlock = 0, int iMaximum32kBlock = 0, BOOL bThreadSafe = TRUE)
		: m_iNumberofBlock(iNumofBlock),
		m_iMaximumBlock(iMaximumBlock),
		m_iMaximum32kBlock(iMaximum32kBlock),
		m_bThreadSafe(bThreadSafe)
	{
		if(m_iNumberofBlock > 0)
			Create(m_iNumberofBlock);
	}

	~cMemPooler()
	{
		Destroy();
	}

	void CreatePool(int iNumofBlock, int iMaximumBlock = 0, int iMaximum32kBlock = 0, BOOL bThreadSafe = TRUE)
	{
		m_iNumberofBlock = iNumofBlock;
		m_iMaximumBlock = iMaximumBlock;
		m_bThreadSafe = bThreadSafe;
		m_iMaximum32kBlock = iMaximum32kBlock;

		Type* pType = NULL;

		for (int i = 0; i < m_iNumberofBlock; ++i)
		{
			pType = new Type;

			if(pType != NULL)
			{
				Push(pType);
			}
			else
			{
				printf("CreatePool Failed nNumofBlock[%d] nMaximumBlock[%d] bThreadSafe[%d]", iNumofBlock, iMaximumBlock, bThreadSafe);
				return;
			}
		}
	}

	void IncreasePoolSize(const int iMaxPoolSize)
	{
		if(m_iMaximumBlock < iMaxPoolSize)
			m_iMaximumBlock = iMaxPoolSize;
	}

	void DestroyPool()
	{
		Destroy();
	}

	Type* Pop()
	{
		Type* pBlock = NULL;

		// Lock empty검사하고 들어가서 있다고 판단된뒤에
		// 뒤쪽에서 front에서 큐가 비어있는 경우가 있다..
		// Lock은 이곳에 위치해야 한다.
		//Lock();
		cCSLock cslock(&m_cs, m_bThreadSafe);

		if(m_MemQueue.empty())
		{
			if(m_iNumberofBlock < m_iMaximumBlock)
			{
				++m_iNumberofBlock;
				pBlock = new Type;
			}
			//Unlock();
			return (pBlock);
		}

		pBlock = m_MemQueue.front();

#if defined(_list_mempool_)
		m_MemQueue.pop_front();
#else
		m_MemQueue.pop_front();
#endif

		//Unlock();
		return (pBlock);
	}

	Type* Remove()
	{
		//Lock();
		cCSLock cslock(&m_cs, m_bThreadSafe);

		if(m_MemQueue.empty())
		{
			//Unlock();
			return (NULL);
		}

		Type* pBlock = m_MemQueue.front();
#if defined(_list_mempool_)
		m_MemQueue.pop_front();
#else
		m_MemQueue.pop_front();
#endif
		//Unlock();
		return (pBlock);
	}

	void Push(Type* pBlock)
	{
		cCSLock cslock(&m_cs, m_bThreadSafe);
		//Lock();

#ifdef _list_mempool_
		m_MemQueue.push_back(pBlock);
#else
		m_MemQueue.push_back(pBlock);
#endif

		//Unlock();
	}

	void PushFront(Type* pBlock)
	{
		//Lock();
		cCSLock cslock(&m_cs, m_bThreadSafe);

#ifdef _list_mempool_
		m_MemQueue.push_front(pBlock);
#else
		m_MemQueue.push_front(pBlock);
#endif

		//Unlock();
	}

	size_t GetRemainPoolCnt()
	{
		return m_MemQueue.size();
	}

	int GetMaxPoolCnt()
	{
		return m_iMaximumBlock;
	}

	int GetCurrentPoolCnt()
	{
		return m_iNumberofBlock;
	}

	size_t GetRemainPool_32K_Cnt()
	{
		return m_MemQueue_Over_32K.size();
	}

	int GetCurrentPool_32K_Cnt()
	{
		return m_iNumberofBlock_32K;
	}

	Type* Pop_32K(const int BufferSize = G_NET_BUFFER_SIZE_64K)
	{
		Type* pBlock = NULL;

		// Lock empty검사하고 들어가서 있다고 판단된뒤에
		// 뒤쪽에서 front에서 큐가 비어있는 경우가 있다..
		// Lock은 이곳에 위치해야 한다.
		//Lock();
		cCSLock cslock(&m_cs32k, m_bThreadSafe);

		if(m_MemQueue_Over_32K.empty())
		{
			if (m_iNumberofBlock_32K < m_iMaximum32kBlock)
			{
				++m_iNumberofBlock_32K;
				pBlock = new Type(BufferSize);
			}

			//Unlock();
			return (pBlock);
		}

		pBlock = m_MemQueue_Over_32K.front();

#if defined(_list_mempool_)
		m_MemQueue_Over_32K.pop_front();
#else
		m_MemQueue_Over_32K.pop_front();
#endif

		//Unlock();
		return (pBlock);
	}

	Type* Remove_32K()
	{
		//Lock();
		cCSLock cslock(&m_cs32k, m_bThreadSafe);

		if(m_MemQueue_Over_32K.empty())
		{
			//Unlock();
			return (NULL);
		}

		Type* pBlock = m_MemQueue_Over_32K.front();
#if defined(_list_mempool_)
		m_MemQueue_Over_32K.pop_front();
#else
		m_MemQueue_Over_32K.pop_front();
#endif
		//Unlock();
		return (pBlock);
	}

	void Push_32K(Type* pBlock)
	{
		//Lock();
		cCSLock cslock(&m_cs32k, m_bThreadSafe);

#ifdef _list_mempool_
		m_MemQueue_Over_32K.push_back(pBlock);
#else
		m_MemQueue_Over_32K.push_back(pBlock);
#endif

		//Unlock();
	}

	/*Type* Create_32K(const int BufferSize)
	{
		Type* pBlock = nullptr;

		if (m_iNumberofBlock_32K >= m_iMaximum32kBlock)
			return pBlock;

		Lock();

		pBlock = new Type(BufferSize);

#ifdef _list_mempool_
		m_MemQueue_Over_32K.push_back(pBlock);
#else
		m_MemQueue_Over_32K.push_back(pBlock);
#endif

		++m_iNumberofBlock_32K;

		Unlock();

		return pBlock;
	}*/

	void CreateWithIncreasingBlockCount()
	{
		//Lock();
		cCSLock cslock(&m_cs, m_bThreadSafe);

		Type* pBlock = new Type();

		m_MemQueue.push_back(pBlock);

		m_iNumberofBlock++;

		//Unlock();
	}
};

END_NETLIB