#pragma once

BEGIN_NETLIB

template<typename K, class V, int MAXLEVEL>
class skiplist_node : __POSITION
{
public:
	K				key;
	V				value;
	skiplist_node<K, V, MAXLEVEL>* forwards[MAXLEVEL + 1];

public:
	skiplist_node()
	{
		for (int n = 0; n < MAXLEVEL; ++n)
		{
			forwards[n] = nullptr;
		}
	}

	skiplist_node(K searchKey) : 
		key(searchKey)
	{
		for (int n = 0; n < MAXLEVEL; ++n)
		{
			forwards[n] = nullptr;
		}
	}

	skiplist_node(K searchKey, V val) :
		key(searchKey), 
		value(val)
	{
		for (int n = 0; n < MAXLEVEL; ++n) 
		{
			forwards[n] = nullptr;
		}
	}

	void Clear()
	{
		memset(this, 0x00, sizeof(*this));
	}

	~skiplist_node() {}
};

template<typename K, class V, int MAXLEVEL = 16>
class cSkipList
{
public:
	typedef K	KeyType;
	typedef V	ValueType;
	typedef skiplist_node<K, V, MAXLEVEL> NodeType;

protected:
	K m_minKey;
	K m_maxKey;
	int max_curr_level;
	skiplist_node<K, V, MAXLEVEL>* m_pHeader;
	skiplist_node<K, V, MAXLEVEL>* m_pTail;

	cPool<NodeType> m_pool;

protected:
	int randomlevel()
	{
		int level = 0;
		int nrandom = rand();
		while ((nrandom & 0xFFFF) < (0.25 * 0xFFFF) && level < (MAXLEVEL - 1))
		{
			++level;
			nrandom = rand();
		}

		return level;
	}

public:
	void insert(KeyType searchKey, ValueType newValue)
	{
		NodeType* pUpdate[MAXLEVEL];
		NodeType* pCurNode = m_pHeader;

		//	추가는 최상단 리스트에서부터 이루어집니다.
		//	최대 레벨 부터 해서 최소레벨 까지 줄어듭니다.
		//	인서트 되는 키 값보다 작은 위치를 찾습니다.
		//	높이 를 전체 탐색 합니다.
		for (int level = max_curr_level; level >= 0; --level)
		{
			//	searchKey 값이 현재 레벨의 key값보다 크다면
			while (pCurNode->forwards[level]->key < searchKey)
			{
				//	전진 합니다.
				pCurNode = pCurNode->forwards[level];
			}

			//	해당 searchKey값이 있어야할 위치를 pUpdate[level]에 담아 둡니다.
			pUpdate[level] = pCurNode;
		}

		//	현재 노드의 최소 레벨을 가져옵니다.
		//	이 위치가 Key값이 들어갈 위치 입니다.
		pCurNode = pCurNode->forwards[0];
		if (pCurNode->key == searchKey)
		{
			pCurNode->value = newValue;
		}
		else
		{
			//	스킵 리스트가 Randomized algorithm이라서 레벨을 랜덤으로 뽑아 옵니다.
			//	뽑아온 레벨이 현재 레벨 보다 크다면 pUpdate[level] 에 m_pHeader 값을 넣습니다.
			//	새로운 레벨이 등장 한거니 첫 시작인 Header 가 필요합니다.
			int newlevel = randomlevel();
			if (newlevel > max_curr_level)
			{
				//	새로운 레벨이 시작 했습니다.
				for (int level = max_curr_level + 1; level <= newlevel; ++level)
				{
					//	새로운 레벨에 header 값으로 셋팅 합니다.
					pUpdate[level] = m_pHeader;
				}

				//	현재 맥스 레벨을 새로운 레벨로 교체 합니다.
				max_curr_level = newlevel;
			}

			//	추가해야할 값이 들어가야할 자리가 나왔습니다.
			//	전부다 추가 해줍니다.
			pCurNode = m_pool.Pop();
			pCurNode->key = searchKey;
			pCurNode->value = newValue;
			for (int level = 0; level <= max_curr_level; ++level)
			{
				pCurNode->forwards[level] = pUpdate[level]->forwards[level];
				pUpdate[level]->forwards[level] = pCurNode;
			}
		}
	}

	bool erase(KeyType searchKey)
	{
		bool bRet = false;
		skiplist_node<K, V, MAXLEVEL>* pUpdate[MAXLEVEL];
		NodeType* pCurNode = m_pHeader;

		//	삭제는 최상단 리스트에서부터 이루어집니다.
		//	최대 레벨 부터 해서 최소레벨 까지 줄어듭니다.
		//	삭제가 되는 키 값보다 작은 위치를 찾습니다.
		//	높이 를 전체 탐색 합니다.
		for (int level = max_curr_level; level >= 0; --level)
		{
			//	searchKey 값이 현재 레벨의 key값보다 크다면
			while (pCurNode->forwards[level]->key < searchKey)
			{
				//	전진 합니다.
				pCurNode = pCurNode->forwards[level];
			}

			//	해당 searchKey값이 있어야할 위치를 pUpdate[level]에 담아 둡니다.
			pUpdate[level] = pCurNode;
		}

		//	현재 노드의 최소 레벨을 가져옵니다.
		//	삭제를 해야할 key 값하고 같으면 삭제 루틴을 시작합니다.
		pCurNode = pCurNode->forwards[0];
		if (pCurNode->key == searchKey)
		{
			//	최소 레벨 부터 최대 레벨 까지 검색 합니다.
			for (int level = 0; level <= max_curr_level; ++level)
			{
				if (pUpdate[level]->forwards[level] != pCurNode)
				{
					break;
				}

				//	찾았으면 삭제가 될 노드의 다음 노드를 이어줍니다.
				pUpdate[level]->forwards[level] = pCurNode->forwards[level];
			}

			//	삭제해야할 노드를 삭제 합니다.
			pCurNode->Clear();
			m_pool.Push(pCurNode);

			bRet = true;

			if (m_pHeader->forwards[max_curr_level] == nullptr)
			{
				m_pHeader->forwards[max_curr_level] = m_pTail;
			}

			//	레벨을 검색합니다.
			//	레벨을 줄일지 말지 검색합니다.
			//	이 루틴이 안들어가면 Skip List는 검색 / 삽입 / 삭제 루틴이 O(n)이 되어버립니다.
			while (max_curr_level > 0 && m_pHeader->forwards[max_curr_level]->key == m_maxKey)
			{
				--max_curr_level;
			}
		}

		return bRet;
	}

	const NodeType* find(KeyType searchKey)
	{
		NodeType* pCurNode = m_pHeader;
		for (int level = max_curr_level; level >= 0; --level)
		{
			while (pCurNode->forwards[level]->key < searchKey)
			{
				pCurNode = pCurNode->forwards[level];
			}
		}

		pCurNode = pCurNode->forwards[0];
		if (pCurNode->key == searchKey)
		{
			return pCurNode;
		}

		return nullptr;
	}

	bool empty() const
	{
		//	최하위 레벨이 Tail 과 같으면 데이터가 없는겁니다.
		return (m_pHeader->forwards[0] == m_pTail);
	}

public:
	POSITION GetStartPosition()
	{
		if (empty())
		{
			return NULL;
		}

		return reinterpret_cast<POSITION>(m_pHeader->forwards[0]);
	}

	V& GetValueAt(POSITION pos)
	{
		NodeType* pNode = static_cast<NodeType*>(pos);

		return pNode->value;
	} 

	POSITION GetNext(POSITION& pos)
	{
		if (pos == nullptr)
		{
			return nullptr;
		}

		NodeType* pNode = static_cast<NodeType*>(pos);
		NodeType* pNext = pNode->forwards[0];

		if (pNext != m_pTail)
		{
			pos = reinterpret_cast<POSITION>(pNext);
		}
		else
		{
			pos = nullptr;
		}

		return pos;
	}

	POSITION Lookup(KeyType searchKey)
	{
		NodeType* pCurNode = m_pHeader;
		for (int level = max_curr_level; level >= 0; --level)
		{
			while (pCurNode->forwards[level]->key < searchKey)
			{
				pCurNode = pCurNode->forwards[level];
			}
		}

		if (pCurNode != nullptr)
		{
			pCurNode = pCurNode->forwards[0];

			if (pCurNode != nullptr &&
				pCurNode != m_pTail &&
				pCurNode->key == searchKey)
			{
				return reinterpret_cast<POSITION>(pCurNode);
			}
		}

		return nullptr;
	}

	void RemoveAtPos(POSITION pos)
	{
		if (pos == nullptr)
		{
			return;
		}

		NodeType* pNode = static_cast<NodeType*>(pos);

		erase(pNode->key);

	}

	void RemoveAll()
	{
		if (m_pHeader != nullptr)
		{
			NodeType* curNode = m_pHeader->forwards[0];
			while (curNode != m_pTail)
			{
				NodeType* tempNode = curNode;
				curNode = curNode->forwards[0];
				m_pool.Push(tempNode);
			}

			m_pHeader->key = m_minKey;
			m_pTail->key = m_maxKey;

			for (int n = 0; n < MAXLEVEL; ++n)
			{
				m_pHeader->forwards[n] = m_pTail;
			}
		}
	}

public:
	void Init(KeyType minKey, KeyType maxKey, int nMaxPoolItem)
	{
		m_minKey = minKey;
		m_maxKey = maxKey;

		m_pool.Create(0, nMaxPoolItem);

		m_pHeader = m_pool.Pop();
		m_pTail = m_pool.Pop();

		m_pHeader->key = m_minKey;
		m_pTail->key = m_maxKey;

		for (int n = 0; n < MAXLEVEL; ++n)
		{
			m_pHeader->forwards[n] = m_pTail;
		}
	}

public:
	cSkipList() : m_pHeader(nullptr), m_pTail(nullptr), max_curr_level(0) { }

	cSkipList(KeyType minKey, KeyType maxKey, int nMaxPoolItem) :
		m_pHeader(nullptr),
		m_pTail(nullptr),
		max_curr_level(0),
		m_minKey(minKey),
		m_maxKey(maxKey)
	{
		m_pool.Create(0, nMaxPoolItem);

		m_pHeader = m_pool.Pop();
		m_pTail = m_pool.Pop();

		m_pHeader->key = m_minKey;
		m_pTail->key = m_maxKey;

		for (int n = 0; n < MAXLEVEL; ++n)
		{
			m_pHeader->forwards[n] = m_pTail;
		}
	}

	virtual ~cSkipList()
	{
		if (m_pHeader != nullptr)
		{
			NodeType* curNode = m_pHeader->forwards[0];
			while (curNode != m_pTail)
			{
				NodeType* tempNode = curNode;
				curNode = curNode->forwards[0];
				m_pool.Push(tempNode);
			}

			delete m_pHeader;
			delete m_pTail;
		}
	}

};

END_NETLIB