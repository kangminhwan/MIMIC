#include "TableServerHeader.h"
#include "TableServerMain.h"
#include "cConfigReader.h"
#include "cProtoUtil.h"
#include "cInstancePacketParser.h"
#include "cGameRoomManager.h"
#include "cCardDeck.h"
#include "cDataLoader.h"
#include "MadeSlackNotification.h"

#include "../include/Netlib/Manager/ServerManager.h"
#include "../include/Netlib/Manager/cThreadManager.h"
#include "../Include/Netlib/Common/cSingleton.h"
#include "../Include/Netlib/Scheduler/cScheduler.h"
#include "../Include/Netlib/FileLoader/csvFileLoader.h"
#include "../Include/Netlib/Thread/cLogThread.h"
#include "../Include/Netlib/Thread/cCommandThread.h"
#include "../Include/Netlib/Alert/cInterfaceAlert.h"
#include "../Include/Netlib/Manager/cLogManager.h"
#include "../Include/Netlib/Redis/cRedisManager.h"
#include "../Include/Netlib/Network/cContextPooler.h"
#include "../Include/Netlib/IOCP/cIocpConnector.h"
#include "../Include/Netlib/IOCP/cIocpContext.h"
#include "../Include/Netlib/Thread/cThreadPool.h"

#include "./mysql/cMySQLConnectionPooler.h"
#include "./mysql/cMySQL.h"

#ifdef _DEBUG

#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

#include "Query.h"
#include "cProtoMsgStub.h"
#include "StringUtil.h"
#include "MadePlatform.h"
#include "cRedisController.h"
#endif

TableServerMain::TableServerMain()
{
	NetLib::ServerManager* pServerManager = NetLib::cSingleton<NetLib::ServerManager>::GetInstance();
	if (pServerManager == nullptr)
	{
		assert(false && "TableServerMain Constructor is Failed. ServerManager is nullptr");
	}
	pServerManager->SocketInit();
}


TableServerMain::~TableServerMain()
{
	DestroyServerResources();
	//if (m_bUseConsole)
	{
		FreeConsole();
	}
	NetLib::cSingleton<MadeSlackNotification>::DeleteInstance();
	NetLib::cSingleton<NetLib::ServerManager>::DeleteInstance();
	//NetLib::cSingleton<cPinballManager>::DeleteInstance();
	NetLib::cSingleton<cGameRoomManager>::DeleteInstance();
}

BOOL TableServerMain::StartServer()
{
	//std::cout << "double의 최대값: " << DBL_MAX << std::endl;
	//std::cout << "double의 최대정수값: " << static_cast<uint64>(DBL_MAX) << std::endl;

	std::wcout.imbue( std::locale( "" ) );

	NetLib::ServerManager* pServerManager = NetLib::cSingleton<NetLib::ServerManager>::ExistsInstance();
	if (!pServerManager)
	{
		return FALSE;
	}

	NetLib::cSingleton<cConfigReader>::GetInstance()->readXml();

	if (!pServerManager->IsSereverStarted())
	{
		pServerManager->SetConfiguration(NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig());

		NetLib::cInterfacePacketParser* pPacketParser = NetLib::cSingleton<cInstancePacketParser>::GetInstance();
		pServerManager->SetPacketParserPtr(pPacketParser);

		pServerManager->CreateConfigurations();

		//pServerManager->SetDlgSendMessagePtr(reinterpret_cast<cInterfaceDlgSendMessage*>(this));

		pServerManager->ServerStart();
		
		// NetLib 에서 생성하는 자원이 아닌, 각 서버 프로젝트에 Resource 들을 초기화 합니다.
		this->InitServerResources();

		tstring web_ip = GetDomainToIP(NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig()->webserver.szDNS);

		StringCbCopy(NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig()->webserver.szIP, sizeof(cConfigReader::m_tServerConfiguration.webserver.szIP), web_ip.c_str());


		NetLib::cSingleton<NetLib::cScheduler>::GetInstance()->AddShceduleEvent(CSDef::E_TIMER_DEFINE::E_SERVER_INFO_CEHCK_TIMER, CSDef::E_TIMER_INTERVAL::E_5_SECOND, false, false);
		NetLib::cSingleton<NetLib::cScheduler>::GetInstance()->AddShceduleEvent(CSDef::E_TIMER_DEFINE::E_PING_CHECK, CSDef::E_TIMER_INTERVAL::E_TIMER_INTERVAL_PING_CHECK , false, false);
		//NetLib::cSingleton<NetLib::cScheduler>::GetInstance()->AddShceduleEvent( CSDef::E_TIMER_DEFINE::E_FRAME_SYNC , CSDef::E_TIMER_INTERVAL::E_33_MILLISECOND , false , false );
		NetLib::cSingleton<NetLib::cScheduler>::GetInstance()->AddShceduleEvent( CSDef::E_TIMER_DEFINE::E_FRAME_SYNC , CSDef::E_TIMER_INTERVAL::E_100_MILLISECOND , false , false );
		NetLib::cSingleton<NetLib::cScheduler>::GetInstance()->AddShceduleEvent( CSDef::E_TIMER_DEFINE::E_SQL_QUERY , CSDef::E_TIMER_INTERVAL::E_1_HOUR , false , false );

		// 1초당 처리할 작업들 처리용 타이머
		NetLib::cSingleton<NetLib::cScheduler>::GetInstance()->AddShceduleEvent( CSDef::E_TIMER_DEFINE::E_1_SEC , CSDef::E_TIMER_INTERVAL::E_1_SECOND , false , false );

		// 5초당 처리할 작업들 처리용 타이머
		NetLib::cSingleton<NetLib::cScheduler>::GetInstance()->AddShceduleEvent( CSDef::E_TIMER_DEFINE::E_5_SEC , CSDef::E_TIMER_INTERVAL::E_5_SECOND , false , false );

		//슬롯갱신
		NetLib::cSingleton<NetLib::cScheduler>::GetInstance()->AddShceduleEvent(CSDef::E_TIMER_DEFINE::E_SLOTLOCK_SYNC , CSDef::E_TIMER_INTERVAL::E_5_SECOND, false, false);

		// 1분에 처리할 작업들 처리용 타이머
		NetLib::cSingleton<NetLib::cScheduler>::GetInstance()->AddShceduleEvent( CSDef::E_TIMER_DEFINE::E_UPDATE_HERO_SESSION_EXPIRE , CSDef::E_TIMER_INTERVAL::E_60_SECOND , false , false );
		NetLib::cSingleton<NetLib::cScheduler>::GetInstance()->AddShceduleEvent( CSDef::E_TIMER_DEFINE::E_CONCURRENT_USERS_LOG , CSDef::E_TIMER_INTERVAL::E_60_SECOND , false , false );
		//NetLib::cSingleton<NetLib::cScheduler>::GetInstance()->AddShceduleEvent( CSDef::E_TIMER_DEFINE::E_SLOT_RANKING_CACHING , CSDef::E_TIMER_INTERVAL::E_60_SECOND , false , false );

		// 서버 초기화시에 한번 처리
		NetLib::cSingleton<NetLib::cScheduler>::GetInstance()->AddShceduleEvent(CSDef::E_TIMER_DEFINE::E_SERVER_INIT, CSDef::E_TIMER_INTERVAL::E_100_MILLISECOND, true, false);

		// 클라이언트 들에게 1초마다 핑을 보내라고 요청한다. 
		NetLib::cSingleton<NetLib::cScheduler>::GetInstance()->AddShceduleEvent(CSDef::E_TIMER_DEFINE::E_PING_REQUEST, CSDef::E_TIMER_INTERVAL::E_1_SECOND, false, false);

		// 웹서버 살아 있는지 체크용(현재로써는...)
		NetLib::cSingleton<NetLib::cScheduler>::GetInstance()->AddShceduleEvent(CSDef::E_TIMER_DEFINE::E_SERVER_ALIVE_CHECK, CSDef::E_TIMER_INTERVAL::E_10_SECOND, false, true);

		//#ifdef _DEBUG
		//		NetLib::cSingleton<NetLib::cScheduler>::GetInstance()->AddShceduleEvent(SERVER_TEST_CODE_TIMER, true);
		//#endif

		// 데이터 파일 로딩
		NetLib::cSingleton<cDataLoader>::GetInstance();
		//NetLib::cSingleton<cDataLoader>::GetInstance()->LoadPinball();

		// 방을 생성한다.

		// 카드 덱 생성
		// 덱 생성 위치는 이동해도 상관 없을것 같다.
		NetLib::cSingleton<cCardDeck>::GetInstance();
		// Redis 커넥션
		NetLib::cSingleton<NetLib::cRedisManager>::GetInstance();
		NetLib::cSingleton<cThreadPooler::cThreadPool>::GetInstance();

		// Connector 등록
		NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig()->ServerType;
		char* myPublicIp = pServerManager->GetPublicIP();
		int myPort = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig()->wDefaultServerPort;

		NetLib::cIocpConnector* pIocpConnector = NetLib::cSingleton<NetLib::cIocpConnector>::ExistsInstance();
		NetLib::cContextPooler* pContextPooler = NetLib::cSingleton<NetLib::cContextPooler>::ExistsInstance();
		if ( pIocpConnector && pContextPooler )
		{
			auto config = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();

			for ( auto& server : config->lobbies ) {

				// 자신과 아이피, 포트까지 동일하면 자신으로 간주
				if ( strcmp( myPublicIp , server.ipAddress.c_str() ) == 0 && myPort == server.nPort )
					continue;

				NetLib::cIocpContext* pIocpContext = pContextPooler->Pop();
				pIocpContext->SetConnectorInfo( server.ipAddress , server.nPort );
				pIocpContext->SetPortID( 0 );
				pIocpConnector->InsertConnector( E_SERVER_TYPE::LOBBY_SERVER , pIocpContext );
			}
			
			
		}

		// Alert 연결
		MadeSlackNotification* alertInterface = NetLib::cSingleton<MadeSlackNotification>::GetInstance();
		NetLib::cSingleton<NetLib::cLogThread>::ExistsInstance()->SetAlert( alertInterface );

		NetLib::cThreadManager* threadManager = NetLib::cSingleton<NetLib::cThreadManager>::ExistsInstance();
		NetLib::cCommandThread* commandThread = threadManager->GetCommandThreadPtr();
		commandThread->SetAlert( alertInterface );

		if ( pServerManager->isSlackOn() )
		{
			std::string serverType = "";
			if ( E_SERVER_TYPE::LOBBY_SERVER == pServerManager->GetServerType() )
				serverType = "LOBBY_SERVER";
			else if ( E_SERVER_TYPE::SLOT_SERVER == pServerManager->GetServerType() )
				serverType = "SLOT_SERVER";
			
			std::string t_message = std::format( "{}-{}" , serverType , pServerManager->GetServerID() );
			alertInterface->Init( t_message );
		}

		// 콘솔창 출력 활성화 해둔다.
		NetLib::cLogManager::outputConsole = TRUE;

		ReleaseTraceA( "TableServer" + std::to_string( pServerManager->GetServerID() ) + " Started" );

#ifdef _DEBUG

		// 족보 테스트
		//for( int n = 0; n < 10; ++n  )
		//	cLowBaduki::JokboTest();

		//cLowBaduki::JokboTestFixedScenario();

		// DB 로딩 테스트
		/*
		General::ParticipantProfile _player;
		QueryManager* queryManager = new QueryManager();
		queryManager->GetPlayer( "qYU2sycNVrFk4cof" , "J4wrIkqxpRgbgWRA" , _player );
		uint64 playerIdx = _player.member_id();
		std::string nick_name = _player.display_name();
		*/
		
		// Slack 알림 테스트
		//tstring jsonData = _T("{\"text\": \"Hello, World!\"}");
		//NetLib::cSingleton<MadeSlackNotification>::GetInstance()->SendAlert( jsonData );

		// 바카라 테스트
		{
			/*General::PlayingCard card;

			std::vector<General::PlayingCard> playerCards;
			card.set_suit_code(General::CardSuit::CardSuit_Heart);
			card.set_rank_code(General::CardRank::CardRank_Four);
			playerCards.push_back(card);
			card.set_suit_code( General::CardSuit::CardSuit_Spade );
			card.set_rank_code( General::CardRank::CardRank_Two );
			playerCards.push_back( card );

			std::vector<General::PlayingCard> bankerCards;
			card.set_suit_code( General::CardSuit::CardSuit_Heart );
			card.set_rank_code( General::CardRank::CardRank_Ace );
			bankerCards.push_back( card );
			card.set_suit_code( General::CardSuit::CardSuit_Spade );
			card.set_rank_code( General::CardRank::CardRank_Two );
			bankerCards.push_back( card );

			cBaccarat::IsNaturalStatic( playerCards , bankerCards );*/
		}
		
		// 슬롯 스핀 테스트
		//{
		//	PmNet::RotationRS _response;
		//	_response.Clear();

		//	const uint64 bet_money = 1000;
		//	const int spinCount = 1000000;
		//	auto pConquerThePlanet = new cConquerThePlanetNew( Common::GameType::GameType_Slot_Cascading_Reel , static_cast< General::SlotCatalog >( General::SlotCatalog::Conquer_The_Planet ) , General::SlotStakePreset::SlotStake_1K );
		//	auto pBiteTheNobleNew = new cBiteTheNobleNew( Common::GameType::GameType_Slot_Culster_Pays_Reel , static_cast< General::SlotCatalog >( General::SlotCatalog::Byte_The_Noble ) , General::SlotStakePreset::SlotStake_1K );
		//	auto pCaptainPeg = new cCaptainPegNew( Common::GameType::GameType_Slot_Line_Pay_Reel , static_cast< General::SlotCatalog >( General::SlotCatalog::Captain_Peg ) , General::SlotStakePreset::SlotStake_1K );
		//	auto pIdolLive = new cIdolLive( Common::GameType::GameType_Slot_Cascading_Overlay_Reel , static_cast< General::SlotCatalog >( General::SlotCatalog::Idol_Live ) , General::SlotStakePreset::SlotStake_1K );
		//	for ( int n = 0; n < spinCount; ++n ) {
		//		PmNet::Rotation _spin;
		//		_spin.Clear();

		//		//pConquerThePlanet->Spin( _response , n );
		//		//pConquerThePlanet->PurchaseSpin( _response , n );
		//	
		//		//int multiplier_rate_sum = 0;
		//		//pConquerThePlanet->FreeSpin( _response , General::SpinMode::SpinMode_Free, General::SymbolFeature::SymbolFeature_FreeScatter, multiplier_rate_sum );
		//	
		//		//pBiteTheNobleNew->Spin( _response , n );
		//		//pBiteTheNobleNew->PurchaseSpin( _response , n );
		//	
		//		//pIdolLive->Spin( _response , n );
		//		//pIdolLive->PurchaseSpin( _response , n );

		//		// 바이트더 노블 프리스핀 구현
		//		uint64 spinUniqueKey = UniqueKey::SpinUniqueKey( 5 );
		//		
		//		General::SpinMode _spin_type = General::SpinMode::SpinMode_Count;
		//		
		//		// GrandSpin 이 활성화 되어 있는 경우에는 그랜드 스핀 로직을 따른다.
		//		//if ( pBiteTheNobleNew->GetGrandSpinCount() <= 0 ) {
		//		
		//		//	_spin_type = General::SpinMode::SpinMode_Base;
		//		//	_spin.set_rotation_kind( _spin_type );
		//		//	pBiteTheNobleNew->Spin( _spin , spinUniqueKey );
		//		//	pBiteTheNobleNew->GetScatterReward();
		//		//}
		//		//// 그랜드 스핀을 돌린다.
		//		//// 재화를 사용하지 않는다.
		//		//else
		//		{
		//			_spin_type = General::SpinMode::SpinMode_Buy;
		//			pBiteTheNobleNew->PurchaseSpin( _spin , spinUniqueKey );
		//			pBiteTheNobleNew->UseGrandSpin();
		//		}

		//		auto add_free_spin = _response.add_rotations();
		//		add_free_spin->CopyFrom( _spin );
		//		
		//		// Free 스핀 여부 결정
		//		const int& scatter_count = pBiteTheNobleNew->GetScatterCount();
		//		int first_free_spin_count = pBiteTheNobleNew->GetFreeSpinCount( scatter_count );
		//		int free_spin_count = first_free_spin_count;
		//		
		//		// Scatter 의 포지션 정보를 가져온다.
		//		if ( free_spin_count > 0 ) {
		//			auto scatter_info = _spin.mutable_scatter_meta();
		//			pBiteTheNobleNew->GetScatterPosList( scatter_info );
		//		}
		//		
		//		int multiplier_rate_sum = 0;
		//		for ( int n = 0; n < free_spin_count; ++n ) {
		//			_spin.Clear();
		//			_spin.set_rotation_kind( General::SpinMode::SpinMode_Free );
		//		
		//			// cur_free_spin_seq 및  total_free_spin_count 처리
		//			_spin.set_cur_bonus_rotation_seq( n + 1 );
		//			_spin.set_total_bonus_rotation_cnt( free_spin_count );
		//		
		//			// Grand 스핀이냐 Normal 스캐터 심볼 타입 결정
		//			General::SymbolFeature specialSymbolType = General::SymbolFeature::SymbolFeature_None;
		//			if ( _spin_type == General::SpinMode::SpinMode_Base )
		//				specialSymbolType = General::SymbolFeature::SymbolFeature_FreeScatter;
		//			else
		//				specialSymbolType = General::SymbolFeature::SymbolFeature_GrandScatter;
		//		
		//			pBiteTheNobleNew->FreeSpin( _spin , General::SpinMode::SpinMode_Free , specialSymbolType , multiplier_rate_sum );
		//		
		//			// Free 스핀에서 나온 스캐터 숫자에 따라 free_spin_count 를 증가 시킴
		//			const int& free_scatter_count = pBiteTheNobleNew->GetScatterCount();
		//			const int add_free_spin_count = pBiteTheNobleNew->GetAddFreeSpinCount( free_scatter_count );
		//			free_spin_count += add_free_spin_count;
		//			if ( add_free_spin_count ) {
		//		
		//				auto scatter_info = _spin.mutable_scatter_meta();
		//				pBiteTheNobleNew->GetScatterPosList( scatter_info );
		//		
		//				// 프리 스핀 카운트를 내려 보내준다.
		//				_spin.set_total_bonus_rotation_cnt( free_spin_count );
		//			}

		//			add_free_spin = _response.add_rotations();
		//			add_free_spin->CopyFrom( _spin );
		//		}

		//	}
		//	printf("Slot Spin Test Complete. SpinCount = [%d]" , spinCount );
		//}

		// 쿼리 테스트
		/*PmNet::MidLookupByAliasRS response;
		QueryManager* queryManager = new QueryManager();
		queryManager->PlayerIdxSearchByNickname( "c8Jnm" , response );*/

		//int slotGameTypeNumber = 0;
		//std::cin >> slotGameTypeNumber;

		/*PlatformAuthReq _request;
		_request.set_outlet_kind(General::AccessChannelType::AccessChannel_Android);

		std::string url = "";
		char szWebUrl[ CSDef::EDef::MAX_BUFFER_256_LEN ] = { 0, };
		StringCbPrintfA( szWebUrl ,
			sizeof( szWebUrl ) ,
			"http://%s:%d/%s/%s" ,
			"127.0.0.1" ,
			35582 ,
			"Game" ,
			"ServerRequest" );

		std::string data;

		Server::ServiceStatusCode errorcode = cProtoMsgStub::WebPostRequest( szWebUrl ,
				_request ,
				static_cast< UINT >( General::PacketID::Packet_PlatformVerify ) ,
				0 ,
				0 ,
				data ,
				10 );*/

		// 첫번째는 검색이 되고 두번째는 검색이 안된다. 왜??
		//const std::string test1 = "SELECT nickname FROM players WHERE nickname = '​한글';";
		//const std::string test2 = "SELECT nickname FROM players WHERE nickname = '한글';";

		/*std::string change_nick_wrong = "한글";
		int stringSize = change_nick_wrong.size();
		change_nick_wrong = StringUtil::RemoveInvisibleSpaces( change_nick_wrong );
		stringSize = change_nick_wrong.size();

		std::string change_nick = "한글";


		std::vector<std::string> _find_nicks;
		std::future<BOOL> result = QueryManager::SelectNickNameAsync( change_nick , _find_nicks );
		result.wait();

		BOOL query_result = result.get();
		query_result = result.get();*/

		/*std::string key = MADEPlatform::GenerateRandomBytes();
		std::string iv = MADEPlatform::GenerateRandomBytes();
	
		std::string password = "1q2w3e4r!";
		MADEPlatform::isValidPassword( password );*/
		/*std::string _cid = "sD+Dd4x7U4rzUtuRdg02oKfRjesSRv61FgUBifJHtxFsBoJqU+Aq/yOx2v33tIQWn2xiqGF4URDt7PQe24E6pA==";
		std::vector<std::string> _MADE_ids;
		QueryManager::SelectByCid( _cid , _MADE_ids );*/

		/*string password = "test1234!@#";
		string hashString = MADEPlatform::GenerateSha256( password );*/

#endif
		return TRUE;
	}

	return TRUE;
}

BOOL TableServerMain::InitServerResources()
{
	// ProtoUtil Buffer 관련 커맨드 쓰레드 갯수 만큼 생성한다.
	int commandsCount = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig()->nCommandThreadCnt;
	int websCount = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig()->nWebThreadCnt;
	protoutil::cProtoUtil::MemoryAllocProtobufSerializeBuffers(commandsCount, websCount);

	NetLib::cSingleton<cMySQLConnectionPooler>::GetInstance();

	// mysql 인스턴스를 만듭니다.
	NetLib::cSingleton<cMySQL>::GetInstance();

	return TRUE;
}

void TableServerMain::DestroyServerResources()
{
}
