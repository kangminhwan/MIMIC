#include "cLoginEventManager.h"
#include "Query.h"
#include "../Include/Netlib/Queue/cLogQueue.h"
#include <chrono>

NetLib::cCriticalSection cLoginEventManager::s_CriticalSection;

cLoginEventManager::cLoginEventManager()
    : m_isInitialized(false)
{
}

cLoginEventManager::~cLoginEventManager()
{
}

void cLoginEventManager::Initialize()
{
    bool needInit = false;
    
    s_CriticalSection.Lock();
    if (!m_isInitialized)
    {
        needInit = true;
    }
    s_CriticalSection.Unlock();
    
    if (needInit)
    {
        // DB 호출은 락 밖에서 수행
        std::vector<PmNet::DropDetail> tempEventList;
        bool result = QueryManager::GetLoginRewardList(tempEventList);
        
        s_CriticalSection.Lock();
        if (result)
        {
            m_loginEventList = std::move(tempEventList);
        }
        m_lastUpdateTime = std::chrono::steady_clock::now();
        m_isInitialized = true;
        size_t eventCount = m_loginEventList.size();
        s_CriticalSection.Unlock();
        
        // 로깅은 락 해제 후 수행
        NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(
            LOG_GRADE::LOG_INFO, 
            "cLoginEventManager::Initialize - Login event cache initialized with %zu events", 
            eventCount
        );
    }
}

bool cLoginEventManager::UpdateLoginEventList()
{
    // DB 호출은 락 밖에서 수행
    std::vector<PmNet::DropDetail> tempEventList;
    bool result = QueryManager::GetLoginRewardList(tempEventList);
    
    // 최소한의 락 시간으로 캐시 업데이트
    s_CriticalSection.Lock();
    if (result)
    {
        m_loginEventList = std::move(tempEventList);
        m_lastUpdateTime = std::chrono::steady_clock::now();
    }
    size_t eventCount = m_loginEventList.size();
    s_CriticalSection.Unlock();
    
    // 로깅은 락 해제 후 수행
    if (result)
    {
        NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(
            LOG_GRADE::LOG_INFO, 
            "cLoginEventManager::UpdateLoginEventList - Cache updated with %zu events", 
            eventCount
        );
    }
    else
    {
        NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(
            LOG_GRADE::LOG_CRI, 
            "cLoginEventManager::UpdateLoginEventList - Failed to update cache"
        );
    }
    
    return result;
}

std::vector<PmNet::DropDetail> cLoginEventManager::GetLoginEventList()
{
    bool needRefresh = false;
    std::vector<PmNet::DropDetail> result;
    
    // 빠른 체크: 캐시 상태 확인
    s_CriticalSection.Lock();
    if (!m_isInitialized || !IsCacheValid())
    {
        needRefresh = true;
    }
    else
    {
        result = m_loginEventList;  // 유효한 캐시 복사
    }
    s_CriticalSection.Unlock();
    
    // 필요한 경우에만 DB에서 새로 로드 (락 밖에서)
    if (needRefresh)
    {
        std::vector<PmNet::DropDetail> tempEventList;
        bool loadResult = QueryManager::GetLoginRewardList(tempEventList);
        
        s_CriticalSection.Lock();
        if (loadResult)
        {
            m_loginEventList = std::move(tempEventList);
            m_lastUpdateTime = std::chrono::steady_clock::now();
            m_isInitialized = true;
        }
        result = m_loginEventList;  // 결과 복사 (성공/실패 관계없이 현재 캐시)
        s_CriticalSection.Unlock();
    }
    
    return result;
}

bool cLoginEventManager::IsCacheValid() const
{
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::minutes>(now - m_lastUpdateTime);
    return duration.count() < 2; // Valid for 2 minutes (buffer for 1-minute timer)
}

void cLoginEventManager::RefreshCache()
{
    // DB 호출은 락 밖에서 수행
    std::vector<PmNet::DropDetail> tempEventList;
    bool result = QueryManager::GetLoginRewardList(tempEventList);
    
    // 최소한의 락 시간으로 캐시 업데이트
    s_CriticalSection.Lock();
    if (result)
    {
        m_loginEventList = std::move(tempEventList);
        m_lastUpdateTime = std::chrono::steady_clock::now();
    }
    s_CriticalSection.Unlock();
}

bool cLoginEventManager::LoadFromDatabase()
{
    std::vector<PmNet::DropDetail> tempEventList;
    
    bool result = QueryManager::GetLoginRewardList(tempEventList);
    if (result)
    {
        m_loginEventList = std::move(tempEventList);
        printf("cLoginEventManager::LoadFromDatabase - Loaded %zu login events\n", m_loginEventList.size());
    }
    
    return result;
}