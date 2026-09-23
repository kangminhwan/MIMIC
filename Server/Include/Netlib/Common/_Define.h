#pragma once

//서비스 국가코드 //////////////////////////////////////////////
#define SERVICE_LOCALE_KOREA 1

//서비스 국가지정 //////////////////////////////////////////////
#define SERVICE_LOCALE		SERVICE_LOCALE_KOREA

#if SERVICE_LOCALE == SERVICE_LOCALE_KOREA
#define SERVICE_LOCALE_NAME	"KR"
#endif

// Client 버전 //////////////////////////////////////////////
#define SERVICE_VERSION "0_0_0_0"

// SVN Revision 지정 //////////////////////////////////////////////
#define SVN_REVISION 42971//배포시 SVN Revision(서버 패치파일 만들때 직전의 SVN 리비전 입력)

#define MAX_BUFFER_SIZE	1000
#define MAX_WORKER_THREADS 9

#define BUFFSIZE 5120+100
//#define SHOW_UDP_IO_OPERATIONS	//	 현재 사용하고있지 않습니다.
//#define PRINT_ROOM_BROADCAST_ENTITY_LIST // TCP, UDP 브로드캐스트 ENTITY 리스트 출력	//	 현재 사용하고있지 않습니다.
#define VIRTUAL_NAGLE_MS	100

//MemPooler Front push
#define USE_PUSH_FRONT_MEMPOOLER

//Packet_Tick_Check Context
#define USE_PACKET_TICK_CHECK_CONTEXT

// csv파일로딩
#define MAX_LINE_CHAR_COUNT 4096

//cWinInet 에서 쓰고있습니다.
#ifdef UNICODE
#define tstring	std::wstring
#else 
#define tstring	std::string
#endif

#define WININET_READ_BUFFER_SIZE 8192 * 2
//// 여기까지 cWinInet 

// 네트워크 버퍼의 길이에 대한 정의
const DWORD G_NET_BUFFER_SIZE_BASIC = 1024;
const DWORD G_NET_BUFFER_SIZE_32K = 1024 * 32;
const DWORD G_NET_BUFFER_SIZE_64K = 1024 * 64;
const DWORD G_DEFIOBUFFERLEN = (8192 * 2);
//const DWORD G_MAXSENDBUFFERLEN = (1024 * 8);
const DWORD G_MAXSENDBUFFERLEN = ( 1024 * 64 );
const DWORD G_MAXRECEIVEBUFFERLEN = (1024 * 8);
const DWORD G_DEF_MAX_STOREBUFFER_SIZE = (8192 * 2);				// 워커 쓰레드의 임시 스토어 버프 사이즈 처리
const DWORD G_DEF_MAX_CONNECTOR_STOREBUFFER_SIZE = (8192 * 10);	// 각각의 커넥터에서 씌이는 임시버퍼의 사이즈

////////////// SYMBOL DEFINE
#define PACKET_ANALYZE_ON			// 패킷 분석기능을 켤경우에만 사용
//#define VIRTUAL_NAGLE_ON_OFF		// 가상 Nagle시스템 적용여부...
#define USING_MULTI_THREAD				// commandthread 멀티쓰레드 작업
//#define PRINT_SYSTEM_RESOURCE_INFO		//서버의 CPU, 메모리 등 리소스 사용 상태 출력
//Log 출력옵션
#define USE_LOG_FILE		// 리스트박스에 출력
#define _CRYPT

///DEBUG DEFINE
#ifdef _DEBUG
#define USE_CONSOLE_INPUT_FOR_SHUTDOWN //콘솔창에 end를 입력받아 서버 정지
#endif

//cContextPooler 에서 사용중
//Ping 로그 남길때에도 사용중입니다.
#define DEVELOPMENT				//	개발중에 서버 종료시 릭을 안남기기 위한
#define DEVELOPMENTSUCCESSLOG	//	성공 로그 남기기 위한 전처리

#define USE_CONTEXTPOOLER_SKIP_LIST_ALGORITHM	//	ContextPooler가 Context Container를 Skip List를 사용합니다.

//#define USE_FORCE_DISCONNECT	//	Context Pooler 가 Force DisConnect 를 사용합니다.

#ifdef USE_CONTEXTPOOLER_SKIP_LIST_ALGORITHM

//#define MAX_COMMAND_THREAD 17	//0	=> All Container, 1 ~ 17 Thread Container	
//#define MAX_LEVEL 16			//

#define MAX_COMMAND_THREAD 49	//0	=> All Container, 1 ~ 17 Thread Container	
#define MAX_LEVEL 48			//	

#define FORWARDS_MEMORY_SET MAX_COMMAND_THREAD * MAX_LEVEL

#endif

/*
 *	__pragma(message(__FILE__ "(" STRINGNET(__LINE__) "): [ " __func__ " ] [ TODO ] " msg)) 이 코드에
 *	LINELINE(__LINE__) 이렇게 한 이유는
 *	__pragma(message(__FILE__ "(" __LINE__ "): [ " __func__ " ] [ TODO ] " msg)) 이렇게 하면
 *	컴파일 할때 출력이 되지 않습니다. 그래서
 *	__pragma(message(__FILE__ "(" STRINGIZE(__LINE__) "): [ " __func__ " ] [ TODO ] " msg)) 이렇게 해보니까
 *	앞에 cpp 나오고 (__LINE__) << 이렇게 출력이 됩니다. 그래서
 *	__pragma(message(__FILE__ "(" STRINGNET(__LINE__) "): [ " __func__ " ] [ TODO ] " msg))
 *	이렇게 했습니다.
*/

/*
 *	사용법
 *	PRAGMA_TODO("TEST")
 *	#pragma message(TODO "TEST")
 *	
 *	두 가지 방법이 있습니다.
 *	마음에 드는 방법으로 사용하시면 됩니다.
 *	pragma message 는 complie time 때 사용되기 때문에 run time 중에는 아무런 영향이 없습니다.
 *	
 *	__FILE__ "(" STRINGNET(__LINE__) "): << 이 구문이 Output 창에서 더블 클릭했을때 점프가 가능하게 해주는 구문입니다.
*/

#define STRINGIZE(l) #l
#define STRINGNET(l) STRINGIZE(l)

#define TODO __FILE__ "(" STRINGNET(__LINE__) "): [ " __FUNCTION__ " ] [ TODO ] "

#define TODONAME(name) __FILE__ "(" STRINGNET(__LINE__) "): [ " __FUNCTION__ " ] " "[ " name " ] "

#define PRAGMA_TODO(msg) __pragma(message(__FILE__ "(" STRINGNET(__LINE__) "): [ " __FUNCTION__ " ] [ TODO ] " msg))

static const char* G_SERVERNAME[E_SERVER_TYPE::SERVER_TYPE_MAX] =
{
	"BASE_SERVER",//BASE_SERVER = 0,
	"FRONT_SERVER",//FRONT_SERVER = 1,
	"OOPS_SERVER",//OOPS_SERVER = 2,
	"PLATFORM_SERVER", //PLATFORM_SERVER = 3
	"LOBBY_SERVER",//LOBBY_SERVER = 4,
	"SLOT_SERVER",//SLOT_SERVER = 5,
	"DB_SERVER",//DB_SERVER = 6,
	"REDIS_DB",//REDIS_DB = 7
};

static const char* G_SERVERSTAGE[E_SERVER_STAGE::E_SERVER_STAGE_MAX] =
{
	"LOCAL",//LOCAL = 1,
	"DEV",//DEV = 2,
	"ALPHA",//ALPHA = 3,
	"LIVE", //LIVE = 4
};

static const char* G_SESSION_NAME[Sessions::SESSION_MAX] =
{
	"",
	"Session_None",
	"Session_Server",
	"Session_Client",
	"Session_Dummy",
	"Session_Agent",
	"Session_Tool",
	"Session_QA_Tool",
};

static const char* G_CONTEXT_TYPE[E_CONTEXT_TYPE::E_CONTEXT_MAX] =
{
	"E_CONTEXT_NONE",
	"E_CONTEXT_CLIENT",
	"E_CONTEXT_SERVER"
};

static const char* G_CONNECTOR_STATUS[E_IOCP_CONNECTOR_STATUS::E_IOCP_CONNECTOR_STATUS_MAX] =
{
	"E_IOCP_CONNECTOR_STATUS_NONE",
	"E_IOCP_CONNECTOR_STATUS_TRY_CONNECT",
	"E_IOCP_CONNECTOR_STATUS_DISCONNECTED",
	"E_IOCP_CONNECTOR_STATUS_CONNECTED"
};