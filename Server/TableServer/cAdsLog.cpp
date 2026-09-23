#include "cClientSession.h"

#include "../Include/Netlib/Common/cInterfaceIocpContext.h"
#include "../Include/Netlib/IOCP/cIocpContext.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"

#include "cAdsLog.h"
#include "Query.h"

#include <future>

cAdsLog::~cAdsLog()
{
	AfterLogs();

	// 광고 로그 저장
	WriteLog();
}

void cAdsLog::BeforeLogs()
{
	if ( m_pClientSession == nullptr ) return;

}

void cAdsLog::AfterLogs()
{
	if ( m_pClientSession == nullptr ) return;


}

void cAdsLog::WriteLog()
{
	if ( m_pClientSession == nullptr ) return;

	const General::ParticipantProfile&  player = m_pClientSession->GetPlayer();

	auto context = m_pClientSession->GetContext();
	std::string ipaddr = m_pClientSession->GetIp(); // context != nullptr ? ::ConvertIP( context->GetIP() ) : "";


	// GoldenTicket AdReward
	switch ( m_code )
	{
	case 70101: // 광고 시청 시작
	{
		std::future<BOOL> insertAdsLogResult = QueryManager::InsertAdsLog(
			m_code , 								// int code ,                 	 // 로그 코드
			m_pClientSession->GetPlatformGuid() , 	// const std::string & uid ,     // UID
			"" , 									// const std::string & cmd ,     // 시퀀스
			ipaddr , 								// IP 정보
			m_adType , 								// const std::string & type ,    // 광고 타입(위치)
			m_pClientSession->GetGameVersion() ); 	// const std::string & ver ,     // 접속한 게임 버전

		insertAdsLogResult.wait();
	}
	break;
	case 70102: // 광고 종료
	{
		std::future<BOOL> insertAdsLogResult = QueryManager::InsertAdsLog(
			m_code ,								// int code ,                  	// 로그 코드
			m_pClientSession->GetPlatformGuid() , 	// const std::string & uid ,    // UID
			"" , 									// const std::string & cmd ,    // 시퀀스
			ipaddr , 								// IP 정보
			m_adType , 								// const std::string & type ,   // 광고 타입(위치)
			m_pClientSession->GetGameVersion() ); 	// const std::string & ver ,    // 접속한 게임 버전

		insertAdsLogResult.wait();
	}
	break;
	}
}