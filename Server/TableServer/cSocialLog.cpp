#include "cClientSession.h"

#include "../Include/Netlib/Common/cInterfaceIocpContext.h"
#include "../Include/Netlib/IOCP/cIocpContext.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"

#include "cSocialLog.h"
#include "Query.h"

#include <future>

cSocialLog::~cSocialLog()
{
	AfterLogs();

	// 소셜 로그 저장
	WriteLog();
}

void cSocialLog::BeforeLogs()
{
	if ( m_pClientSession == nullptr ) return;


}

void cSocialLog::AfterLogs()
{
	if ( m_pClientSession == nullptr ) return;


}

void cSocialLog::WriteLog()
{
	if ( m_pClientSession == nullptr ) return;

	const General::ParticipantProfile&  player = m_pClientSession->GetPlayer();

	auto context = m_pClientSession->GetContext();
	std::string ipaddr = m_pClientSession->GetIp(); //context != nullptr ? ::ConvertIP( context->GetIP() ) : "";

	switch ( m_code )
	{
	case 50101: // 친구 신청
	{
		std::future<BOOL> insertSocialLogResult = QueryManager::InsertSocialLog(
			m_code , 								// int code ,                  		// 로그 코드
			m_pClientSession->GetPlatformGuid() , 	// const std::string & uid ,        // UID
			"" , 									// const std::string & cmd ,        // 시퀀스
			ipaddr , 								// IP 정보
			m_friend_id , 							// const std::string & friend_id ,  // 친구의 UID
			m_log_data , 							// const std::string & logdata ,    // 친구 데이터
			m_pClientSession->GetGameVersion() ); 	// const std::string & ver ,        // 접속한 게임 버전
	
		insertSocialLogResult.wait();
	}
	break;
	case 50102: // 친구 삭제
	{
		std::future<BOOL> insertSocialLogResult = QueryManager::InsertSocialLog(
			m_code , 								// int code ,                  		// 로그 코드
			m_pClientSession->GetPlatformGuid() , 	// const std::string & uid ,        // UID
			"" , 									// const std::string & cmd ,        // 시퀀스
			ipaddr , 								// IP 정보
			m_friend_id , 							// const std::string & friend_id ,  // 친구의 UID
			m_log_data , 							// const std::string & logdata ,    // 친구 데이터
			m_pClientSession->GetGameVersion() ); 	// const std::string & ver ,        // 접속한 게임 버전

		insertSocialLogResult.wait();
	}
	break;
	}
}