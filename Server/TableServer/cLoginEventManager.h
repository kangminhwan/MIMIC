#pragma once

#include "../Include/Netlib/Common/cSingleton.h"
#include "../Include/Netlib/Common/cCriticalSection.h"
#include "../../ProtocolBuffer/cpp/PmNet.pb.h"
#include <vector>
#include <chrono>

class cLoginEventManager
{
private:
    static NetLib::cCriticalSection s_CriticalSection;
    
    std::vector<PmNet::DropDetail> m_loginEventList;
    std::chrono::steady_clock::time_point m_lastUpdateTime;
    bool m_isInitialized;

public:
    cLoginEventManager();
    ~cLoginEventManager();

    // Initialize the manager
    void Initialize();

    // Update login event list from database (called by timer)
    bool UpdateLoginEventList();

    // Get cached login event list
    std::vector<PmNet::DropDetail> GetLoginEventList();

    // Check if cache is valid (updated within last minute)
    bool IsCacheValid() const;

    // Force refresh cache
    void RefreshCache();

private:
    // Load events from database
    bool LoadFromDatabase();
};