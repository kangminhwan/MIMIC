#pragma once

BEGIN_NETLIB

template<typename T>
class cPool
{
private:
	typedef struct _Node
	{
		T m_NodeItem;
		_Node* m_pNextNode;

	}NODE, *PNODE;

	PNODE m_pFirst;
	NetLib::cSRWLock_CriticalSection m_Lock;

	int m_nCurItemNumber;
	int m_nCreateItem;
	int m_nMaxItem;

public:
	T* Pop()
	{
		NetLib::cUnionLock Lock(&m_Lock, FALSE);

		if (m_pFirst == nullptr)
		{
			if (m_nCreateItem >= m_nMaxItem)
			{
				return nullptr;
			}

			++m_nCreateItem;
			PNODE pNewNode = new NODE();
			return &pNewNode->m_NodeItem;
		}
		else
		{
			--m_nCurItemNumber;
			PNODE pOldNode = m_pFirst;
			m_pFirst = pOldNode->m_pNextNode;
			return &pOldNode->m_NodeItem;
		}
	}

	void Push(T* pObject)
	{
		if (pObject == nullptr)
		{
			return;
		}

		NetLib::cUnionLock Lock(&m_Lock, FALSE);

		// char const volatile 로 변경 안하고 바로 ULONG_PTR로 변경할경우 
		// convert가 안되는 타입이 있습니다. 그걸 방지하기 위해
		// char type으로 변경을 먼저 합니다~
		PNODE pNode = reinterpret_cast<PNODE>(reinterpret_cast<char*>(pObject) - reinterpret_cast<ULONG_PTR>(&reinterpret_cast<char const volatile&>(reinterpret_cast<PNODE>(0)->m_NodeItem)));
		pNode->m_pNextNode = m_pFirst;
		m_pFirst = pNode;
		++m_nCurItemNumber;
	}

public:
	void Create(int nCreateItem, int nMaxItem)
	{
		if (nCreateItem > nMaxItem)
		{
			nCreateItem = nMaxItem;
		}

		if (m_nCreateItem > 0 ||
			m_nMaxItem > 0)
		{
			if (m_nMaxItem < nMaxItem)
			{
				m_nMaxItem = nMaxItem;
			}
			return;
		}

		for (int n = 0; n < nCreateItem; ++n)
		{
			PNODE pNewNode = new NODE();
			pNewNode->m_pNextNode = m_pFirst;
			m_pFirst = pNewNode;
			++m_nCreateItem;
			++m_nCurItemNumber;
		}

		m_nMaxItem = nMaxItem;
	}

private:
	void Destroy()
	{
		while (m_pFirst)
		{
			PNODE pOldNode = m_pFirst;
			m_pFirst = m_pFirst->m_pNextNode;
			delete pOldNode;
		}

		m_nCurItemNumber = 0;
		m_nCreateItem = 0;
		m_nMaxItem = 0;
	}

public:
	cPool() :
		m_pFirst(nullptr),
		m_nCurItemNumber(0),
		m_nCreateItem(0),
		m_nMaxItem(0)
	{

	}

	~cPool()
	{
		Destroy();
	}
};

END_NETLIB