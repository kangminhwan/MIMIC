#include "../../Include/Netlib/Manager/ServerManager.h"
#include "../../Include/Netlib/Manager/cLogManager.h"
#include "../../Include/Netlib/Manager/cThreadManager.h"
#include "../../Include/Netlib/Manager/cSessionManager.h"
#include "../../Include/Netlib/MiniDump/cMiniDump.h"
#include "../../Include/Netlib/Scheduler/cScheduler.h"
#include "../../Include/Netlib/Network/cNetWork.h"
#include "../../Include/Netlib/Common/cInterfacePacketParser.h"
#include "../../Include/Netlib/Common/cInterfaceDlgSendMessage.h"
#include "../../Include/Netlib/Common/cInterfaceIocpContext.h"
#include "../../Include/Netlib/Common/cInterfaceLog.h"
#include "../../Include/Netlib/UdpModule/cIocpUDP.h"
#include "../../Include/Netlib/UdpModule/cUDPSession.h"
#include "../../Include/Netlib/UdpModule/cUDPSessionManager.h"
#include "../../Include/Netlib/Network/cContextPooler.h"
#include "../../Include/Netlib/IOCP/cIocp.h"
#include "../../Include/Netlib/IOCP/cIocpConnector.h"
#include "../../Include/Netlib/Queue/cCommandQueue.h"
#include "../../Include/Netlib/Queue/cLogQueue.h"
#include "../../Include/Netlib/Queue/cWebQueue.h"
#include "../../Include/Netlib/FileLoader/cIniFileLoader.h"
#include "../../Include/Netlib/Session/cSession.h"
#include "../../Include/Netlib/Thread/cCommandThread.h"
#include "../../Include/Netlib/Thread/cWebThread.h"
#include "../../Include/Netlib/Common/cSingleton.h"
#include "../../Include/Netlib/Timer/cTimer.h"
#include "../../Include/Netlib/Thread/cThreadPool.h"

#ifdef USING_MULTI_THREAD
#include "../../Include/Netlib/Manager/cCommandQueueManager.h"
#endif
#ifdef PRINT_SYSTEM_RESOURCE_INFO
#include "../../Include/Netlib/ResourceMonitor/cResourceInfo.h"
#endif

NetLib::ServerManager::ServerManager() :
	m_bServiceStart(false),
	m_pTServerConfiguration(nullptr),
	m_pInterfacePackerParser(nullptr),
	m_pInterfacePacketParserUDP(nullptr),
	m_pInterfaceDlgSendMessage(nullptr),
	m_hInst(NULL)
{
	Init();
}

NetLib::ServerManager::ServerManager(HINSTANCE hInst) :
	m_bServiceStart(false),
	m_pTServerConfiguration(nullptr),
	m_pInterfacePackerParser(nullptr),
	m_pInterfacePacketParserUDP(nullptr),
	m_pInterfaceDlgSendMessage(nullptr),
	m_hInst(hInst)
{
	Init();
}

NetLib::ServerManager::~ServerManager()
{
	Destroy();

	if(m_pMiniDump)
	{
		m_pMiniDump->End();
	}
	NetLib::cSingleton<NetLib::cMiniDump>::DeleteInstance();
}

void NetLib::ServerManager::Init()
{
	m_pIocpUDP = nullptr;

	m_pIOCP = nullptr;
	m_pNetWork = nullptr;
	m_pContextPooler = nullptr;
	m_pThreadManager = nullptr;
	m_pCommandQueue = nullptr;
	m_pLogQueue = nullptr;
	m_pInterfacePackerParser = nullptr;

	NetLib::cSingleton<NetLib::cTimer>::GetInstance();
	NetLib::cSingleton<NetLib::cScheduler>::GetInstance();
	NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance();
	m_pNetWork = NetLib::cSingleton<NetLib::cNetWork>::GetInstance();
	m_pMiniDump = NetLib::cSingleton<NetLib::cMiniDump>::GetInstance();
	NetLib::cSingleton<NetLib::cSessionManager>::GetInstance();
	NetLib::cSingleton<NetLib::cUDPSessionManager>::GetInstance();
	NetLib::cSingleton<NetLib::cWebQueue>::GetInstance();
	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance();

#ifdef USING_MULTI_THREAD
	NetLib::cSingleton<NetLib::cCommandQueueManager>::GetInstance();
#else
	cSingleton<cCommandQueue>::GetInstance();
#endif

	if(m_pMiniDump)
	{
		m_pMiniDump->Begin();
	}

	memset(m_BasePath, 0x00, sizeof(m_BasePath));
	/////////////////////////////////////////////////////////
	//밑에 함수의 설명은 LogManager.CPP 파일의 Create함수에 쓰여있습니다.
	::GetModuleFileName(NULL, m_BasePath, sizeof(m_BasePath));
	::PathRemoveFileSpec(m_BasePath);
	/////////////////////////////////////////////////////////
	//::GetCurrentDirectory(sizeof(m_BasePath), m_BasePath);

	memset(m_PrivateIP, 0x00, sizeof(m_PrivateIP));
	memset(m_PublicIP, 0x00, sizeof(m_PublicIP));
	memset(m_OwnIP, 0x00, sizeof(m_OwnIP));
}

void NetLib::ServerManager::Destroy()
{
	NetLib::cSingleton<NetLib::cScheduler>::DeleteInstance();
	NetLib::cSingleton<NetLib::cThreadManager>::DeleteInstance();
	if(m_pInterfacePackerParser)
	{
		delete m_pInterfacePackerParser;
		m_pInterfacePackerParser = nullptr;
	}

	if(m_pIocpUDP)
	{
		delete[] m_pIocpUDP;
		m_pIocpUDP = nullptr;
	}

	NetLib::cIniFileLoader::Destroy();
	
	NetLib::cSingleton<NetLib::cIocpConnector>::DeleteInstance();
	NetLib::cSingleton<NetLib::cUDPSessionManager>::DeleteInstance();
	NetLib::cSingleton<NetLib::cSessionManager>::DeleteInstance();
	NetLib::cSingleton<NetLib::cNetWork>::DeleteInstance();
	NetLib::cSingleton<NetLib::cContextPooler>::DeleteInstance();

#ifdef USING_MULTI_THREAD
	NetLib::cSingleton<NetLib::cCommandQueueManager>::DeleteInstance();
#else
	cSingleton<cCommandQueue>::DeleteInstance();
#endif

#ifdef PRINT_SYSTEM_RESOURCE_INFO
	cSingleton<cResourceInfo>::DeleteInstance();
	m_pResourceInfo = nullptr;
#endif

	NetLib::cSingleton<NetLib::cIOCP>::DeleteInstance();
	NetLib::cSingleton<NetLib::cLogManager>::DeleteInstance();
	NetLib::cSingleton<NetLib::cLogQueue>::DeleteInstance();
	NetLib::cSingleton<NetLib::cWebQueue>::DeleteInstance();

	NetLib::cSingleton<NetLib::cTimer>::DeleteInstance();

	NetLib::cSingleton<cThreadPooler::cThreadPool>::DeleteInstance();

	m_pThreadManager = nullptr;
	m_pNetWork = nullptr;
	m_pContextPooler = nullptr;
	m_pIOCP = nullptr;
}

void NetLib::ServerManager::CreateElement()
{
	SYSTEM_INFO sys;
	GetSystemInfo(&sys);

#ifdef PRINT_SYSTEM_RESOURCE_INFO
	m_pResourceInfo = cSingleton<cResourceInfo>::GetInstance();
#endif
	m_pIOCP = NetLib::cSingleton<NetLib::cIOCP>::GetInstance();
	m_pContextPooler = NetLib::cSingleton<NetLib::cContextPooler>::GetInstance();
	NetLib::cSingleton<NetLib::cLogManager>::GetInstance();

//#if _DEBUG
//	// 콘솔창 출력 활성화 해둔다.
//	NetLib::cLogManager::outputConsole = TRUE;
//#endif
}

void NetLib::ServerManager::SetInterfaceInstance()
{
	NetLib::cCommandThread* pCommandThread = m_pThreadManager->GetCommandThreadPtr();
	NetLib::cWebThread* pWebThread = m_pThreadManager->GetWebThreadPtr();
	if(pCommandThread == nullptr)
	{
		assert(false && "ServerManager::SetInterfaceInstance is Failed. CommandThread is nullptr");
		return;
	}

	if(pWebThread == nullptr)
	{
		assert(false && "ServerManager::SetInterfaceInstace is Failed. WebThread is nullptr");
	}

	if(!m_pInterfacePackerParser)
	{
		::OutputDebugString(_T("InterfacePackerParser is NULL, plz set!!"));
		::OutputDebugString(_T("Use SetPacketParserPtr Func in ServerManager Class"));
		return;
	}

	pCommandThread->SetParserInstancePtr(m_pInterfacePackerParser);
	pWebThread->SetParserInstancePtr(m_pInterfacePackerParser);
}

void NetLib::ServerManager::CreateIocpQueue()
{
	if(!m_pTServerConfiguration)
	{
		SendMessageToListBox(EStringColor::RED, _T("Servermanager::CreateIocpQueue() failed"));
		SendMessageToListBox(EStringColor::RED, _T("Servermanager::Server Configuration not exists"));
		return;
	}

	if (m_pTServerConfiguration->ServerType == E_SERVER_TYPE::LOBBY_SERVER ||
		m_pTServerConfiguration->ServerType == E_SERVER_TYPE::SLOT_SERVER)
	{
		NetLib::cSingleton<NetLib::cWebQueue>::GetInstance()->CreateWebQueueElementPool(m_pTServerConfiguration->nWebQueueCnt);
	}

#ifdef USING_MULTI_THREAD
	NetLib::cCommandQueueManager* pCommandQueueManager = NetLib::cSingleton<NetLib::cCommandQueueManager>::ExistsInstance();
	if(pCommandQueueManager == nullptr)
	{
		assert(false && "CreateIocpQueue Failed. cCommandQueueManager is nullptr");
		return;
	}
	pCommandQueueManager->Init(m_pTServerConfiguration->nCommandThreadCnt, m_pTServerConfiguration->nCommandQueueCnt);
#else
	m_pCommandQueue = NetLib::cSingleton<NetLib::cCommandQueue>::ExistsInstance();
	if(m_pCommandQueue == nullptr)
	{
		assert(false && "CreateIocpQueue Failed. cCommandQueue is nullptr");
		return;
	}
	m_pCommandQueue->CreateCommandQueueElementPool(m_pTServerConfiguration->nCommandQueueCnt);
#endif
	m_pLogQueue = NetLib::cSingleton<NetLib::cLogQueue>::ExistsInstance();
	if(m_pLogQueue == nullptr)
	{
		assert(false && "CreateIocpQueue Failed. cLogQueue is nullptr");
		return;
	}

	m_pLogQueue->CreateLogQueueElementPool(m_pTServerConfiguration->nLogQueueCnt);
}

void NetLib::ServerManager::AnalizeNetInfo()
{
	const int nLanCardCnt = 5;
	IP_ADAPTER_INFO ipAdapterInfo[nLanCardCnt];
	DWORD dwBufLen = sizeof(ipAdapterInfo);

	// IP Address List 얻어옵니다.
	DWORD dwStatus = GetAdaptersInfo(ipAdapterInfo, &dwBufLen);
	if (dwStatus != ERROR_SUCCESS)
	{
		SendMessageToListBox(RED, _T("ServerManager::AnlizeNetInfo() failed."));
		return;
	}

	std::vector<char*> VecIP;
	VecIP.reserve(nLanCardCnt);

	for (int nLanCnt = 0; nLanCnt < nLanCardCnt; ++nLanCnt)
	{
		IP_ADAPTER_INFO tIpAdapterInfo = ipAdapterInfo[nLanCnt];

		if (tIpAdapterInfo.AddressLength == 0)
		{
			continue;
		}

		char* pTempIP = new char[CSDef::EDef::MAX_IP_ADDRESS_LEN];
		memset(pTempIP, 0x00, CSDef::EDef::MAX_IP_ADDRESS_LEN);

		IP_ADDR_STRING ipAddrString1 = tIpAdapterInfo.IpAddressList;

		StringCbCopyA(pTempIP, CSDef::EDef::MAX_IP_ADDRESS_LEN, ipAddrString1.IpAddress.String);
		VecIP.push_back(pTempIP);

		if (tIpAdapterInfo.Next)
		{
			IP_ADAPTER_INFO* pIpAdapterInfo = tIpAdapterInfo.Next;
			IP_ADDR_STRING ipAddrString2 = pIpAdapterInfo->IpAddressList;
			pTempIP = new char[CSDef::EDef::MAX_IP_ADDRESS_LEN];
			memset(pTempIP, 0x00, CSDef::EDef::MAX_IP_ADDRESS_LEN);
			StringCbCopyA(pTempIP, CSDef::EDef::MAX_IP_ADDRESS_LEN, ipAddrString2.IpAddress.String);
			VecIP.push_back(pTempIP);
		}
	}

	// 아이피 분석, 사설아이피, 공인 아이피로 분석합니다.
	// 공인 아이피가 없을 경우에는 공인 아이피 정보에 사설 아이피로 셋팅합니다.

	char s[CSDef::EDef::MAX_IP_ADDRESS_LEN] = { 0, };
	std::vector<char*>::iterator iter = VecIP.begin();
	std::vector<char*>::iterator iterEnd = VecIP.end();

	for (; iter != iterEnd; ++iter)
	{
		char* pIP = (*iter);

		StringCbCopyA(s, CSDef::EDef::MAX_IP_ADDRESS_LEN, pIP);

		char* pContext = nullptr;

		char* ptr = strtok_s(s, ".", &pContext);
		
		// 사설 아이피 확인
		if (strcmp(ptr, "10") == 0 ||
			//strcmp(ptr, "172") == 0 ||
			strcmp(ptr, "192") == 0)
		{
			StringCbCopyA(m_PrivateIP, sizeof(m_PrivateIP), pIP);
		}
		else
		{
			if (strcmp("0.0.0.0", (*iter)) != 0)
			{
				StringCbCopyA(m_PublicIP, sizeof(m_PublicIP), pIP);
			}
		}
	}

	// 공인 아이피가 비어 있으면, 사설 아이피로 셋팅합니다.
	if (safe_strlen(m_PublicIP, sizeof(m_PublicIP)) == 0 ||
		strcmp("0.0.0.0", m_PublicIP) == 0)
	{
		StringCbCopyA(m_PublicIP, sizeof(m_PublicIP), m_PrivateIP);
	}

	// 아이피 분석에 사용한 char* 동적으로 할당한 메모리
	// 해제 합니다.
	iter = VecIP.begin();
	iterEnd = VecIP.end();
	for (; iter != iterEnd; ++iter)
	{
		if ((*iter) != nullptr)
		{
			delete (*iter);
			(*iter) = nullptr;
		}
	}
	VecIP.clear();

	SendMessageToListBox(RED, _T("ServerManager::AnalizeNetInfo() Success."));
}

void NetLib::ServerManager::CreateThread()
{
	m_pThreadManager = NetLib::cSingleton<NetLib::cThreadManager>::GetInstance();

	if(m_pTServerConfiguration)
	{
		m_pThreadManager->SetCommandThreadCnt(m_pTServerConfiguration->nCommandThreadCnt);
		m_pThreadManager->SetWorkerThreadCnt(m_pTServerConfiguration->nWorkerThreadCnt);
		m_pThreadManager->SetWebThreadCnt(m_pTServerConfiguration->nWebThreadCnt);

		// 로그 쓰레드는 특별한 이유가 없는한 1개를 유지 합니다.
		m_pThreadManager->SetLogThreadCnt();
	}
}

void NetLib::ServerManager::CheckOwnNetInfo()
{
	GetLocalAddress(m_OwnIP);
}

void NetLib::ServerManager::CreateConfigurations()
{
	CreateElement();
	CreateIocpQueue();
	CreateThread();
	SetInterfaceInstance();
	CheckOwnNetInfo();
	AnalizeNetInfo();
}

bool NetLib::ServerManager::ServerListenIPv4(bool bUsingOutputBufferIntoAcceptRequest, WORD wPort, UINT iBacklog)
{
	if(m_pNetWork)
	{
		UINT nID;
		if(m_pNetWork->CreateServerIPv4Socket(wPort, iBacklog, nID, bUsingOutputBufferIntoAcceptRequest))
		{
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_SYSTEM, _T("ServerManager::ServerListen() success."));
			return true;
		}
		else
		{
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_SYSTEM, _T("ServerManager::ServerListen::CreateServerIPv4Socket failed."));
			return false;
		}
	}

	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_SYSTEM, _T("ServerManager::ServerListen() failed."));
	return false;
}

bool NetLib::ServerManager::ServerListenIPv6(bool bUsingOutputBufferIntoAcceptRequest, WORD wPort, UINT iBacklog)
{
	if(m_pNetWork)
	{
		UINT nID;
		if(m_pNetWork->CreateServerIPv6Socket(wPort, iBacklog, nID, bUsingOutputBufferIntoAcceptRequest))
		{
			NetLib::cLogQueue* pLogQueue = NetLib::cSingleton<NetLib::cLogQueue>::ExistsInstance();
			if(pLogQueue)
			{
				pLogQueue->PushCommand(LOG_GRADE::LOG_SYSTEM, _T("ServerManager::ServerListenIPv6() success."));
			}
			return true;
		}
		else
		{
			NetLib::cLogQueue* pLogQueue = NetLib::cSingleton<NetLib::cLogQueue>::ExistsInstance();
			if(pLogQueue)
			{
				pLogQueue->PushCommand(LOG_GRADE::LOG_SYSTEM, _T("ServerManager::ServerListen::CreateServerIPv6Socket failed."));
			}
			return false;
		}
	}
	NetLib::cLogQueue* pLogQueue = NetLib::cSingleton<NetLib::cLogQueue>::ExistsInstance();
	if(pLogQueue)
	{
		pLogQueue->PushCommand(LOG_GRADE::LOG_SYSTEM, _T("ServerManager::ServerListenIPv6() failed."));
	}

	return false;
}

void NetLib::ServerManager::ServerStart()
{
	if(m_bServiceStart)
	{
		::OutputDebugString(_T("Service already started!!"));
		return;
	}

	if(!m_pTServerConfiguration)
	{
		::OutputDebugString(_T("ServerConfiguration Error"));
		return;
	}

	if(m_pIOCP->Create())
	{
		if(m_pContextPooler)
		{
			if((m_pTServerConfiguration->nSocketPoolSize) <= (m_pTServerConfiguration->wMaxUser + m_pTServerConfiguration->wBackLog))
			{
				assert(FALSE, _T("nSocketPoolSize must bigger than wMaxUser+wBackLog"));
				Sleep(1000);
				exit(EXIT_FAILURE);
				return;
			}

			m_pContextPooler->Create(m_pTServerConfiguration->nSocketPoolSize);
		}

		// Default Accept
		if (m_pTServerConfiguration->wDefaultServerPort > 0)
		{
			if (!ServerListenIPv4(false, m_pTServerConfiguration->wDefaultServerPort, m_pTServerConfiguration->wBackLog))
			{
				return;
			}

			if (m_pTServerConfiguration->ServerType == E_SERVER_TYPE::LOBBY_SERVER)
			{
				if (!ServerListenIPv6(false, m_pTServerConfiguration->wDefaultServerPort, m_pTServerConfiguration->wBackLog))
				{
					return;
				}
			}
		}

		//Output Buffer Accept 
		if (m_pTServerConfiguration->wBufferServerPort > 0)
		{
			if (!ServerListenIPv4(true, m_pTServerConfiguration->wBufferServerPort, m_pTServerConfiguration->wBackLog))
			{
				return;
			}
		}

		m_pThreadManager->CreateServerThread();

		// 게임서버일 경우에만 UDP Thread를 생성하도록 한다.
		if (m_pTServerConfiguration->ServerType == E_SERVER_TYPE::LOBBY_SERVER)
		{
			// commandthread 갯수에 따라 UDP쓰레드 갯수를 조절한다.
			// 현재는 commandthread 갯수 만큼 UDP쓰레드 갯수를 만들도록 변경한다.
			int nCommandThreadCnt = 0;
#ifdef USING_MULTI_THREAD
			NetLib::cCommandQueueManager* pCommandQueueManager = NetLib::cSingleton<NetLib::cCommandQueueManager>::ExistsInstance();
			if (pCommandQueueManager == nullptr)
			{
				throw("NetLib::ServerManager::ServerStart(). cCommandQueueManager is nullptr");
				return;
			}

			nCommandThreadCnt = pCommandQueueManager->GetCommandQueueCnt();
#else
			m_pCommandQueue = NetLib::cSingleton<NetLib::cCommandQueue>::ExistsInstance();
			if (m_pCommandQueue == nullptr)
			{
				throw("NetLib::ServerManager::ServerStart(). cCommandQueue is nullptr");
				return;
			}
			m_pCommandQueue->CreateCommandQueueElementPool(m_pTServerConfiguration->nCommandQueueCnt);

			nCommandThreadCnt = m_pTServerConfiguration->nCommandThreadCnt; // 이경우는 커맨드 Queue 하나에, 커맨드 쓰레드 여러개일 가능성이 있는데
#endif
			int nCPU, nLogical;
			GetProcessorNumber(nCPU, nLogical);// 물리 코어 갯수를 구함

			//m_pIocpUDP = new cIocpUDP[nCPU];

			//NetLib::cIocpUDP* pIocpUDP = nullptr;

			//// 물리 코어 갯수 만큼만 UDP Bind 합니다.
			//for (int nArray = 0; nArray < nCPU; ++nArray)
			//{
			//	pIocpUDP = m_pIocpUDP + nArray;
			//	pIocpUDP->SetInterfaceLog(reinterpret_cast<NetLib::cInterfaceLog*>(m_pLogQueue));

			//	if (!pIocpUDP->StartIocpUDPModule(m_pTServerConfiguration->wDefaultServerPort + nArray + 1, m_pTServerConfiguration->nThreadCntPerUdpSocket))
			//	{
			//		throw("StartIocpUdpModule Failed");
			//	}

			//	m_udp_instance_array.push_back(pIocpUDP);
			//}
		}

		NetLib::cScheduler* pScheduler = NetLib::cSingleton<NetLib::cScheduler>::ExistsInstance();
		if(pScheduler)
		{
			pScheduler->StartScheduler();
		}

		if(m_pTServerConfiguration->bUseConsole)
		{
#ifdef _DEBUG
			const char* szPort = "DefaultPort";
			const char* szOutputBufferPort = "BufferPort";
#else
			const char* szPort = "DEFAULT_PORT";
			const char* szOutputBufferPort = "BUFFER_PORT";
#endif
			char szTitle[MAX_PATH] = { 0, };
			::GetConsoleTitleA(szTitle, MAX_PATH);

			std::string totaltitle(szTitle);
			std::string sFind(".exe");
			std::string::size_type findpos = totaltitle.find(sFind);
			totaltitle = totaltitle.substr(0, findpos);
			findpos = totaltitle.rfind("\\");
			totaltitle = totaltitle.substr(findpos + 1, totaltitle.size());

			sprintf_s(szTitle, "%s SID:%d GID:%d %s:%d %s:%d r%d ver(%s) %s"
				, totaltitle.c_str(), m_pTServerConfiguration->SID_FOR_MANAGE, m_pTServerConfiguration->GID_FOR_MANAGE,
				szPort, m_pTServerConfiguration->wDefaultServerPort, szOutputBufferPort, m_pTServerConfiguration->wBufferServerPort, SVN_REVISION, SERVICE_VERSION, SERVICE_LOCALE_NAME);

			::SetConsoleTitleA(szTitle);

			NetLib::cLogQueue* pLogQueue = NetLib::cSingleton<NetLib::cLogQueue>::ExistsInstance();
			if(pLogQueue)
			{
				pLogQueue->PushCommand(LOG_GRADE::LOG_SYSTEM, "ServerManager::ServerStart() Title - %s", szTitle);
				pLogQueue->PushCommand(LOG_GRADE::LOG_SYSTEM, "ServerManager::ServerStart() - Success.");
			}
		}

		m_bServiceStart = true;
	}
}

void NetLib::ServerManager::ServerStatusReport()
{
	size_t nRemainCommandQueueCnt = 0;
	int nCurrentCommandQueueCnt = 0;
	int nMaxCommandQueueCnt = 0;

	size_t nRemainLogQueueCnt = 0;
	int nCurrentLogQueueCnt = 0;
	int nMaxLogQueueCnt = 0;

	size_t nRemainWebQueueCnt = 0;
	int nCurrentWebQueueCnt = 0;
	int nMaxWebQueueCnt = 0;

	size_t nUDPSessionCnt = 0;
	size_t nRemainUDPSessionCnt = 0;
	size_t nConnetedUserCnt = 0;
	size_t nRemainTCPSessionCnt = 0;
	size_t nPendingSessionCnt = 0;
	int nContextPoolerCnt = 0;
	int nLiveContextCnt = 0;

	if(!m_pTServerConfiguration)
		return;

	if(!m_pTServerConfiguration->bUseConsole)
		return;

	if(IsSereverStarted())
	{
		NetLib::cLogQueue* pLogQueue = NetLib::cSingleton<NetLib::cLogQueue>::ExistsInstance();
		if(!pLogQueue)
			return;

		nRemainLogQueueCnt = pLogQueue->GetRemainQueueCnt();
		nCurrentLogQueueCnt = pLogQueue->GetCurrentPoolCnt();
		nMaxLogQueueCnt = pLogQueue->GetMaxPoolCnt();
		pLogQueue->PushCommand(LOG_GRADE::LOG_SYSTEM, _T("=========== SERVER INFO ==========="));
		pLogQueue->PushCommand(LOG_GRADE::LOG_SYSTEM, _T("=     (Remain, Current, Max)      ="));
		pLogQueue->PushCommand(LOG_GRADE::LOG_SYSTEM, _T("== LogueQueueCnt(%d,%d,%d)"), nRemainLogQueueCnt, nCurrentLogQueueCnt, nMaxLogQueueCnt);

#ifdef USING_MULTI_THREAD
		NetLib::cCommandQueueManager* pCommandQueueManager = NetLib::cSingleton<NetLib::cCommandQueueManager>::GetInstance();
		if (!pCommandQueueManager)
			return;

		nRemainCommandQueueCnt = 1;

		for (int n = 0; n < pCommandQueueManager->GetCommandQueueCnt(); ++n)
		{
			NetLib::cCommandQueue* pCommandQueue = pCommandQueueManager->GetCommandQueuePtr(n);
			if (pCommandQueue == nullptr)
				continue;

			nRemainCommandQueueCnt += pCommandQueue->GetRemainQueueCnt();// 현 펑션 처리중인 큐를 하나 더해줌
			nCurrentCommandQueueCnt += pCommandQueue->GetCurrentPoolCnt();
			nMaxCommandQueueCnt += pCommandQueue->GetMaxPoolCnt();
		}
#else
		NetLib::cCommandQueue* pCommandQueue = NetLib::cSingleton<NetLib::cCommandQueue>::ExistsInstance();
		if (!pCommandQueue)
			return;

		nRemainCommandQueueCnt = 1 + pCommandQueue->GetRemainQueueCnt();// 현 펑션 처리중인 큐를 하나 더해줌
		nCurrentCommandQueueCnt = pCommandQueue->GetCurrentPoolCnt();
		nMaxCommandQueueCnt = pCommandQueue->GetMaxPoolCnt();
#endif

		pLogQueue->PushCommand(LOG_GRADE::LOG_SYSTEM, _T("== CommandQueueCnt(%d,%d,%d)"), nRemainCommandQueueCnt, nCurrentCommandQueueCnt, nMaxCommandQueueCnt);

		/*NetLib::cWebQueue* pWebQueue = NetLib::cSingleton<NetLib::cWebQueue>::ExistsInstance();
		if (pWebQueue != nullptr)
		{
			NetLib::cSingleton<NetLib::cWebQueue>::GetInstance()->ReportStatus(nRemainWebQueueCnt, nCurrentWebQueueCnt, nMaxWebQueueCnt);
			pLogQueue->PushCommand(LOG_GRADE::LOG_SYSTEM, _T("== WebQueueCnt(%d,%d,%d)"), nRemainWebQueueCnt, nCurrentWebQueueCnt, nMaxWebQueueCnt);
		}*/

		/*NetLib::cUDPSessionManager* pUDPSessionMgr = NetLib::cSingleton<NetLib::cUDPSessionManager>::ExistsInstance();
		if(pUDPSessionMgr)
		{
			nUDPSessionCnt = pUDPSessionMgr->GetTotalUser();
			nRemainUDPSessionCnt = pUDPSessionMgr->GetRemainUDPSessionCount();
			pLogQueue->PushCommand(LOG_GRADE::LOG_SYSTEM, _T("== UDPSessionCnt %d, RemainUDPSessionCnt %d"), nUDPSessionCnt, nRemainUDPSessionCnt);
		}*/

		pLogQueue->PushCommand(LOG_GRADE::LOG_SYSTEM, _T("== AcceptCount %d"), m_pNetWork->AcceptCountReport());

		nLiveContextCnt = static_cast<int>(GetLiveIocpContextCnt());
		pLogQueue->PushCommand(LOG_GRADE::LOG_SYSTEM, _T("== GetLiveIocpContextCnt %d"), nLiveContextCnt);

		nContextPoolerCnt = GetIocpContextPoolCnt();
		pLogQueue->PushCommand(LOG_GRADE::LOG_SYSTEM, _T("== GetIocpContextPoolCnt %d"), nContextPoolerCnt);

		UINT serversessions = 0;
		UINT clientsessions = 0;
		UINT unknownsessions = 0;
		UINT agentsessions = 0;
		UINT toolsessions = 0;
		NetLib::cSessionManager* pSessionManager = NetLib::cSingleton<NetLib::cSessionManager>::GetInstance();
		nRemainTCPSessionCnt = pSessionManager->GetPoolerSize();
		nConnetedUserCnt = pSessionManager->GetSessionCount();
		nPendingSessionCnt = pSessionManager->GetPendingSessionCount();
		pSessionManager->GetSessionCountBySessionTypeUpgrade(serversessions, clientsessions, unknownsessions, agentsessions, toolsessions);
		

		pLogQueue->PushCommand(LOG_GRADE::LOG_SYSTEM, _T("== Total TCPSessionCnt %d, RemainTCPSessionCnt %d, PendingCnt %d"), nConnetedUserCnt, nRemainTCPSessionCnt, nPendingSessionCnt);
		pLogQueue->PushCommand(LOG_GRADE::LOG_SYSTEM, _T("== server %d, client %d, agent %d, tool %d, unknown %d"),
			serversessions, clientsessions, agentsessions, toolsessions, unknownsessions);

#ifdef PRINT_SYSTEM_RESOURCE_INFO
		std::vector<WORD> sid_list;
		std::vector<PROCESS_RESOURCE_DATA> data_list;
		m_pResourceInfo->GetResource(m_pTServerConfiguration->szModuleExeFileName, sid_list, data_list);

		for (int n = 0; n<data_list.size(); ++n)
		{
			if(_tcslen(data_list[n].server_file_name) == 0 || data_list[n].proc_use_mem == 0)
				continue;

			pLogQueue->PushCommand(LOG_GRADE::LOG_SYSTEM, _T("== Resource %s, SysCPU %u, CPU %.4f, SysMem %u, Mem %u(KB)"),
									data_list[n].server_file_name,
									data_list[n].sys_use_cpu,
									data_list[n].proc_use_cpu,
									data_list[n].sys_total_mem,
									data_list[n].proc_use_mem);
		}
#endif
		pLogQueue->PushCommand(LOG_GRADE::LOG_SYSTEM, _T("==================================="), nRemainLogQueueCnt);
	}
}

int  NetLib::ServerManager::GetIocpContextPoolCnt()
{
	if(m_pContextPooler)
		return (int)m_pContextPooler->GetCount();

	return -1;
}

LONG NetLib::ServerManager::GetLiveIocpContextCnt()
{
	if (m_pContextPooler)
		return m_pContextPooler->GetLiveContextCount();

	return -1;
}

std::vector<int> NetLib::ServerManager::GetUdpPortList()
{
	std::vector<int> udp_ports;

	for (size_t n = 0; n < m_udp_instance_array.size(); ++n)
	{
		if (m_udp_instance_array[n] == nullptr)
			continue;

		int udp_port = m_udp_instance_array[n]->GetBindingUdpPort();

		udp_ports.push_back(udp_port);
	}

	return udp_ports;
}

BOOL NetLib::ServerManager::SocketInit()
{
	return m_pNetWork->NetworkStartup();
}

void NetLib::ServerManager::SocketClose()
{
	m_pNetWork->NetworkCleanup();
}

NetLib::cCommandThread* NetLib::ServerManager::GetCommandThreadPtr()
{
	return m_pThreadManager->GetCommandThreadPtr();
}

NetLib::cWorkerThread* NetLib::ServerManager::GetWorkerThreadPtr()
{
	return m_pThreadManager->GetWorkerThreadPtr();
}

NetLib::cLogThread* NetLib::ServerManager::GetLogThreadPtr()
{
	return m_pThreadManager->GetLogThreadPtr();
}

NetLib::cWebThread* NetLib::ServerManager::GetWebThreadPtr()
{
	return m_pThreadManager->GetWebThreadPtr();
}