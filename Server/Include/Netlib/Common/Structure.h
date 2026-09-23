#pragma once

typedef int int32;
typedef unsigned int uint32;
typedef __int64 int64;
typedef unsigned __int64 uint64;
typedef unsigned short WORD;
typedef unsigned char BYTE;

struct TPacketAnalyze
{
	DWORD dwRecvPacketPerMinute;
	DWORD dwSendPacketPerMinute;

	DWORD dwRecvDataSize;
	DWORD dwSendDataSize;
	ULONGLONG ullConnectionTime;
};

// 서버 환경변수 구조체
struct DBConnection
{
	WORD	pading0;
	BYTE	pading1;

	BYTE	Conn;
	int		Port;
	TCHAR	szDBAddr[20];
	TCHAR	szID[30];
	TCHAR	szPwd[30];
	TCHAR	szDBName[30];
	TCHAR   szDBport[30];
};

struct DBInfomation
{
	UINT MaxConn;
	UINT DBConnectionPoolCnt;
	DBConnection ConnInfo[EDBGroup::EDB_CNT];
};

struct DBConnectionInfo
{
	TCHAR	szHost[CSDef::EDef::MAX_DNS_LEN];
	UINT	nPORT;
	TCHAR	szUser[CSDef::EDef::MAX_DNS_LEN];
	TCHAR	szPASS[CSDef::EDef::MAX_DNS_LEN];
	TCHAR	szNAME[CSDef::EDef::MAX_DNS_LEN];
};

struct ConnectorInfo
{
	ConnectorInfo()
		: bInitialize(FALSE),
		nPort(0),
		nRemotePort(0),
		nServerType(0),
		ServerGroupID(0),
		pServerContext(nullptr),
		uiSessionCnt(0),
		uiMaxSessionCnt(0),
		allocatedslot(0)
	{
		szIP[0] = 0;
	}

	TCHAR	szIP[CSDef::EDef::MAX_DNS_LEN];
	char	readDnsFlag;
	BOOL	bInitialize;
	int		nPort;
	int		nRemotePort;
	int		nServerType;
	int		ServerGroupID;
	UINT	uiSessionCnt;
	UINT	uiMaxSessionCnt;
	void*	pServerContext;
	__int64 allocatedslot;
	std::string publicIpAddress;
	std::string ipAddress;
	std::string serverTypeString;
	int		serverId;				// 서버 아이디
};

struct WebServerInfo
{
	TCHAR	szIP[CSDef::EDef::MAX_IP_ADDRESS_LEN];
	TCHAR	szDNS[CSDef::EDef::MAX_DNS_LEN];
	TCHAR	szController[CSDef::EDef::MAX_BUFFER_256_LEN];
	TCHAR	szAction[CSDef::EDef::MAX_BUFFER_256_LEN];
	int		nPort;
};

typedef struct ServerConfiguration
{
	ServerConfiguration()
	{
		memset(this, 0x00, sizeof(ServerConfiguration));
	}

	~ServerConfiguration()
	{
		if(DBInfo)
			delete DBInfo;
	}

	TCHAR	szAppName[CSDef::EDef::MAX_BUFFER_64_LEN];
	TCHAR   szModuleExeFileName[CSDef::EDef::MAX_BUFFER_64_LEN];
	BYTE	ServerType; // Flag.h E_SERVER_TYPE
	char	ServerTypeString[CSDef::EDef::MAX_BUFFER_64_LEN]; // 서버 타입 스트링
	BYTE	ServerStage; // Flag.h E_SERVER_TYPE
	char	ServerStageString[CSDef::EDef::MAX_BUFFER_64_LEN]; // 서버 타입 스트링
	std::string PublicIpAddres;

	BYTE	pading0;

	WORD	wDefaultServerPort;
	WORD	wBufferServerPort;
	int		nSocketPoolSize;
	WORD	wMaxUser;
	WORD	wBackLog;
	int		nCommandThreadCnt;
	int		nWorkerThreadCnt;
	int		nWebThreadCnt;
	int		nDBThreadCnt;
	int		nThreadCntPerUdpSocket;
	int		nLogThreadCnt;
	int		nCommandQueueCnt;
	int		nLogQueueCnt;
	int		nWebQueueCnt;
	int		nPerSocket_OvPoolCnt;
	int		nUdpSockCount;

	int		nServerKeyHint;	//	서버 키를 생성하기 위해 만들 IP hint 입니다.

	BOOL	bUseConsole;
	int		nReCreateLogMinute;
	LOG_GRADE LogGrade;
	// widnows position
	int		nX;
	int		nY;
	int		cX;
	int		cY;

	/*
	각 서버들은 모두 서로 접속시키도록 한다.
	*/
	//ConnectorInfo lobby[CSDef::EDef::MAX_LOBBY_COUNT];	// 로비서버는 2개
	//ConnectorInfo slot[CSDef::EDef::MAX_SLOT_COUNT];
	std::vector<ConnectorInfo> lobbies;
	std::vector<ConnectorInfo> slots;

	int nRoomCntPerThread;
	int	nRoomPlayerMax;
	int nDamageHackDetectOn;

	int SID_FOR_MANAGE;// ManagerServer에서 관리하는 SID 번호 server_idx
	int GID_FOR_MANAGE;// ManagerServer에서 관리하는 GID 번호 server_group

	//UINT nMaxRoomCnt;	//	Constant Template 으로 옮겼습니다.
	UINT nMaxChannelUser;//

	WebServerInfo webserver;

	// database
	DBInfomation* DBInfo;

	// redis
	UINT	nRedisPort;
	TCHAR	szRedisDns[CSDef::EDef::MAX_DNS_LEN];

	// account db
	DBConnectionInfo accountDB;

	// log db
	TCHAR	szLogHost[CSDef::EDef::MAX_DNS_LEN];
	UINT	nLogPORT;
	TCHAR	szLogUser[CSDef::EDef::MAX_DNS_LEN];
	TCHAR	szLogPASS[CSDef::EDef::MAX_DNS_LEN];
	TCHAR	szLogNAME[CSDef::EDef::MAX_DNS_LEN];

	// game db -> 샤드구성은 최대 10개로 한다.
	DBConnectionInfo gameDBs[CSDef::EDef::MAX_SHARD_DB_CNT];

	// print import logs
	BOOL bPrintImportantLog;

	BOOL bSLackAlert;

	std::string pinballVersion;

}TServerConfiguration, *pTServerConfiguration;

struct stSchedule
{
	stSchedule()
	{
		memset(this, 0x00, sizeof(stSchedule));
	}

	ULONGLONG dwStart;
	//ULONGLONG dwEnd;
	ULONGLONG dwLastEvent;
	ULONGLONG dwMSec; // milisec
	ULONGLONG nextTick;
	bool  bAutoDelete;
	UINT uTargetCommandThreadIndex;
	UINT uTimerID;
	bool bRandomCommand; // select 1 command thread
};

struct stOBSERVER_LOG
{
	stOBSERVER_LOG() { Clear(); }
	void Clear() { memset(this, 0x00, sizeof(stOBSERVER_LOG)); }
	TCHAR	m_tzLog[CSDef::MAX_OBSERVER_BUFFER_SIZE];
};

struct PROCESS_RESOURCE_DATA
{
	double						proc_use_cpu;
	BYTE						sys_use_cpu;
	DWORD						proc_use_mem;
	DWORD						sys_total_mem;
	DWORD						recv_bytes;
	DWORD						send_bytes;
	TCHAR						server_file_name[CSDef::MAX_DEF_LEN_SERVER_NAME];
	WORD						server_id;
};

struct MANAGE_INFO
{
	MANAGE_INFO() { Clear(); }
	void Clear() { memset(this, 0x00, sizeof(MANAGE_INFO)); }
	WORD					server_id;
	WORD					group_id;
	WORD					server_type;
	TCHAR					server_path[MAX_PATH];
	TCHAR					data_path[MAX_PATH];
	TCHAR					server_file_name[CSDef::MAX_DEF_LEN_SERVER_NAME];
	TCHAR					ftp_path[MAX_PATH];
	TCHAR					private_ip[CSDef::MAX_IP_ADDRESS_LEN];
};

struct Sys_Net_Session_Log_Out_Data
{
	int64 llAllocatedSessionSlot;

	Sys_Net_Session_Log_Out_Data()
	{
		llAllocatedSessionSlot = -1;
	}
};

#pragma pack(1)


struct ServerInfo
{
	ServerInfo()
	{
		memset(this, 0x00, sizeof(ServerInfo));
	}

	WORD	wServerIdx;
	BYTE	byServerGroup;
	BYTE	byServerType;
	UINT	uiUsers;
	WORD	wPort;
	TCHAR	szServerIP[CSDef::MAX_IP_ADDRESS_LEN];
};

struct GMCS_NOTI_GAMESERVERINFO
{
	GMCS_NOTI_GAMESERVERINFO()
	{
		memset(this, 0x00, sizeof(*this));
	}

	void operator = (GMCS_NOTI_GAMESERVERINFO& rhs)
	{
		this->ServerIndex = rhs.ServerIndex;
		this->ServerGroupID = rhs.ServerGroupID;
		this->ServerType = rhs.ServerType;
		this->ServerStatus = rhs.ServerStatus;

		this->WebPort = rhs.WebPort;
		this->RedisPort = rhs.RedisPort;
		this->PrivatePort = static_cast<int>(rhs.PrivatePort);
		this->PublicPort = rhs.PublicPort;

		strcpy_s(this->szWebIP, sizeof(this->szWebIP), rhs.szWebIP);
		strcpy_s(this->szRedisIP, sizeof(this->szRedisIP), rhs.szRedisIP);
		strcpy_s(this->szPrivateIP, sizeof(this->szPrivateIP), rhs.szPrivateIP);
		strcpy_s(this->szPublicIP, sizeof(this->szPublicIP), rhs.szPublicIP);

		this->allocatedslot = rhs.allocatedslot;

		sprintf_s(szWebIPandPortString, sizeof(szWebIPandPortString), "%s:%d", szWebIP, WebPort);

		size_t len = strlen(rhs.szPublicIP);
		len = (size_t)::MultiByteToWideChar(CP_ACP, 0, rhs.szPublicIP, -1, wszPublicIP, (int)len);

		wcscpy_s(wszPublicIP, _countof(wszPublicIP), rhs.wszPublicIP);
	}

	BOOL GetPublicIP(OUT wchar_t* pBuf, IN int bufCnt, OUT int& port)
	{
		if(!pBuf || bufCnt != CSDef::EDef::MAX_IP_ADDRESS_LEN)
			return FALSE;

		wcscpy_s(pBuf, bufCnt, this->wszPublicIP);
		port = this->PublicPort;
		return TRUE;
	}

	///////////////////////////////////////////////////////////////////////////
	int ServerIndex;
	int ServerGroupID;
	int ServerType;
	int ServerStatus;

	int WebPort;
	int RedisPort;
	int PrivatePort;
	int PublicPort;

	char szWebIP[CSDef::EDef::MAX_IP_ADDRESS_LEN];
	char szRedisIP[CSDef::EDef::MAX_IP_ADDRESS_LEN];
	char szPrivateIP[CSDef::EDef::MAX_IP_ADDRESS_LEN];
	char szPublicIP[CSDef::EDef::MAX_IP_ADDRESS_LEN]; // 클라에게 전송용.

	__int64 allocatedslot;

	char szWebIPandPortString[CSDef::EDef::MAX_IP_ADDRESS_LEN * 2];
	wchar_t wszPublicIP[CSDef::EDef::MAX_IP_ADDRESS_LEN];
};

struct stThreadMonitor
{
	stThreadMonitor()
	{
		lastTick = 0;
	}

	uint64 lastTick;			// write only thread, read only monitoring thread
	std::string description;	// thread description

	void UpdateTick()
	{
		lastTick = ::GetTickCount64();
	}

	void EndTick()
	{
		lastTick = 0;
	}
};

struct stWebConnectionInfo
{
	stWebConnectionInfo()
	{
		Clear();
	}

	~stWebConnectionInfo()
	{
		Clear();
	}

	void Clear()
	{
		server_idx = 0;
		server_type = 0;
		private_IP.clear();
		game_server_port = 0;
		bAlive = TRUE;
	}

	int server_idx;
	int server_type;
	std::string private_IP;
	int game_server_port;
	BOOL bAlive;
};

#pragma pack()