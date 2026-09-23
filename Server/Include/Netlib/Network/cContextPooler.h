#pragma once
#include "../Common/Netlib.h"
#include "../IOCP/cIocpContext.h"

BEGIN_NETLIB

class cCommandQueue;
class cContextPooler
{
private:

	friend class cNetWork;

private:

//	http://pigbrain.github.io/datastructure/2015/07/12/SkipList_on_DataStructure
#ifdef USE_CONTEXTPOOLER_SKIP_LIST_ALGORITHM

#pragma region Skip List Algorithm

#pragma region Skip List Enum

	enum event_erase
	{
		ee_thread_erase_no,
		ee_thread_erase_ok,
		ee_push_ok,
		ee_push_no,
	};

#pragma endregion

#pragma region Skip List Node

	//	Context를 효율적으로 관리 하기 위한
	struct contextskiplistnode : public __POSITION
	{
		LONG m_refcount;

		UINT Entity;

		cIocpContext* pContext;

		//	전체 탐색을 위한 pointer는 0번 스레드전용은 1번부터입니다.
		//	0번이 삭제가 되면 완전 사라지는 것이라고 보면 됩니다.
		contextskiplistnode* forwards[MAX_COMMAND_THREAD][MAX_LEVEL + 1];	//	해당 스레드용 pointer

		contextskiplistnode()
		{
			memset(this, 0x00, sizeof(*this));
		}

		contextskiplistnode(UINT _uiEntity) :
			Entity(_uiEntity)
		{
			memset(forwards, 0x00, sizeof(contextskiplistnode*) * FORWARDS_MEMORY_SET);
		}

		contextskiplistnode(UINT _uiEntity, cIocpContext* _pContext) :
			Entity(_uiEntity),
			pContext(_pContext)
		{
			memset(forwards, 0x00, sizeof(contextskiplistnode*) * FORWARDS_MEMORY_SET);
		}

		void clear()
		{
			memset(this, 0x00, sizeof(*this));
		}

		~contextskiplistnode() {}
	};

#pragma endregion

#pragma region Skip List Container

	//	http://pigbrain.github.io/datastructure/2015/07/12/SkipList_on_DataStructure
	class contextskiplist
	{
	private:
		UINT m_minEntity;
		UINT m_maxEntity;
		int max_cur_level[MAX_COMMAND_THREAD];

		contextskiplistnode* m_pHeader;
		contextskiplistnode* m_pTail;

		NetLib::cPool<contextskiplistnode> m_pool;

		NetLib::cSRWLock_CriticalSection m_srwlock;

		LONG m_lLiveContextCount;

	private:
		//	스킵 리스트는 level 이 커질려면 random 에서 level 을 뽑아 와야합니다.
		//	MAX_LEVEL - 1 이상보다 커지면 안됩니다.
		int randomlevel()
		{
			int level = 0;
			while ((rand() & 0xFFFF) < (0.25 * 0xFFFF) && level < (MAX_LEVEL - 1))
			{
				++level;
			}

			return level;
		}

	public:
		//	insert가 될때는 0번 하고 파라미터로 넘어온 ThreadIndex로 Update합니다.
		bool insert(NetLib::cIocpContext* pContext, thread_type e_thread_type, UINT uithreadindex)
		{
			if (uithreadindex >= MAX_COMMAND_THREAD	||
				pContext == nullptr)
			{
				return false;
			}

			bool result = true;
			if (e_thread_type == thread_type::ty_all)
			{
				//	ALL Container에 inert가 될 때는 Lock을 걸어야합니다.
				m_srwlock.AcquireLockExclusive();
				if (nullptr == oninsert(pContext, thread_type::ty_all))
					result = false;
				m_srwlock.ReleaseLockExclusive();
			}

			//	Thread Container에 insert가 될 때는  Lock을 걸 이유가 없습니다.
			if (nullptr == oninsert(pContext, uithreadindex))
				result = false;
			return result;
		}

		event_erase erase(UINT Entity, thread_type e_thread_type, UINT uithreadindex)
		{
			if (uithreadindex >= MAX_COMMAND_THREAD)
			{
				return event_erase::ee_thread_erase_no;
			}

			event_erase e_event = event_erase::ee_thread_erase_no;

			e_event = onerase(Entity, e_thread_type, false, uithreadindex);

			if (e_event == event_erase::ee_thread_erase_no)
			{
				return e_event;
			}

			if (e_event == event_erase::ee_thread_erase_ok &&
				e_thread_type == thread_type::ty_all)
			{
				//	ALL Container에 erase가 될 때는 Lock을 걸어야합니다.
				m_srwlock.AcquireLockExclusive();
				e_event = onerase(Entity, e_thread_type, true, thread_type::ty_all);
				m_srwlock.ReleaseLockExclusive();
			}

			return e_event;
		}

		//	find를 요청한 스레드에서 Context를 찾습니다.
		//	못찾으면 락을 걸고 All Container에서 찾습니다.
		cIocpContext* find(UINT Entity, UINT uithreadindex)
		{
			if (uithreadindex >= MAX_COMMAND_THREAD)
			{
				return nullptr;
			}

			if (uithreadindex > 0)
			{
				NetLib::cIocpContext* pContext = onfind(Entity, uithreadindex);
				if (pContext != nullptr)
				{
					return pContext;
				}
			}

			//	ALL Container에서는 무조건 Lock을 겁니다.
			//	find 이니까 Shared lock 을 겁니당~
			NetLib::cUnionLock Lock(&m_srwlock, TRUE);
			return onfind(Entity, thread_type::ty_all);
		}

		bool empty(UINT uithreadindex)
		{
			//	All Container가 비어있으면 비어있는것
			return (m_pHeader->forwards[uithreadindex][0] == m_pTail);
		}

		POSITION GetStartPosition(UINT uithreadindex)
		{
			if (empty(uithreadindex))
			{
				return nullptr;
			}

			return reinterpret_cast<POSITION>(m_pHeader->forwards[uithreadindex][0]);
		}

		cIocpContext*& GetValueAt(POSITION pos)
		{
			contextskiplistnode* pnode = static_cast<contextskiplistnode*>(pos);

			return pnode->pContext;
		}

		POSITION GetNext(POSITION& pos, UINT uithreadindex)
		{
			if (pos == nullptr)
			{
				return nullptr;
			}

			contextskiplistnode* pnode = static_cast<contextskiplistnode*>(pos);
			contextskiplistnode* pnext = pnode->forwards[uithreadindex][0];

			if (pnext != m_pTail)
			{
				pos = reinterpret_cast<POSITION>(pnext);
			}
			else
			{
				pos = nullptr;
			}

			return pos;
		}

		//	절대 프로그램 실행중에 실행 호출 하면 안됩니다.
		//	안전이고 머고 하나도 없고 전부다 지워버립니다.
		void RemoveAll()
		{
			if (m_pHeader != nullptr)
			{
				contextskiplistnode* pcurnode = m_pHeader->forwards[0][0];
				while (pcurnode != m_pTail)
				{
					contextskiplistnode* ptempnode = pcurnode;
					pcurnode = pcurnode->forwards[0][0];
					ptempnode->clear();
					m_pool.Push(ptempnode);
				}

				m_pHeader->Entity = m_minEntity;
				m_pTail->Entity = m_maxEntity;

				for (int n = 0; n < MAX_COMMAND_THREAD; ++n)
				{
					for (int level = 0; level <= MAX_LEVEL; ++level)
					{
						m_pHeader->forwards[n][level] = m_pTail;
					}
				}
			}
		}

	private:
		contextskiplistnode* oninsert(NetLib::cIocpContext* pContext, UINT uithreadindex)
		{
			UINT Entity = pContext->GetEntity();

			contextskiplistnode* pupdate[MAX_LEVEL];
			contextskiplistnode* pcurnode = m_pHeader;

			for (int level = max_cur_level[uithreadindex]; level >= 0; --level)
			{
				while (pcurnode->forwards[uithreadindex][level]->Entity < Entity)
				{
					pcurnode = pcurnode->forwards[uithreadindex][level];
				}

				pupdate[level] = pcurnode;
			}

			pcurnode = pcurnode->forwards[uithreadindex][0];
			if (pcurnode->Entity == Entity)
			{
				pcurnode->pContext = pContext;
			}
			else
			{
				int newlevel = randomlevel();
				if (newlevel > max_cur_level[uithreadindex])
				{
					for (int level = max_cur_level[uithreadindex] + 1; level <= newlevel; ++level)
					{
						pupdate[level] = m_pHeader;
					}

					max_cur_level[uithreadindex] = newlevel;
				}

				pcurnode = static_cast<contextskiplistnode*>(pContext->GetPos());

				if (pcurnode == nullptr)
				{
					pcurnode = m_pool.Pop();
					if (pcurnode == nullptr)
						return nullptr;
					pcurnode->Entity = pContext->GetEntity();
					pcurnode->pContext = pContext;
					pContext->SetPos(reinterpret_cast<POSITION>(pcurnode));
				}
				else
				{
					if (pcurnode->Entity != pContext->GetEntity())
					{
						int nn = 10;
					}
				}

				for (int level = 0; level <= max_cur_level[uithreadindex]; ++level)
				{
					pcurnode->forwards[uithreadindex][level] = pupdate[level]->forwards[uithreadindex][level];
					pupdate[level]->forwards[uithreadindex][level] = pcurnode;
				}

				InterlockedIncrement(&pcurnode->m_refcount);

				if (uithreadindex == static_cast<int>(thread_type::ty_all))
				{
					InterlockedIncrement(&m_lLiveContextCount);
				}
			}

			return pcurnode;
		}

		event_erase onerase(UINT Entity, thread_type e_thread_type, bool bPush, UINT uithreadindex)
		{
			event_erase e_erase = event_erase::ee_thread_erase_no;

			contextskiplistnode* pupdate[MAX_LEVEL];
			contextskiplistnode* pcurnode = m_pHeader;

			for (int level = max_cur_level[uithreadindex]; level >= 0; --level)
			{
				while (pcurnode->forwards[uithreadindex][level]->Entity < Entity)
				{
					pcurnode = pcurnode->forwards[uithreadindex][level];
				}

				pupdate[level] = pcurnode;
			}

			pcurnode = pcurnode->forwards[uithreadindex][0];
			if (pcurnode->Entity == Entity)
			{
				//	해당 Context가 있는 Thread에서 삭제를 시도할때 thread_type::ty_all 타입으로 넘겨 줍니다.
				//	삭제 시도 thread type 이 all 이거나 cur 이면 m_refcount 카운트는 무조건 2 여야합니다.
				if (e_thread_type == thread_type::ty_all ||
					e_thread_type == thread_type::ty_cur_thread)
				{
					if (pcurnode->m_refcount > 2)
					{
						return e_erase;
					}
				}

				InterlockedDecrement(&pcurnode->m_refcount);

				if (uithreadindex == static_cast<int>(thread_type::ty_all))
				{
					InterlockedDecrement(&m_lLiveContextCount);
				}

				for (int level = 0; level <= max_cur_level[uithreadindex]; ++level)
				{
					if (pupdate[level]->forwards[uithreadindex][level] != pcurnode)
					{
						break;
					}

					pupdate[level]->forwards[uithreadindex][level] = pcurnode->forwards[uithreadindex][level];
				}

				e_erase = event_erase::ee_thread_erase_ok;

				if (bPush == true)
				{
					pcurnode->clear();
					m_pool.Push(pcurnode);
					e_erase = event_erase::ee_push_ok;
				}

				if (m_pHeader->forwards[uithreadindex][max_cur_level[uithreadindex]] == nullptr)
				{
					m_pHeader->forwards[uithreadindex][max_cur_level[uithreadindex]] = m_pTail;
				}

				while (max_cur_level[uithreadindex] > 0 && m_pHeader->forwards[uithreadindex][max_cur_level[uithreadindex]]->Entity == m_maxEntity)
				{
					--max_cur_level[uithreadindex];
				}
			}

			return e_erase;
		}

		cIocpContext* onfind(UINT Entity, UINT uithreadindex)
		{
			contextskiplistnode* pcurnode = m_pHeader;

			for (int level = max_cur_level[uithreadindex]; level >= 0; --level)
			{
				while (pcurnode->forwards[uithreadindex][level]->Entity < Entity)
				{
					pcurnode = pcurnode->forwards[uithreadindex][level];
				}
			}

			pcurnode = pcurnode->forwards[uithreadindex][0];
			if (pcurnode->Entity == Entity)
			{
				return pcurnode->pContext;
			}

			return nullptr;
		}

	public:
		void Init(int nMaxPoolItem)
		{
			//	Header 하고 Tail 때문에 +2를 해서 Max치를 만들어 둡니다.
			m_pool.Create(0, nMaxPoolItem + 2);

			m_pTail = m_pool.Pop();
			m_pTail->Entity = m_maxEntity;
			m_pHeader = m_pool.Pop();
			m_pHeader->Entity = m_minEntity;
				
			for (int n = 0; n < MAX_COMMAND_THREAD; ++n)
			{
				for (int level = 0; level <= MAX_LEVEL; ++level)
				{
					m_pHeader->forwards[n][level] = m_pTail;
				}
			}
		}

		LONG GetLiveContext() { return m_lLiveContextCount; }

	public:
		contextskiplist() :
			m_pTail(nullptr),
			m_pHeader(nullptr),
			m_minEntity(0),
			m_maxEntity(0xffffffff),
			m_lLiveContextCount(0)
		{
		}

		~contextskiplist()
		{
			RemoveAll();

			m_pool.Push(m_pTail);
			m_pool.Push(m_pHeader);
		}
	};

#pragma endregion

#pragma endregion

#endif

private:
	cMemPooler<cIocpContext>	m_cIocpContextPool;						// 정상적인 클라이언트풀

#ifndef USE_CONTEXTPOOLER_SKIP_LIST_ALGORITHM

	cSRWLock_CriticalSection m_PendingContextLock;									//
	ATL::CAtlList<cIocpContext*> m_atlListPendingContext;					// 종료되었을때 잠시동안 들고있을 List

	cSRWLock_CriticalSection m_ConnectedtableLock;								//Table 의 Insert와 Remove의 
	ATL::CAtlMap<UINT, cIocpContext*> m_atlmapConnectedContextTable;	//현재 Connect되어있는 Context들입니다.

#else

	//	현재 Connect 되어 있는 Context들 입니다.
	contextskiplist m_contexttable;

#endif

#ifdef USE_TIME_WAIT
	cCriticalSection m_TimeWaitLock;									//	Time Wait 상태의 Context Lock
	ATL::CAtlMap<UINT, cIocpContext*> m_atlmapTimeWaitContextTable;		//	Time Wait 상태의 Context Table
#endif

#if defined(DEVELOPMENT)
	cCriticalSection m_Lock;
	ATL::CAtlMap<UINT,cIocpContext*>	m_atlmapContext;
#endif

#ifdef VIRTUAL_NAGLE_ON_OFF
	BOOL m_bVirtualNagleOnOff;
#endif

public:
	void Init();
	void Destory();

public:
	void Create(int iMaximum);

	size_t  GetCount() { return m_cIocpContextPool.GetRemainPoolCnt(); }

	LONG	GetLiveContextCount() { return m_contexttable.GetLiveContext(); }

	cIocpContext*	Pop();
	void	PushContext(cIocpContext* pContext, bool bTimeOut = false);				//	Skip List Algorithm 에서도 사용	[	구현 중	]

private:
	void	Push(cIocpContext* pContext);
#ifndef USE_CONTEXTPOOLER_SKIP_LIST_ALGORITHM
	void	PushPendingList(cIocpContext* pContext);
public:
		void	CheckPendingList();
private:
#endif

#ifdef USE_TIME_WAIT
	void	PushTimeWait(cIocpContext* pContext);
	bool	RemoveTimeWaitContext(UINT Entity);
#endif

private:
	void PushErrorContext(cIocpContext* pContext);

public:

	bool ConnectContext(NetLib::cIocpContext* pContext, UINT uithreadindex);		//	Skip List Algorithm 에서도 사용	[	구현 완료	]
	bool ChangeCommandContext(NetLib::cIocpContext* pContext, UINT uithreadindex);	//	Skip List Algorithm 에서만 사용	[	구현 완료	]

	cIocpContext* FindConnectedContext(UINT Entity, int nthreadindex);
	NetLib::cSession* FindConnectedSession(UINT Entity, int nthreadindex);
	NetLib::cCommandQueue* FindCommandQueueArray(UINT Entity, int nthreadindex);

	void AliveContextCheck(UINT uithreadindex);					//	Skip List Algorithm 에서도 사용	[	구현 완료	]
	void SlotServerAliveContextCheck(UINT uithreadindex);
	POSITION GetStartPosition(UINT uithreadindex);				// 쓰레드 별로 Context Array 시작포인트를 반환 합니다.
	POSITION GetNextPosition(POSITION& prevPos, UINT uithreadindex);
	NetLib::cIocpContext* GetValue(POSITION pos);

private:	
	//딜레이가 처음걸린애만 일단 짜르고 나머지는 기회를 좀더준다.
	bool b_Check_Tick = false;

public:
	bool GetCheckTick() { return b_Check_Tick; }
	void SetCheckTick(bool _b_Check_Tick) { b_Check_Tick = _b_Check_Tick; }

#ifdef USE_CONTEXTPOOLER_SKIP_LIST_ALGORITHM
private:
	void AliveClientContext(cIocpContext* pContext, ULONGLONG ulTick, thread_type etype, UINT uithreadindex);
	void SlotServerAliveClientContext(cIocpContext* pContext, ULONGLONG ulTick, thread_type etype, UINT uithreadindex);
	void AliveServerContext(cIocpContext* pContext, ULONGLONG ulTick, thread_type etype, UINT uithreadindex);
#else
public:
	bool RemoveContext(UINT Entity);
#endif

public:
	cContextPooler();
	virtual ~cContextPooler();
};

END_NETLIB