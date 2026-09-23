#include "cSystemStub.h"

#include "..\Include\Netlib\Common/cSingleton.h"
#include "..\Include\Netlib\IOCP/cIocpConnector.h"
#include "..\Include\Netlib\Network/cContextPooler.h"
#include "..\Include\Netlib\Manager/cThreadManager.h"
#include "..\Include\Netlib\Manager/cCommandQueueManager.h"
#include "..\Include\Netlib\Manager/ServerManager.h"
#include "..\Include\Netlib\Manager/cSessionManager.h"
#include "..\Include\Netlib\Manager/cWebServerManager.h"
#include "..\Include\Netlib\Queue/cCommandQueue.h"
#include "..\Include\Netlib\Queue/cLogQueue.h"
#include "..\Include\Netlib\Queue/cWebQueue.h"
#include "..\Include\Netlib\Session/cSession.h"

#include "..\Include\Netlib\Scheduler\cScheduler.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

#include "cInstancePacketParser.h"
#include "cGameRoomManager.h"
#include "cProtoUtil.h"
#include "cMaintenanceManager.h"
#include "cClientSession.h"
#include "cFriendManager.h"
#include "cConfigReader.h"
#include "cDataLoader.h"
#include "cLoginEventManager.h"
#include "cGameVersionChecker.h"

cSystemStub::cSystemStub()
{
}


cSystemStub::~cSystemStub()
{
}

// UINT /*nID*/, UINT /*nCommand*/, BYTE* /*pData*/, UINT /*nLength*/, UINT /*nThreadIndex*/
void cSystemStub::bindmethod( std::function<void( UINT , UINT , BYTE* , UINT , UINT )>* fp )
{
	if ( fp == nullptr )
	{
		return;
	}

	if ( fp[ CSNet::ProtocolCommand::SYS_SCHEDULE_MSG ] != nullptr )
	{
		assert( false && "cScheduleStub::bindmethod" );
	}

	fp[ CSNet::ProtocolCommand::SYS_SCHEDULE_MSG ] = std::bind( &cSystemStub::SYS_SCHEDULE_MSG_FP , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
	fp[ CSNet::ProtocolCommand::SYS_NET_SESSION_LOG_OUT ] = std::bind( &cSystemStub::SYS_NET_SESSION_LOG_OUT , std::ref( *this ) , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 , std::placeholders::_4 , std::placeholders::_5 );
}

void cSystemStub::SYS_NET_SESSION_LOG_OUT( UINT nID , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex )
{
	if ( nLength != sizeof( Sys_Net_Session_Log_Out_Data ) )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cScheduleStub::SYS_NET_SESSION_LOG_OUT Failed. nLength != sizeof(Sys_Net_Session_Log_Out_Data)" );
		return;
	}

	Sys_Net_Session_Log_Out_Data* pPtr = reinterpret_cast< Sys_Net_Session_Log_Out_Data* >( pData );

	NetLib::cSessionManager* pSessionManager = NetLib::cSingleton<NetLib::cSessionManager>::ExistsInstance();
	if ( pSessionManager == nullptr )
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "cScheduleStub::SYS_NET_SESSION_LOG_OUT Failed. SessionManager is nullptr" );
		return;
	}

	pSessionManager->RemovePending( pPtr->llAllocatedSessionSlot );

	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_INFO , "cScheduleStub::SYS_NET_SESSION_LOG_OUT Success. AccountIDX[ %I64d ]" , pPtr->llAllocatedSessionSlot );
}

void cSystemStub::SYS_SCHEDULE_MSG_FP(UINT nID, UINT nCommand, BYTE* pData, UINT nLength, UINT nThreadIndex)
{
#ifdef USING_MULTI_THREAD
	NetLib::cCommandQueueManager* pCommandQueueManager = NetLib::cSingleton<NetLib::cCommandQueueManager>::ExistsInstance();
	if (pCommandQueueManager == nullptr)
		return;

	const int nCommandQueueCnt = pCommandQueueManager->GetCommandQueueCnt();
	const int nServerCommandQueueArray = pCommandQueueManager->GetServerCommandQueueArray();
#endif
	stSchedule Schedule;
	memcpy(&Schedule, pData, sizeof(stSchedule));

	NetLib::ServerManager* pServerManager = NetLib::cSingleton<NetLib::ServerManager>::ExistsInstance();
	if (pServerManager == nullptr)
	{
		return;
	}

	//switch (Schedule.dwMSec)
	switch (Schedule.uTimerID)
	{
	case CSDef::E_TIMER_DEFINE::E_SERVER_INFO_CEHCK_TIMER:
		{
#ifdef USING_MULTI_THREAD
			// 마지막 CommandThread 에서만 처리
			if (1 < nCommandQueueCnt && nServerCommandQueueArray - 1 != Schedule.uTargetCommandThreadIndex)
				return;
#endif
			pServerManager->ServerStatusReport();

			NetLib::cIocpConnector* pIocpConnector = NetLib::cSingleton<NetLib::cIocpConnector>::ExistsInstance();
			if (pIocpConnector)
			{
				pIocpConnector->KeepConnect();
			}
		}
		break;
	case CSDef::E_TIMER_DEFINE::E_SQL_QUERY:
		{
			QueryManager::PingSQL();
		}
		break;
	case CSDef::E_TIMER_DEFINE::E_PING_CHECK:
		{
			// PING 체크
			NetLib::cSingleton<NetLib::cContextPooler>::GetInstance()->AliveContextCheck(nThreadIndex);

			NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->CheckPendingSession();
		}
		break;
	case CSDef::E_TIMER_DEFINE::E_1_SEC:
		{
			// 쓰레드 살아 있음 용도용
			stThreadMonitor* pThreadMonitor =
				NetLib::cSingleton<NetLib::cThreadManager>::GetInstance()->GetThreadMonitor(E_SERVER_THREAD_TYPE::E_SERVER_THREAD_TYPE_COMMAND, nThreadIndex);

			pThreadMonitor->UpdateTick();

#ifdef USING_MULTI_THREAD
			// 마지막 CommandThread 에서만 처리
			if (1 < nCommandQueueCnt && nServerCommandQueueArray - 1 != Schedule.uTargetCommandThreadIndex)
				return;
#endif
		}
		break;
	case CSDef::E_TIMER_DEFINE::E_UPDATE_HERO_SESSION_EXPIRE:
		{
			//cScheduleStub::UpdateHeroSessionExpire(nThreadIndex);
			
#ifdef USING_MULTI_THREAD
			// 마지막 CommandThread 에서만 처리
			if (1 < nCommandQueueCnt && nServerCommandQueueArray - 1 != Schedule.uTargetCommandThreadIndex)
				return;
#endif

			//NetLib::cSingleton<NetLib::cWebQueue>::GetInstance()->PushCommand((UINT)0, ServerCommon::SERVER_MSG_ID::SERVER_MSG_UPDATE_USER_COUNT);
		}
		break;
	case CSDef::E_TIMER_DEFINE::E_SERVER_INIT:
		{
#ifdef USING_MULTI_THREAD
			// 마지막 CommandThread 에서만 처리
			if (1 < nCommandQueueCnt && nServerCommandQueueArray - 1 != Schedule.uTargetCommandThreadIndex)
				return;
#endif

			// 버젼 체커 로딩
			NetLib::cSingleton<cInstancePacketParser>::GetInstance()->InitializeGameVersionChecker();

			// 채팅서버 최초에 1번 실행될 작업들을 여기에서 처리 합니다.
			// 서버가 시작되자 마자, 현재 접속한 인원을 셋팅합니다.
			//NetLib::cSingleton<NetLib::cWebQueue>::GetInstance()->PushCommand((UINT)0, ServerCommon::SERVER_MSG_ID::SERVER_MSG_UPDATE_USER_COUNT);

			// Initialize Login Event Cache
			NetLib::cSingleton<cLoginEventManager>::GetInstance()->Initialize();

			// DB 에서 처리할 라이브러리 미확정 문제로 코드 주석처리
			/*
			std::vector<stWebConnectionInfo*> web_servers = NetLib::cSingleton<cDBManager>::GetInstance()->SelectServerConfig();
			if (web_servers.size() < 1)
			{
				::MessageBox(NULL, _T("WebServerCount 0, Please Check GlobalDB ServerConfig Table Then add web servers"), _T("No WebServer"), MB_ICONEXCLAMATION);
				exit(0);
				//throw("WebServerCount 0, Please Check GlobalDB ServerConfig Table Then add web servers");
			}
			NetLib::cSingleton<NetLib::cWebServerManager>::GetInstance()->InitWebServers(web_servers);
			*/

		}
		break;
	case CSDef::E_TIMER_DEFINE::E_SERVER_ALIVE_CHECK:
		{
			// 서버 AliveChecking
			//NetLib::cSingleton<NetLib::cWebQueue>::GetInstance()->PushCommand(static_cast<UINT>(0), ServerCommon::SERVER_MSG_ALIVE_CHECK);
			//손실한도 킥아웃
			/*std::string t_lost_ci = NetLib::cSingleton<cMaintenanceManager>::GetInstance()->PopBackLostLimit();
			if ( t_lost_ci != "" )
			{
				NetLib::cContextPooler* contextPooler = NetLib::cSingleton<NetLib::cContextPooler>::ExistsInstance();
				POSITION startPos = contextPooler->GetStartPosition( nThreadIndex );
				if ( startPos != nullptr ) {

					while ( startPos )
					{
						NetLib::cIocpContext* pContext = NetLib::cSingleton<NetLib::cContextPooler>::GetInstance()->GetValue( startPos );
						auto session = pContext->GetSession();
						if ( session != nullptr && session->GetSessionType() == Sessions::SESSION_CLIENT ) {

							cClientSession* pClientSession = static_cast< cClientSession* >( session );

							if ( pClientSession->GetAccountGuid() == t_lost_ci )
							{
								std::u8string operation_message = u8"손실한도가 초과되었습니다.";
								std::string_view operation_message_utf8View( reinterpret_cast< const char* >( operation_message.data() ) , operation_message.size() );

								PmNet::ServiceNotice system_message;
								system_message.set_notice( operation_message_utf8View.data() );
								system_message.set_expel_flag( true );


								std::string errorString;
								pClientSession->SendRequest( General::PacketID::Packet_ServiceNotice , system_message , General::ResultCode::Result_Success , errorString );

								pClientSession->SessionLogout( pContext->GetEntity() );

							}
						}
						contextPooler->GetNextPosition( startPos , nThreadIndex );
					}
				}
			}*/


// 강제 메세지
			if ( NetLib::cSingleton<cMaintenanceManager>::GetInstance()->ReadMaintenance_ForceMessage() )
			{
				// 점검 공지가 있다. 시간은 쿼리단에서 확인함으로 현재 걸려 있는 점검이라고 생각하면 된다.

				// 마켓 별로 확인해서 버젼에 해당 하지 않으면 접속을 끊어낸다.
				const Server::MaintenanceMessage& systemMessage = NetLib::cSingleton<cMaintenanceManager>::GetInstance()->GetCurrentMaintenance_ForcedMessage();

				std::vector<General::StoreChannel> markets;

				if ( systemMessage.play_store() )
					markets.push_back( General::StoreChannel::StoreChannel_GooglePlay );

				if ( systemMessage.app_store() )
					markets.push_back( General::StoreChannel::StoreChannel_AppleAppStore );

				if ( systemMessage.one_store() )
					markets.push_back( General::StoreChannel::StoreChannel_OneStore );

				if ( systemMessage.pc() )
					markets.push_back( General::StoreChannel::StoreChannel_PC );

				// 해당 마켓이 없으면 처리 하지 않는다.
				if ( markets.size() == 0 )
					return;

				NetLib::cContextPooler* contextPooler = NetLib::cSingleton<NetLib::cContextPooler>::ExistsInstance();
				if ( contextPooler == nullptr )
					return;

				POSITION startPos = contextPooler->GetStartPosition( nThreadIndex );
				if ( startPos != nullptr ) {

					while ( startPos )
					{
						NetLib::cIocpContext* pContext = NetLib::cSingleton<NetLib::cContextPooler>::GetInstance()->GetValue( startPos );
						auto session = pContext->GetSession();
						if ( session != nullptr && session->GetSessionType() == Sessions::SESSION_CLIENT ) {

							cClientSession* pClientSession = static_cast< cClientSession* >( session );

							// 해당하는 마켓의 클라이언트만 끊어낸다.
							for ( const auto& market : markets ) {

								if ( market == pClientSession->GetMarket() )
								{
									const Server::MaintenanceMessage& message = NetLib::cSingleton<cMaintenanceManager>::GetInstance()->GetCurrentMaintenance_ForcedMessage();
									std::string errorString;
									PmNet::MandatoryNotice forced_message;
									forced_message.set_headline( message.title() );
									forced_message.set_notice( message.message() );
									pClientSession->SendRequest( General::PacketID::Packet_PriorityNotice , forced_message , General::ResultCode::Result_Success , errorString );
								}
							}

							// 버젼비교는 cGameVersionChecker::VersionStringToInt
						}

						contextPooler->GetNextPosition( startPos , nThreadIndex );
					}
				}
			}

			// 시스템 메세지
			if ( NetLib::cSingleton<cMaintenanceManager>::GetInstance()->ReadMaintenance_SystemMessage() )
			{
				// 점검 공지가 있다. 시간은 쿼리단에서 확인함으로 현재 걸려 있는 점검이라고 생각하면 된다.

				// 마켓 별로 확인해서 버젼에 해당 하지 않으면 접속을 끊어낸다.
				const Server::MaintenanceMessage& systemMessage = NetLib::cSingleton<cMaintenanceManager>::GetInstance()->GetCurrentMaintenance_SystemMessage();

				std::vector<General::StoreChannel> markets;

				if ( systemMessage.play_store() )
					markets.push_back( General::StoreChannel::StoreChannel_GooglePlay );

				if ( systemMessage.app_store() )
					markets.push_back( General::StoreChannel::StoreChannel_AppleAppStore );

				if ( systemMessage.one_store() )
					markets.push_back( General::StoreChannel::StoreChannel_OneStore );

				if ( systemMessage.pc() )
					markets.push_back( General::StoreChannel::StoreChannel_PC );

				// 해당 마켓이 없으면 처리 하지 않는다.
				if ( markets.size() == 0 )
					return;

				NetLib::cContextPooler* contextPooler = NetLib::cSingleton<NetLib::cContextPooler>::ExistsInstance();
				if ( contextPooler == nullptr )
					return;

				POSITION startPos = contextPooler->GetStartPosition( nThreadIndex );
				if ( startPos != nullptr ) {

					while ( startPos )
					{
						NetLib::cIocpContext* pContext = NetLib::cSingleton<NetLib::cContextPooler>::GetInstance()->GetValue( startPos );
						auto session = pContext->GetSession();
						if ( session != nullptr && session->GetSessionType() == Sessions::SESSION_CLIENT ) {

							cClientSession* pClientSession = static_cast< cClientSession* >( session );

							// 해당하는 마켓의 클라이언트만 끊어낸다.
							for ( const auto& market : markets ) {

								if ( market == pClientSession->GetMarket() )
								{
									const Server::MaintenanceMessage& message = NetLib::cSingleton<cMaintenanceManager>::GetInstance()->GetCurrentMaintenance_SystemMessage();
									std::string errorString;
									PmNet::ServiceNotice system_message;
									system_message.set_headline( message.title() );
									system_message.set_notice( message.message() );
									system_message.set_bulletin_kind( General::AnnouncementKind::Announcement_System );
									pClientSession->SendRequest( General::PacketID::Packet_ServiceNotice , system_message , General::ResultCode::Result_Success , errorString );
								}
							}

							// 버젼비교는 cGameVersionChecker::VersionStringToInt
						}

						contextPooler->GetNextPosition( startPos , nThreadIndex );
					}
				}
			}

			// 점검 공지가 있는지 확인한다.
			// 각 서버들마다 따로 관리하도록한다.
			if ( NetLib::cSingleton<cMaintenanceManager>::GetInstance()->ReadMaintenance() ) {

				// 점검 공지가 있다. 시간은 쿼리단에서 확인함으로 현재 걸려 있는 점검이라고 생각하면 된다.
				
				// 마켓 별로 확인해서 버젼에 해당 하지 않으면 접속을 끊어낸다.
				const Server::MaintenanceMessage& message = NetLib::cSingleton<cMaintenanceManager>::GetInstance()->GetCurrentMaintenance();

				std::vector<General::StoreChannel> markets;

				if ( message.play_store() )
					markets.push_back( General::StoreChannel::StoreChannel_GooglePlay );

				if ( message.app_store() )
					markets.push_back( General::StoreChannel::StoreChannel_AppleAppStore );

				if ( message.one_store() )
					markets.push_back( General::StoreChannel::StoreChannel_OneStore );

				if ( message.pc() )
					markets.push_back( General::StoreChannel::StoreChannel_PC );

				// 해당 마켓이 없으면 처리 하지 않는다.
				if ( markets.size() == 0 )
					return;

				int i_version_min = NetLib::cSingleton<cGameVersionChecker>::GetInstance()->VersionStringToInt( message.version_min() );
				int i_version_max = NetLib::cSingleton<cGameVersionChecker>::GetInstance()->VersionStringToInt( message.version_max() );

				NetLib::cContextPooler* contextPooler = NetLib::cSingleton<NetLib::cContextPooler>::ExistsInstance();
				if ( contextPooler == nullptr )
					return;

				POSITION startPos = contextPooler->GetStartPosition( nThreadIndex );
				if ( startPos != nullptr ) {

					while ( startPos )
					{
						NetLib::cIocpContext* pContext = NetLib::cSingleton<NetLib::cContextPooler>::GetInstance()->GetValue( startPos );
						auto session = pContext->GetSession();
						if ( session != nullptr && session->GetSessionType() == Sessions::SESSION_CLIENT ) {


							cClientSession* pClientSession = static_cast< cClientSession* >( session );
							if ( NetLib::cSingleton<cMaintenanceManager>::GetInstance()->CheckWhiteList( pClientSession->GetIp() ) )
							{
								contextPooler->GetNextPosition( startPos , nThreadIndex );
								continue;
							}
							// 해당하는 마켓의 클라이언트만 끊어낸다.
							for ( const auto& market : markets ) {

								if ( market == pClientSession->GetMarket() ) {

									if ( false == NetLib::cSingleton<cMaintenanceManager>::GetInstance()->CheckMaintenanceVersion( i_version_min , i_version_max , pClientSession->GetGameVersion() ) )
										continue;

									if( pClientSession->GetJoinedRoomNumber() == 0 )
									{
										
										const Server::MaintenanceMessage& message = NetLib::cSingleton<cMaintenanceManager>::GetInstance()->GetCurrentMaintenance();
										std::string errorString;
										PmNet::ServiceNotice system_message;
										system_message.set_notice( message.message() );
										pClientSession->SendRequest( General::PacketID::Packet_ServiceNotice , system_message , General::ResultCode::Result_ServicePaused , errorString );
										pClientSession->SessionLogout( pContext->GetEntity() );
										pClientSession->DisConnectContext( pContext->GetEntity() , nThreadIndex );
										if ( pContext->GetContextType() == E_CONTEXT_TYPE::E_CONTEXT_SERVER )
										{
											if ( FALSE == NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->DeRegisterServer( static_cast< BYTE >( E_SERVER_TYPE::LOBBY_SERVER ) , pContext->GetAllocateSlot() ) )
											{
												NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI ,
													_T( "cProtoMsgStub::SERVER_MSG_REQ_AUTHENTICATION Connector DeRegisterServer Failed. GetAllocateSlot %I64d" , pContext->GetAllocateSlot() ) );
											}
										}

										pContext->SetSession( nullptr );
										pContext->Disconnect();
									}
									else
									{
										NetLib::cSingleton<cGameRoomManager>::GetInstance()->UserGameRoomOut( pClientSession );
									}
								}
							}

							
							// 버젼비교는 cGameVersionChecker::VersionStringToInt

						}

						contextPooler->GetNextPosition( startPos , nThreadIndex );
					}
				}


			}

			// 이미지 메시지 읽기
			NetLib::cSingleton<cMaintenanceManager>::GetInstance()->ReadMaintenance_ImageMessage();

			// check pinball_ratio
			/*if ( nullptr != NetLib::cSingleton<cDataLoader>::ExistsInstance() && true == cDataLoader::LoadingEnd )
			{
				std::future<BOOL> result = QueryManager::GetPInballRatioAsync( NetLib::cSingleton<cDataLoader>::GetInstance()->GetPinballRatio() );
				result.wait();

				if ( FALSE == result.get() ) {
				}
			}*/
		}
		break;
	case CSDef::E_TIMER_DEFINE::E_PING_REQUEST:
		{
			NetLib::cContextPooler* contextPooler = NetLib::cSingleton<NetLib::cContextPooler>::ExistsInstance();
			if (contextPooler == nullptr)
				return;

			POSITION startPos = contextPooler->GetStartPosition(nThreadIndex);
			if ( startPos != nullptr ) {

				while ( startPos )
				{
					NetLib::cIocpContext* pContext = NetLib::cSingleton<NetLib::cContextPooler>::GetInstance()->GetValue( startPos );
					E_CONNECT_STATUS eStatus = pContext->GetContextStatus();
					if ( eStatus == E_CONNECT_STATUS::E_CONNECT_CONNECTED )
					{
						// 클라이언트 세션만 대상 — 서버 세션은 아래 별도 경로(Packet_NodeHeartbeat)로 처리
						auto pSession = pContext->GetSession();
						if ( pSession != nullptr && pSession->GetSessionType() == Sessions::SESSION_CLIENT )
						{
							PmNet::HeartbeatRQ req;
							req.set_hb_val( ::GetTickCount64() );
							GetOwner()->SendBuffer( pContext , nThreadIndex , General::PacketID::Packet_LinkProbe , req , General::ResultCode::Result_Success , "" );
						}
					}

					contextPooler->GetNextPosition( startPos , nThreadIndex );
				}
			}

			// IOCP 커넥터로 접속한 서버들에게 핑을 보낸다.
			// 보내지 않으면 세션 정리 당한다.!
			// 서버 핑: Packet_NodeHeartbeat — DispatchTable 의 S2S 제외 범위라 protobuf 그대로 유지
			{
				PmNet::HeartbeatRS response;
				response.set_hb_val( ::GetTickCount64() );

				GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTO_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer( nThreadIndex );
				pGOOGLE_PROTO_BUFFER->Clear();
				if ( response.SerializeToArray( pGOOGLE_PROTO_BUFFER->SerializeBuffer , sizeof( pGOOGLE_PROTO_BUFFER->SerializeBuffer ) ) == false ) return;

				NetLib::cSingleton<NetLib::cIocpConnector>::GetInstance()->TargetAllSendPacket( E_SERVER_TYPE::LOBBY_SERVER , General::Packet_NodeHeartbeat , pGOOGLE_PROTO_BUFFER->SerializeBuffer , response.ByteSizeLong() );
			}
		}
		break;
	case CSDef::E_TIMER_DEFINE::E_FRAME_SYNC:
		{
			// 초당 30 프레임 처리용
			// 쓰레드 마다 게임 Tick 처리를 한다.
			NetLib::cSingleton<cGameRoomManager>::GetInstance()->Process( nThreadIndex );

			//NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_GRADE::LOG_CRI , "schedule thread check thread id : %d", nThreadIndex );
		}
		break;
	case CSDef::E_TIMER_DEFINE::E_SESSION_CLEAR:
	{
		NetLib::cSingleton<NetLib::cSessionManager>::GetInstance()->ClearSession();
	}
	break;
	case CSDef::E_TIMER_DEFINE::E_CONCURRENT_USERS_LOG:
	{
		const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();
		if ( 1 != configReader->SID_FOR_MANAGE )
			break;

		std::map<std::string , int> channelUserCount;
		//std::vector<std::string> channelID = { "Online", "LowBadugi_Chip_10F", "LowBadugi_Coin_10F", "LowBadugi_Chip", "LowBadugi_Coin", "Holdem", "Baccara", "Blackjack", "1", "Roulette" };
		std::vector<std::string> channelID = { "Online", "LowBadugi_Chip_10F", "LowBadugi_Coin_10F", "LowBadugi_Chip", "LowBadugi_Coin", "Holdem", "Baccara", "Blackjack", "1" };

		NetLib::cSingleton<cFriendManager>::GetInstance()->GetChannelUserCount( channelID , channelUserCount );

		auto log_concurrent_user = QueryManager::InsertGameConcurrentUsersLog(
			channelUserCount[ "Online" ] ,
			channelUserCount[ "LowBadugi_Chip" ] ,
			channelUserCount[ "LowBadugi_Coin" ] ,
			channelUserCount[ "LowBadugi_Chip_10F" ] + channelUserCount[ "LowBadugi_Coin_10F" ] ,
			channelUserCount[ "Holdem" ] ,
			channelUserCount[ "Baccara" ] ,
			channelUserCount[ "Blackjack" ] ,
			channelUserCount[ "1" ] 
			//channelUserCount[ "Roulette" ] 
			);
		 
		log_concurrent_user.wait();

		auto slot_log_concurrent_user = QueryManager::InsertSlotConcurrentUsersLog( channelUserCount );

		slot_log_concurrent_user.wait();
	}

	break;
	case CSDef::E_TIMER_DEFINE::E_SLOTLOCK_SYNC:
	{

		if ( cDataLoader::LoadingEnd && !cDataLoader::SystemLoadingEnd )
		{
			auto systemdataload = QueryManager::GetSystemDataAsync( NetLib::cSingleton<cDataLoader>::GetInstance()->GetSystemDataRef() );
			systemdataload.wait();

			cDataLoader::SystemLoadingEnd = true;
		}

		
	}
	break;
	case CSDef::E_TIMER_DEFINE::E_5_SEC:
	{
		
		// Login Event Cache Update (every 60 seconds = 12 * 5 seconds)
		static int loginEventCacheCounter = 0;
		loginEventCacheCounter++;
		if (loginEventCacheCounter >= 12) // 60 seconds
		{
			loginEventCacheCounter = 0;
			NetLib::cSingleton<cLoginEventManager>::GetInstance()->UpdateLoginEventList();
		}
	}
	break;
	}
}