#pragma once
#include "../Common/Netlib.h"
#include "../Common/cInterfaceDlgSendMessage.h"

namespace protoutil
{
	class cProtoUtil;
};

BEGIN_NETLIB

#ifdef PRINT_SYSTEM_RESOURCE_INFO
class cResourceInfo;
#endif
class cInterfacePacketParser;
class cInterfacePacketParserUDP;
class cMiniDump;
class cIocpUDP;
class cIOCP;
class cNetWork;
class cContextPooler;
class cThreadManager;
class cCommandThread;
class cCommandQueue;
class cWorkerThread;
class cLogThread;
class cLogQueue;
class cWebThread;
class ServerManager
{
private:
	TServerConfiguration* m_pTServerConfiguration; //환경 변수 등록용
	bool m_bServiceStart;
	cMiniDump* m_pMiniDump;							//덤프파일 생성용
	TCHAR m_OwnIP[16];
	TCHAR m_BasePath[256];

	char m_PrivateIP[CSDef::MAX_IP_ADDRESS_LEN];
	char m_PublicIP[CSDef::MAX_IP_ADDRESS_LEN];

	std::vector<cIocpUDP*> m_udp_instance_array;

public:
	cIocpUDP*				m_pIocpUDP;					// UDP socket port 1 ( symmetric 판별용으로 2개의 포트 뚫어둠 )
	cIOCP*					m_pIOCP;					// 컴플리션포트 생성 및 associate
	cNetWork*				m_pNetWork;					// 서버소켓 생성 등 네트웍 시작 종료처리
	cContextPooler*			m_pContextPooler;
	cThreadManager*			m_pThreadManager;

#ifndef USING_MULTI_THREAD
	cCommandThread*			pCommandThread;
#endif
#ifdef PRINT_SYSTEM_RESOURCE_INFO
	cResourceInfo* m_pResourceInfo;
#endif
	HINSTANCE				m_hInst;

	// queue
	cCommandQueue*			m_pCommandQueue;
	cLogQueue*				m_pLogQueue;

	// 패킷 파서 처리용 인터페이스
public:
	cInterfacePacketParser*	m_pInterfacePackerParser;
	cInterfacePacketParserUDP* m_pInterfacePacketParserUDP;

	// 로그 메시지 및 서버메시지 인터페이스
	cInterfaceDlgSendMessage* m_pInterfaceDlgSendMessage;

#if defined(PROTOCOL_TRACE_ON) &&  defined(_DEBUG)
	static ATL::CAtlMap<int, std::string> m_ProtocolCommandStringMap;

public:
	static int InitProtocolCommandStringMap();
	static std::string GetProtocolCommandString(const int cmd);
	static bool WriteProtocolCommandString(int cmd, bool isRecv, int remoteType, const char* myServerName);
#endif

	//함수
public:

	//메인 다이알로그생성
	//bool CreateMainDialog(int nCmd);

	void SetInstance(HINSTANCE hInst) { m_hInst = hInst; }

	void Init();
	void CreateConfigurations();

private:
	void Destroy();
	void CreateElement();
	void CreateThread();
	void CreateIocpQueue();
	void AnalizeNetInfo();

	void SetInterfaceInstance();
	void CheckOwnNetInfo();

	//변수
public:
	// IocpUDP pointer return
	cIocpUDP* GetIocpUDP() { return m_pIocpUDP; }

	void SetConfiguration(TServerConfiguration* ptr)
	{
		m_pTServerConfiguration = ptr;
	}

	E_SERVER_TYPE GetServerType()
	{
		if(m_pTServerConfiguration)
			return static_cast<E_SERVER_TYPE>(m_pTServerConfiguration->ServerType);

		return E_SERVER_TYPE::NONE_SERVER;
	}

	int GetServerGroupID()
	{
		if(m_pTServerConfiguration)
			return static_cast<E_SERVER_TYPE>(m_pTServerConfiguration->GID_FOR_MANAGE);

		return -1;
	}

	int GetServerID()
	{
		if (m_pTServerConfiguration)
			return static_cast<E_SERVER_TYPE>(m_pTServerConfiguration->SID_FOR_MANAGE);

		return -1;
	}

	TCHAR* GetRootDirectory() { return m_BasePath; }

	BOOL SocketInit();
	void SocketClose();

	void ServerStart();
	//void ServerStop();

	//void ManagerStart();

	bool ServerListenIPv4(bool bUsingOutputBufferIntoAcceptRequest, WORD wPort, UINT iBacklog = 1);
	bool ServerListenIPv6(bool bUsingOutputBufferIntoAcceptRequest, WORD wPort, UINT iBacklog = 1);

	bool IsSereverStarted() { return m_bServiceStart; }
	void SetServerStop() { m_bServiceStart = false; }

	int  GetIocpContextPoolCnt();
	LONG GetLiveIocpContextCnt();
	//int  GetFailedContextCnt();
	TCHAR* GetOwnIPAddress() { return m_OwnIP; }
	char* GetPublicIP() { return m_PublicIP; }
	char* GetPrivateIP() { return m_PrivateIP; }
	int GetUdpInstanceCnt() { return static_cast<int>(m_udp_instance_array.size()); }
	std::vector<int> GetUdpPortList();

	void ServerStatusReport();

	bool isSlackOn() { return m_pTServerConfiguration->bSLackAlert; }

	// 패킷 파서 등록
public:
	void SetPacketParserPtr(cInterfacePacketParser* pInterface) { m_pInterfacePackerParser = pInterface; }
	void SetPacketParserUdpPtr(cInterfacePacketParserUDP* pInterface) { m_pInterfacePacketParserUDP = pInterface; }

	void SetDlgSendMessagePtr(cInterfaceDlgSendMessage* pInterface) { m_pInterfaceDlgSendMessage = pInterface; }

	inline void SendMessageToListBox(EStringColor eColor, const TCHAR* tzFormat, ...)
	{
		if(!m_pInterfaceDlgSendMessage)
			return;

		TCHAR Buffer[BUFFSIZE];
		va_list marker;

		va_start(marker, tzFormat);
		vswprintf_s(Buffer, tzFormat, marker);

		if(m_pTServerConfiguration)
		{
			if(m_pTServerConfiguration->bUseConsole)
				m_pInterfaceDlgSendMessage->SendMessageToListBox(eColor, Buffer);
			else
				m_pInterfaceDlgSendMessage->SendLogMessage(eColor, Buffer); // console 모드일때는, 콘솔창에 뿌립니다.
		}

		va_end(marker);
	}

	inline void SendLogMessage(EStringColor eColor, const TCHAR* tzFormat, ...)
	{
		if(!m_pInterfaceDlgSendMessage)
			return;

		TCHAR Buffer[BUFFSIZE];
		va_list marker;

		va_start(marker, tzFormat);
		vswprintf_s(Buffer, tzFormat, marker);

		m_pInterfaceDlgSendMessage->SendLogMessage(eColor, Buffer);

		va_end(marker);
	}

	// 환경변수 
public:
	TServerConfiguration* GetConfiguration() { return m_pTServerConfiguration; }

	// Thread 포인터 Get메써드
	cCommandThread* GetCommandThreadPtr();
	cWorkerThread*	GetWorkerThreadPtr();
	cLogThread*		GetLogThreadPtr();
	cWebThread*		GetWebThreadPtr();

public:
	ServerManager();
	ServerManager(HINSTANCE hInst);
	~ServerManager();
};

END_NETLIB