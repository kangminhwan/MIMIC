#include "cDataLoader.h"
#include "cProtoUtil.h"
#include "StringUtil.h"
#include "TrieNode.h"
#include "..\include\Netlib\Manager\ServerManager.h"
#include "..\Include\Netlib\Common\cSingleton.h"
#include "..\Include\Netlib\Scheduler\cScheduler.h"
#include "..\Include\Netlib\FileLoader\csvFileLoader.h"
#include "../Include/Netlib/RestSdkHttp/restsdkHttp.h"

#include <nlohmann/json.hpp>

#include "Query.h"

#include <format>
#include <iostream>
#include <locale>
#include <codecvt>
#include <string>
#include <sstream>

#include <google/protobuf/util/json_util.h>
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>

#include "cConfigReader.h"

bool cDataLoader::LoadingEnd = false;
bool cDataLoader::SystemLoadingEnd = false;

cDataLoader::cDataLoader()
{
	TraceA( "cDataLoader loading start" );

	memset( m_DataDirectory , 0x00, sizeof( m_DataDirectory ) );
	
	TCHAR* currentDic = NetLib::csvFileLoader::GetExecutableDirectory();
	_sntprintf_s( m_DataDirectory , _countof( m_DataDirectory ) , _T( "%s\\data" ) , currentDic );

	LoadGameConfig( _T( "GameConfig.csv" ) );
	LoadBasicSetting( _T( "BasicSetting.csv" ) );

	// ButtonList 먼저 로딩하고 ChannelList 로딩한다.
	LoadButtonList( _T( "ButtonList.csv" ) );
	//LoadChannelList( _T( "ChannelList.csv" ) );
	LoadChannelData( _T( "ChannelData.csv" ) );
	LoadSystemData( _T( "SystemData.csv" ) );

	LoadRestrictedWord( _T( "RestrictedWord.csv" ) );

	LoadLevelData( _T( "Level_data.csv" ) );
	LoadAvatar( _T( "Avatar_Data.csv" ) );
	LoadMission( _T( "QuestData.csv" ) );
	LotteryReward( _T( "Lottery_Reward.csv" ) );

	LoadItemProduct( _T( "ItemProduct.csv" ) );
	LoadStoreProduct( _T( "StoreProduct.csv" ) );

	LoadAttendanceReward( _T( "Attendance_reward.csv" ) );

	//LoadPinballMissRate( _T( "pinball_data\\15_pb_rtp_control.csv" ) );
	LoadSystemDataDB();


	LoadingEnd = true;
	TraceA( "cDataLoader loading complete" );
	m_mail_state = General::InboxState::InboxState_Claimable;
	dailyExpiredMonth = 5;
}

void cDataLoader::LoadSystemDataDB()
{

}

void cDataLoader::LoadGameConfig( const TCHAR* fileName )
{
	TCHAR csvFileFullPath[ MAX_PATH ];
	_sntprintf_s( csvFileFullPath , MAX_PATH , _T( "%s\\%s" ) , m_DataDirectory, fileName );

	auto fileLoader = NetLib::cSingleton<NetLib::csvFileLoader>::GetInstance();

	fileLoader->Create( csvFileFullPath );
	fileLoader->Open();
	int recordCount = fileLoader->GetRecordCount();

	auto record_1 = fileLoader->GetRecord( 0 );
	auto record_2 = fileLoader->GetRecord( 1 );
	int columnCount = record_1->m_vecField.size();

	// 1, 2 번 레코드에서 값을 읽어서 맵에 저장한다.
	// 마지막 라인엔 End 라서 생략한다.
	for ( int n = 1; n < columnCount - 1; ++n ) {

		std::wstring headerString = record_1->m_vecField[ n ];
		int value = _wtoi( record_2->m_vecField[ n ].c_str() );

		m_configDataMap.insert( std::pair<std::wstring , int>( headerString , value ) );
	}

	for ( auto dataRow : m_configDataMap ) {
		TraceW( std::format( L"Data Loader : ID [ {} ], VALUE [ {} ]" , dataRow.first , dataRow.second ) );
	}


	LowBadukiPlayerCount = m_configDataMap[ L"LowBadukiPlayerCount" ];
	LowBadukiWait = m_configDataMap[ L"LowBadukiWait" ];
	LowBadukiTurnMinimumWait = m_configDataMap[ L"LowBadukiTurnMinimumWait" ];
	RecentlyHistoryCount = m_configDataMap[ L"RecentlyHistoryCount" ];
	LowBadukiBetWait = m_configDataMap[ L"LowBadukiBetWait" ];
	CardChangeWait = m_configDataMap[ L"CardChangeWait" ];
	BaseDistributionCardWaitTwo = m_configDataMap[ L"BaseDistributionCardWaitTwo" ];
	BaseDistributionCardWaitThree = m_configDataMap[ L"BaseDistributionCardWaitThree" ];
	BaseDistributionCardWaitFour = m_configDataMap[ L"BaseDistributionCardWaitFour" ];
	BaseDistributionCardWaitFive = m_configDataMap[ L"BaseDistributionCardWaitFive" ];
	PlayerTurnResWait = m_configDataMap[ L"PlayerTurnResWait" ];
	PlayResultWaitGiveUp = m_configDataMap[ L"PlayResultWaitGiveUp" ];
	PlayResultWaitTwo = m_configDataMap[ L"PlayResultWaitTwo" ];
	PlayResultWaitThree = m_configDataMap[ L"PlayResultWaitThree" ];
	PlayResultWaitFour = m_configDataMap[ L"PlayResultWaitFour" ];
	PlayResultWaitFive = m_configDataMap[ L"PlayResultWaitFive" ];
	LowBadukiCardChangeDelay = m_configDataMap[ L"ChardChangeDelay" ];

	// 첫번째 라인은 헤더 0 번
	// 1분 부터 데이터
	// 필드 0은 지나가도록 한다.
	// 현재 데이터 구조상 2라인 뿐이 처리 못하기 때문에 아래 코드는 차후 변경되어야 한다.
	//for ( int n = 0 ; n < recordCount; ++n )
	//{
	//	auto record = fileLoader->GetRecord(n);

	//	// 필드 카운트
	//	record->m_vecField.size();

	//	// 1번 필드는 처리 안함
	//	for ( int field = 1 ; field < record->m_vecField.size() ; ++field )
	//	{
	//		// 헤더 라인은 스트링으로 처리
	//		if ( n == 0 ) {
	//			std::wstring headerString = record->m_vecField[ field ];
	//			m_DataMap.insert( std::pair<std::wstring , int>( headerString , 0 ) );
	//		}
	//		else {
	//			int value = _wtoi( record->m_vecField[ field ].c_str() );
	//			value = _wtoi( record->m_vecField[ field ].c_str() );
	//		}
	//			


	//	}
	//}

	NetLib::cSingleton<NetLib::csvFileLoader>::DeleteInstance();
}

void cDataLoader::LoadBasicSetting( const TCHAR* fileName )
{
	TCHAR csvFileFullPath[ MAX_PATH ];
	_sntprintf_s( csvFileFullPath , MAX_PATH , _T( "%s\\%s" ) , m_DataDirectory , fileName );

	auto fileLoader = NetLib::cSingleton<NetLib::csvFileLoader>::GetInstance();

	fileLoader->Create( csvFileFullPath );
	fileLoader->Open();

	int recordCount = fileLoader->GetRecordCount();

	TraceA( "LoadBasicSetting load start" );

	m_basicSettings.clear();
	m_ListArraySettings.clear();

	for ( int row = 1; row < recordCount; ++row )
	{
		auto rowData = fileLoader->GetRecord( row );

		if ( rowData == nullptr )
			continue;

		if ( rowData->m_vecField.size() < 2 )
			continue;

		if ( rowData->m_vecField[ 0 ].size() < 2 )
			continue;

		std::string defaultTypeString(
			rowData->m_vecField[ 0 ].begin() ,
			rowData->m_vecField[ 0 ].end()
		);

		auto enumValue = Server::BaselineConfigKey_descriptor()->FindValueByName( defaultTypeString );

		if ( enumValue == nullptr )
		{
			std::string errorInfo = std::format( "Invalid BaselineConfigKey: {}" , defaultTypeString );
			TraceA( errorInfo );
			continue;
		}

		Server::BaselineConfigKey defaultType =
			static_cast< Server::BaselineConfigKey >( enumValue->number() );

		const std::wstring& valueText = rowData->m_vecField[ 1 ];

		switch ( defaultType )
		{
		case Server::BaselineConfig_LostLimitValue:
		case Server::BaselineConfig_LostLimitTime:
		{
			std::vector<uint64> values = StringUtil::ParseUInt64Array( valueText );

			if ( values.empty() )
			{
				std::string errorInfo = std::format( "Invalid array value. key: {}" , defaultTypeString );
				TraceA( errorInfo );
				continue;
			}

			m_ListArraySettings.insert(
				std::pair<Server::BaselineConfigKey , std::vector<uint64>>( defaultType , values )
			);

			break;
		}

		default:
		{
			wchar_t* endPtr = nullptr;
			uint64 value = std::wcstoull( valueText.c_str() , &endPtr , 10 );

			if ( endPtr == nullptr || *endPtr != L'\0' )
			{
				std::string errorInfo = std::format( "Invalid uint64 value. key: {}" , defaultTypeString );
				TraceA( errorInfo );
				continue;
			}

			m_basicSettings.insert(
				std::pair<Server::BaselineConfigKey , uint64>( defaultType , value )
			);

			break;
		}
		}
	}

	TraceA( "LoadBasicSetting load complete" );

	NetLib::cSingleton<NetLib::csvFileLoader>::DeleteInstance();
}

void cDataLoader::LoadChannelData( const TCHAR* fileName )
{
	TCHAR csvFileFullPath[ MAX_PATH ];
	_sntprintf_s( csvFileFullPath , MAX_PATH , _T( "%s\\%s" ) , m_DataDirectory , fileName );

	auto fileLoader = NetLib::cSingleton<NetLib::csvFileLoader>::GetInstance();

	fileLoader->Create( csvFileFullPath );
	fileLoader->Open();
	int recordCount = fileLoader->GetRecordCount();

	auto record_1 = fileLoader->GetRecord( 0 ); // 컬럼데이터 구조
	int columnCount = record_1->m_vecField.size(); // end 는 빼고 로딩해야함

	TraceA( "LoadChannelList load start" );

	const auto configReader = NetLib::cSingleton<cConfigReader>::GetInstance()->getConfig();

	// 로비 서버 아이디 별로 분산한다.
	m_channelDistribution.insert( std::pair<General::PlayCategory , int>( General::PlayCategory::PlayCategory_TexasHoldem , 3) );
	//m_channelDistribution.insert( std::pair<General::PlayCategory , int>( Common::GameType::GameType_Roulette , 1 ) );
	//m_channelDistribution.insert( std::pair<General::PlayCategory , int>( Common::GameType::GameType_Pinball , 1 ) );

	//ID , GameType , MoneyType , ButtonType , SeedMoney , MoneyMin , MoneyKick , MoneyMax , LimitBetMoney , BetList , Player_Banker_Max , Tie_Pair_Max , Default_RoomList , End

	for ( int row = 1; row < recordCount; ++row ) {
		auto rowData = fileLoader->GetRecord( row );
		int colCount = rowData->m_vecField.size();

		Server::Channel _channel;
		std::wstring _id_load = rowData->m_vecField[ 0 ].c_str();	//string id = 1;
		std::string _id( _id_load.begin() , _id_load.end() );
		_channel.set_id( _id );

		std::string gameTypeString( rowData->m_vecField[ 1 ].begin() , rowData->m_vecField[ 1 ].end() );
		std::string t_gameTypeString = gameTypeString;
		General::PlayCategory gameType = static_cast< General::PlayCategory >( General::PlayCategory_descriptor()->FindValueByName( t_gameTypeString )->number() );
		if ( gameType != General::PlayCategory::PlayCategory_TexasHoldem )
			continue;
		_channel.set_game_type( gameType );

		// 채널 분리 작업, 게임 타입으로 분리한다.
		auto mapPair = m_channelDistribution.find( gameType );
		if ( mapPair == m_channelDistribution.end() )
			throw std::invalid_argument( "Table Game Type Distribution Error" );

		_channel.set_server_id( mapPair->second );

		std::string channelContentTypeString( rowData->m_vecField[ 2 ].begin(), rowData->m_vecField[ 2 ].end() );
		Server::ChannelPlayMode _channel_content_type = Server::ChannelPlayMode::ChannelPlayMode_None;
		if ( channelContentTypeString.size() > 1 )
		{
			if ( channelContentTypeString == "Normal" )
				channelContentTypeString = "ChannelPlayMode_Standard";
			else if ( channelContentTypeString == "Lounge" || channelContentTypeString == "Lounge" )
				channelContentTypeString = "ChannelPlayMode_Lounge";
			_channel_content_type = static_cast< Server::ChannelPlayMode >( Server::ChannelPlayMode_descriptor()->FindValueByName( channelContentTypeString )->number() );
		}

		_channel.set_channel_content_type( _channel_content_type );

		std::string moneyTypeString = "AssetKind_" + string( rowData->m_vecField[ 3 ].begin() , rowData->m_vecField[ 3 ].end() );
		General::AssetKind moneyType =static_cast<General::AssetKind >( General::AssetKind_descriptor()->FindValueByName(moneyTypeString)->number() );
		_channel.set_money_type( static_cast<General::AssetKind>( moneyType ) );
		std::string buttonListString( rowData->m_vecField[ 4 ].begin() , rowData->m_vecField[ 4 ].end() );
		std::vector<int> buttons;

		// Split by '/'
		std::stringstream ss( buttonListString );
		std::string token;

		while ( std::getline( ss , token , '/' ) ) {
			if ( !token.empty() ) {
				try {
					int value = std::stoi( token );
					buttons.push_back( value );
				}
				catch ( const std::exception& e ) {
					// 잘못된 숫자 무시 or 로그 출력
				}
			}
		}

		// 결과 적용
		for ( int t_button : buttons ) {
			_channel.mutable_button_list()->Add( t_button );
		}

		uint32 _dealer_fee = _wtoi( rowData->m_vecField[ 9 ].c_str() );	// 딜러비
		_channel.set_dealer_fee( _dealer_fee );

		uint64 _temp64 = _wtoi64( rowData->m_vecField[ 5 ].c_str() );	// uint64 seed_money = 4;				// 플레이 하기 위해 필요한 시드 머니
		_channel.set_seed_money( _temp64 );
		
		_temp64 = _wtoi64( rowData->m_vecField[ 6 ].c_str() );			// uint64 money_min = 6;				// 채널 입장시 필요한 최소 금액
		_channel.set_money_min( _temp64 );

		_temp64 = _wtoi64( rowData->m_vecField[ 8 ].c_str() );			// uint64 money_kick					// 채널 Kick 조건
		_channel.set_money_kick( _temp64 );

		_temp64 = _wtoi64( rowData->m_vecField[ 7 ].c_str() );			// uint64 money_max = 7;				// 채널 입장시 필요한 최대 금액, 0 으로 셋팅되면 제한 없음
		_temp64 = _temp64 == 0 ? UINT64_MAX : _temp64;
		_channel.set_money_max( _temp64 );

		_temp64 = _wtoi64( rowData->m_vecField[ 10 ].c_str() );			// uint64 limit_bet_money = 8;			// 최대 베팅 금액, 0 으로 셋팅되면 제한 없음
		_temp64 = _temp64 == 0 ? UINT64_MAX : _temp64;
		_channel.set_limit_bet_money( _temp64 );

		// BetList 필드 파싱
		//uint64 bet_1 = 9;					// 베팅 1 Money Value
		//uint64 bet_2 = 10;					// 베팅 2 Money Value
		//uint64 bet_3 = 11;					// 베팅 3 Money Value
		//uint64 bet_4 = 12;					// 베팅 4 Money Value
		//string bet_max = 13;				// string 으로 MAX 가 들어오게 됨 (100000/500000/1000000/5000000/MAX)
		{
			if ( wcslen( rowData->m_vecField[ 12 ].c_str() ) > 2 ) {
				
				std::vector<std::wstring> bets;
				wchar_t fieldData[ 100 ] = { 0, };
				wcscpy_s( fieldData , rowData->m_vecField[ 12 ].c_str() );

				wchar_t* buffer;
				wchar_t* token = wcstok_s( fieldData , L"/" , &buffer );
				bets.push_back(token);
				while ( token ) {
					//printf( "%ls\n" , token );
					token = wcstok_s( NULL , L"/" , &buffer );
					if ( token != nullptr )
						bets.push_back( token );
				}

				// Parsing 한 목록을 

				_temp64 = _wtoi64( bets[0].c_str() );
				_temp64 = _temp64 == 0 ? UINT64_MAX : _temp64;
				_channel.set_bet_1( _temp64 );

				_temp64 = _wtoi64( bets[ 1 ].c_str() );
				_temp64 = _temp64 == 0 ? UINT64_MAX : _temp64;
				_channel.set_bet_2( _temp64 );

				_temp64 = _wtoi64( bets[ 2 ].c_str() );
				_temp64 = _temp64 == 0 ? UINT64_MAX : _temp64;
				_channel.set_bet_3( _temp64 );

				_temp64 = _wtoi64( bets[ 3 ].c_str() );
				_temp64 = _temp64 == 0 ? UINT64_MAX : _temp64;
				_channel.set_bet_4( _temp64 );

				//_channel.set_bet_max( bets[ 4 ].c_str() );
			}
		}

		_temp64 = _wtoi64( rowData->m_vecField[ 13 ].c_str() );			// uint64 player_and_banker_max = 11;	// 플레이어 & 뱅커 Max
		_channel.set_player_and_banker_max( _temp64 );
		
		_temp64 = _wtoi64( rowData->m_vecField[ 14 ].c_str() );			// uint64 tie_and_pair_max = 13;		// 타이 & 페어 Max
		_channel.set_tie_and_pair_max( _temp64 );

		_temp64 = _wtoi64( rowData->m_vecField[ 11 ].c_str() );			// uint64 default_room_list = 14;		// 디폴트로 생성해야할 방 갯수?? 확인 필요
		_channel.set_default_room_list( _temp64 );

		_temp64 = _wtoi64( rowData->m_vecField[ 15 ].c_str() );			//맥스pp값 = 14;		// 디폴트로 생성해야할 방 갯수?? 확인 필요
		_channel.set_pp( _temp64 );

		
		
		m_channelList.insert(std::pair<std::string , Server::Channel>( _id , _channel ));

		std::string serializedData;
		google::protobuf::util::MessageToJsonString( _channel , &serializedData );
		TraceA( serializedData );
	}

	std::string errorInfo = std::format( "LoadChannelList load complete row count {}" , m_channelList.size() );
	TraceA( errorInfo );

	NetLib::cSingleton<NetLib::csvFileLoader>::DeleteInstance();
}

void cDataLoader::LoadRestrictedWord( const TCHAR* fileName )
{
	Trie* trie = NetLib::cSingleton<Trie>::GetInstance();

	TCHAR csvFileFullPath[ MAX_PATH ];
	_sntprintf_s( csvFileFullPath , MAX_PATH , _T( "%s\\%s" ) , m_DataDirectory , fileName );
	auto fileLoader = NetLib::cSingleton<NetLib::csvFileLoader>::GetInstance();

	fileLoader->Create( csvFileFullPath );
	fileLoader->Open();
	int recordCount = fileLoader->GetRecordCount();

	auto record_1 = fileLoader->GetRecord( 0 ); // 컬럼데이터 구조
	int columnCount = record_1->m_vecField.size(); // end 는 빼고 로딩해야함

	TraceA( "LoadRestrictedWord load start" );

	for ( int row = 1; row < recordCount; ++row ) {
		auto rowData = fileLoader->GetRecord( row );
		int colCount = rowData->m_vecField.size();

		//Word
		std::wstring RestrictedWord( rowData->m_vecField[ 0 ].begin() , rowData->m_vecField[ 0 ].end() );
		trie->insert( RestrictedWord );

	
	}
	
	NetLib::cSingleton<NetLib::csvFileLoader>::DeleteInstance();
}

void cDataLoader::LoadButtonList( const TCHAR* fileName )
{
	TCHAR csvFileFullPath[ MAX_PATH ];
	_sntprintf_s( csvFileFullPath , MAX_PATH , _T( "%s\\%s" ) , m_DataDirectory , fileName );

	auto fileLoader = NetLib::cSingleton<NetLib::csvFileLoader>::GetInstance();

	fileLoader->Create( csvFileFullPath );
	fileLoader->Open();
	int recordCount = fileLoader->GetRecordCount();

	auto record_1 = fileLoader->GetRecord( 0 ); // 컬럼데이터 구조
	int columnCount = record_1->m_vecField.size(); // end 는 빼고 로딩해야함

	TraceA( "LoadButtonList load start" );

	for ( int row = 1; row < recordCount; ++row ) {
		auto rowData = fileLoader->GetRecord( row );
		int colCount = rowData->m_vecField.size();

		// ID
		std::string buttonTypeString( rowData->m_vecField[ 0 ].begin() , rowData->m_vecField[ 0 ].end() );

		if ( buttonTypeString.size() == 0 || buttonTypeString.compare( " " ) == 0 )
			continue;

		General::LobbyButtonPreset buttonType = General::LobbyButtonPreset::LobbyButton_None;
		if ( buttonTypeString.compare( "0" ) != 0 ) {
			if ( buttonTypeString == "Button_Type_None" ) buttonTypeString = "LobbyButton_None";
			else if ( buttonTypeString == "Button_Type_1" ) buttonTypeString = "LobbyButton_LowBadugiHalf";
			else if ( buttonTypeString == "Button_Type_2" ) buttonTypeString = "LobbyButton_LowBadugiFull";
			else if ( buttonTypeString == "Button_Type_3" ) buttonTypeString = "LobbyButton_LowBadugiFriend";
			else if ( buttonTypeString == "Button_Type_4" ) buttonTypeString = "LobbyButton_Holdem";
			buttonType = static_cast< General::LobbyButtonPreset >( General::LobbyButtonPreset_descriptor()->FindValueByName( buttonTypeString )->number() );
		}

		// ButtonList
		{
			std::vector<General::TableAction> button_list;
			button_list.clear();

			if ( wcslen( rowData->m_vecField[ 1 ].c_str() ) > 2 ) {

				std::vector<std::wstring> buttons;
				wchar_t fieldData[ 100 ] = { 0, };
				wcscpy_s( fieldData , rowData->m_vecField[ 1 ].c_str() );

				wchar_t* buffer;
				wchar_t* token = wcstok_s( fieldData , L"/" , &buffer );
				buttons.push_back( token );
				while ( token ) {
					token = wcstok_s( NULL , L"/" , &buffer );
					if ( token != nullptr )
						buttons.push_back( token );
				}

				for ( auto button : buttons )
				{
					int _temp = _wtoi64( button.c_str() );
					button_list.push_back( static_cast<General::TableAction>( _temp ) );
				}

				m_buttonList.insert( std::pair<General::LobbyButtonPreset , std::vector<General::TableAction>>( buttonType , button_list ));
			}
		}

		//m_channelList.insert( std::pair<std::string , Server::Channel>( _id , _channel ) );

		/*std::string serializedData;
		google::protobuf::util::MessageToJsonString( _channel , &serializedData );
		TraceA( serializedData );*/
	}

	std::string errorInfo = std::format( "LoadButtonList load complete row count {}" , m_channelList.size() );
	TraceA( errorInfo );

	NetLib::cSingleton<NetLib::csvFileLoader>::DeleteInstance();
}





//void cDataLoader::LoadCasReelFreeGrandData( const TCHAR* fileName )
//{
//	TCHAR csvFileFullPath[ MAX_PATH ];
//	_sntprintf_s( csvFileFullPath , MAX_PATH , _T( "%s\\%s" ) , m_DataDirectory , fileName );
//
//	auto fileLoader = NetLib::cSingleton<NetLib::csvFileLoader>::GetInstance();
//
//	fileLoader->Create( csvFileFullPath );
//	fileLoader->Open();
//	int recordCount = fileLoader->GetRecordCount();
//
//	auto record_1 = fileLoader->GetRecord( 0 ); // 컬럼데이터 구조
//	int columnCount = record_1->m_vecField.size(); // end 는 빼고 로딩해야함
//
//	TraceA( "LoadCasReelFreeGrandData load start" );
//
//	uint64 _temp64 = 0;
//
//	Server::CasReelData _casReelData;
//
//	for ( int row = 1; row < recordCount; ++row ) {
//		auto rowData = fileLoader->GetRecord( row );
//		int colCount = rowData->m_vecField.size();
//
//		_casReelData.Clear();
//
//		// SymbolCode
//		std::string nameString( rowData->m_vecField[ 0 ].begin() , rowData->m_vecField[ 0 ].end() );
//		General::SymbolCode symbolType = static_cast< General::SymbolCode >( General::SymbolCode_descriptor()->FindValueByName( nameString )->number() );
//		_casReelData.set_symbol_type( symbolType );
//
//		// SlotGameType
//		std::string slotGameTypeString( rowData->m_vecField[ 1 ].begin() , rowData->m_vecField[ 1 ].end() );
//		General::SlotCatalog slotGameType = static_cast< General::SlotCatalog >( General::SlotCatalog_descriptor()->FindValueByName( slotGameTypeString )->number() );
//		_casReelData.set_reel_kind( slotGameType );
//
//		// ID
//		int _id = _wtoi( rowData->m_vecField[ 2 ].c_str() );
//		_casReelData.set_id( _id );
//
//		wchar_t* endPtr;  // To store the end pointer
//
//		// Areal
//		int _areal = _wtoi( rowData->m_vecField[ 3 ].c_str() );
//		_casReelData.set_areel( _areal );
//
//		// Breal
//		int _breal = _wtoi( rowData->m_vecField[ 4 ].c_str() );
//		_casReelData.set_breel( _breal );
//
//		// Creal
//		int _creal = _wtoi( rowData->m_vecField[ 5 ].c_str() );
//		_casReelData.set_creel( _creal );
//
//		// Dreal
//		int _dreal = _wtoi( rowData->m_vecField[ 6 ].c_str() );
//		_casReelData.set_dreel( _dreal );
//
//		// Ereal
//		int _ereal = _wtoi( rowData->m_vecField[ 7 ].c_str() );
//		_casReelData.set_ereel( _ereal );
//
//		// Freel
//		int _freal = _wtoi( rowData->m_vecField[ 8 ].c_str() );
//		_casReelData.set_freel( _freal );
//
//		// 슬롯 게임 타입 체크
//		auto iter = m_casReelFreeDataLists.find( slotGameType );
//		if ( iter == m_casReelFreeDataLists.end() ) {
//			std::vector<Server::CasReelData> datas;
//			datas.push_back( _casReelData );
//			m_casReelFreeDataLists.insert( std::pair<General::SlotCatalog , std::vector<Server::CasReelData>>( slotGameType , datas ) );
//		}
//		else {
//			iter->second.push_back( _casReelData );
//		}
//
//		std::string serializedData;
//		google::protobuf::util::MessageToJsonString( _casReelData , &serializedData );
//		TraceA( serializedData );
//	}
//
//	std::string errorInfo = std::format( "LoadCasReelFreeGrandData load complete slot game count count {}" , m_casReelFreeDataLists.size() );
//	TraceA( errorInfo );
//
//	NetLib::cSingleton<NetLib::csvFileLoader>::DeleteInstance();
//}



void cDataLoader::LoadLevelData( const TCHAR* fileName )
{
	TCHAR csvFileFullPath[ MAX_PATH ];
	_sntprintf_s( csvFileFullPath , MAX_PATH , _T( "%s\\%s" ) , m_DataDirectory , fileName );

	auto fileLoader = NetLib::cSingleton<NetLib::csvFileLoader>::GetInstance();

	fileLoader->Create( csvFileFullPath );
	fileLoader->Open();
	int recordCount = fileLoader->GetRecordCount();

	auto record_1 = fileLoader->GetRecord( 0 ); // 컬럼데이터 구조
	int columnCount = record_1->m_vecField.size(); // end 는 빼고 로딩해야함

	TraceA( "LoadLevelData load start" );

	uint64 _temp64 = 0;
	Server::Level _level;

	for ( int row = 1; row < recordCount; ++row ) {
		auto rowData = fileLoader->GetRecord( row );
		int colCount = rowData->m_vecField.size();

		// ID
		std::string idString( rowData->m_vecField[ 0 ].begin() , rowData->m_vecField[ 0 ].end() );
		if ( idString.size() == 0 )
			continue;

		_level.set_id( idString );

		// Level
		int level = _wtoi( rowData->m_vecField[ 1 ].c_str() );
		_level.set_level( level );

		// Exp
		uint64 exp = _wtoi( rowData->m_vecField[ 2 ].c_str() );
		_level.set_exp( exp );

		// Reward_Chip
		uint64 rewardChip = _wtoi( rowData->m_vecField[ 3 ].c_str() );
		_level.set_reward_chip( rewardChip );

		/*std::string serializedData;
		google::protobuf::util::MessageToJsonString( _casSymbolId , &serializedData );
		TraceA( serializedData );*/

		m_Levels.push_back( _level );
	}

	NetLib::cSingleton<NetLib::csvFileLoader>::DeleteInstance();
}

void cDataLoader::LoadAvatar( const TCHAR* fileName )
{
	TCHAR csvFileFullPath[ MAX_PATH ];
	_sntprintf_s( csvFileFullPath , MAX_PATH , _T( "%s\\%s" ) , m_DataDirectory , fileName );

	auto fileLoader = NetLib::cSingleton<NetLib::csvFileLoader>::GetInstance();

	fileLoader->Create( csvFileFullPath );
	fileLoader->Open();
	int recordCount = fileLoader->GetRecordCount();

	auto record_1 = fileLoader->GetRecord( 0 ); // 컬럼데이터 구조
	int columnCount = record_1->m_vecField.size(); // end 는 빼고 로딩해야함

	TraceA( "LoadAvatar load start" );

	uint64 _temp64 = 0;
	General::AvatarProfile _avatar;

	for ( int row = 1; row < recordCount; ++row ) {
		auto rowData = fileLoader->GetRecord( row );
		int colCount = rowData->m_vecField.size();

		// ID
		int avatar_id = _wtoi( rowData->m_vecField[ 0 ].c_str() );
		_avatar.set_avatar_ref_id( avatar_id );

		// Avatar Name
		std::wstring avatar_name( rowData->m_vecField[ 1 ].begin() , rowData->m_vecField[ 1 ].end() );
		
		/*std::string avatar_name_ansi;
		convert_unicode_to_ansi_string( avatar_name_ansi , avatar_name.c_str() , avatar_name.size() );
		_avatar.set_avatar_label( avatar_name_ansi );*/

		std::string avatar_name_utf;
		convert_unicode_to_utf8_string( avatar_name_utf , avatar_name.c_str() , avatar_name.size() );
		_avatar.set_avatar_label( avatar_name_utf );

		// Free
		int is_free = _wtoi( rowData->m_vecField[ 2 ].c_str() );
		_avatar.set_free_available( is_free );

		// Default ( 디폴트 생성 여부 )
		int is_default = _wtoi( rowData->m_vecField[ 3 ].c_str() );
		_avatar.set_default_granted( is_default );

		m_avatars.push_back( _avatar );
		m_avatar_map.insert( std::pair<int , General::AvatarProfile>( avatar_id , _avatar ) );
		if ( is_free )
			m_free_avatars.push_back( _avatar );

		std::string serializedData;
		google::protobuf::util::MessageToJsonString( _avatar , &serializedData );
		TraceA( serializedData );
	}

	std::string errorInfo = std::format( "LoadAvatar load complete row count {}" , m_avatars.size() );
	TraceA( errorInfo );

	NetLib::cSingleton<NetLib::csvFileLoader>::DeleteInstance();
}

void cDataLoader::LoadMission( const TCHAR* fileName )
{
	TCHAR csvFileFullPath[ MAX_PATH ];
	_sntprintf_s( csvFileFullPath , MAX_PATH , _T( "%s\\%s" ) , m_DataDirectory , fileName );

	auto fileLoader = NetLib::cSingleton<NetLib::csvFileLoader>::GetInstance();

	fileLoader->Create( csvFileFullPath );
	fileLoader->Open();
	int recordCount = fileLoader->GetRecordCount();

	auto record_1 = fileLoader->GetRecord( 0 ); // 컬럼데이터 구조
	int columnCount = record_1->m_vecField.size(); // end 는 빼고 로딩해야함

	TraceA( "LoadMission load start" );

	uint64 _temp64 = 0;
	General::TaskProgress _quest;

	for ( int row = 1; row < recordCount; ++row ) {
		auto rowData = fileLoader->GetRecord( row );
		int colCount = rowData->m_vecField.size();

		_quest.Clear();

		// ID
		//std::string idString( rowData->m_vecField[ 0 ].begin() , rowData->m_vecField[ 0 ].end() );

		// AchieveType
		std::string achieveTypeString( rowData->m_vecField[ 0 ].begin() , rowData->m_vecField[ 0 ].end() );
		if ( achieveTypeString == "Mission" )
			achieveTypeString = "TaskCategory_Mission";
		else if ( achieveTypeString == "LoungeMission" )
			achieveTypeString = "TaskCategory_LoungeMission";
		else if ( achieveTypeString == "Achievement" )
			achieveTypeString = "TaskCategory_Achievement";
		General::TaskCategory achiveType = static_cast< General::TaskCategory >( General::TaskCategory_descriptor()->FindValueByName( achieveTypeString )->number() );
		_quest.set_task_category( achiveType );

		//// QUEST GROUP ID
		//int quest_group_id = _wtoi( rowData->m_vecField[ 2 ].c_str() );
		//_quest.set_task_group_id( quest_group_id );

		// QUEST ID
		int quest_id = _wtoi( rowData->m_vecField[ 1 ].c_str() );
		_quest.set_task_id( quest_id );

		// QUEST MAJOR
		//_wtoi( rowData->m_vecField[ 4 ].c_str() );

		// QUEST SUB INDEX
		//_wtoi( rowData->m_vecField[ 5 ].c_str() );

		// QUEST SEQUENCE
		//_wtoi( rowData->m_vecField[ 6 ].c_str() );

		// Sort_IDX
		//_wtoi( rowData->m_vecField[ 7 ].c_str() );

		// TaskTrigger
		std::string achieveEventTypeString( rowData->m_vecField[ 2 ].begin() , rowData->m_vecField[ 2 ].end() );
		if ( achieveEventTypeString == "LowBadukiPlay" )
			achieveEventTypeString = "TaskTrigger_LowBadugiPlay";
		else if ( achieveEventTypeString == "LowBadukiEnemyAllin" )
			achieveEventTypeString = "TaskTrigger_LowBadugiEnemyAllIn";
		else if ( achieveEventTypeString == "LowBadukiJokbo7Made" )
			achieveEventTypeString = "TaskTrigger_LowBadugiSevenMade";
		else if ( achieveEventTypeString == "LowBadukiWinCount" )
			achieveEventTypeString = "TaskTrigger_LowBadugiWin";
		else if ( achieveEventTypeString == "HoldemPlay" )
			achieveEventTypeString = "TaskTrigger_HoldemPlay";
		else if ( achieveEventTypeString == "HoldemWinCount" )
			achieveEventTypeString = "TaskTrigger_HoldemWin";
		else if ( achieveEventTypeString == "BaccaratPlay" || achieveEventTypeString == "BaccaraPlay" )
			achieveEventTypeString = "TaskTrigger_BaccaratPlay";
		else if ( achieveEventTypeString == "BaccaratWinCount" || achieveEventTypeString == "BaccaraWinCount" )
			achieveEventTypeString = "TaskTrigger_BaccaratWin";
		else if ( achieveEventTypeString == "BlackjackPlay" )
			achieveEventTypeString = "TaskTrigger_BlackjackPlay";
		else if ( achieveEventTypeString == "BlackjackWinCount" )
			achieveEventTypeString = "TaskTrigger_BlackjackWin";
		else if ( achieveEventTypeString == "SlotSpinCount" )
			achieveEventTypeString = "TaskTrigger_SlotSpin";
		else if ( achieveEventTypeString == "NickChangeCount" )
			achieveEventTypeString = "TaskTrigger_AliasChange";
		else if ( achieveEventTypeString == "EnemyAllin" )
			achieveEventTypeString = "TaskTrigger_EnemyAllIn";
		else if ( achieveEventTypeString == "AdWatchCount" )
			achieveEventTypeString = "TaskTrigger_AdView";
		else if ( achieveEventTypeString == "ReachToChip" )
			achieveEventTypeString = "TaskTrigger_ChipReach";
		else if ( achieveEventTypeString == "GameWinGetChip" )
			achieveEventTypeString = "TaskTrigger_GameWinChip";
		else if ( achieveEventTypeString == "FriendRequest" )
			achieveEventTypeString = "TaskTrigger_FriendRequest";
		/*else if ( achieveEventTypeString == "PinballPlayCount" )
			achieveEventTypeString = "TaskTrigger_PinballPlay";*/
		else if ( achieveEventTypeString == "HoldemJokboFlush" )
			achieveEventTypeString = "TaskTrigger_HoldemFlush";
		else if ( achieveEventTypeString == "HoldemEnemyAllin" )
			achieveEventTypeString = "TaskTrigger_HoldemEnemyAllIn";
		//else if ( achieveEventTypeString == "RoulettePlay" )
		//	achieveEventTypeString = "TaskTrigger_RoulettePlay";
		General::TaskTrigger achieveType = static_cast< General::TaskTrigger >( General::TaskTrigger_descriptor()->FindValueByName( achieveEventTypeString )->number() );
		_quest.set_task_trigger( achieveType );

		// QUEST GOAL COUNT
		wchar_t* endPtr; // std::wcstoull 함수에서 변환 중 오류 발생 시 오류 위치를 알려주는 포인터
		uint64_t quest_goal_count = std::wcstoull( rowData->m_vecField[ 3 ].c_str() , &endPtr , 10 ); // 10진수로 변환

		// 오류 처리
		if ( *endPtr != L'\0' ) {
			// 변환 중 오류 발생 시 처리
			// 오류 처리에 대한 예를 추가하세요
		}

		_quest.set_target_progress( quest_goal_count );

		// REWARD TYPE
		std::string rewardTypeString( rowData->m_vecField[ 4 ].begin() , rowData->m_vecField[ 4 ].end() );
		std::string t_rewardTypeString;
		if ( rewardTypeString == "Coin" )
			t_rewardTypeString = "GrantItem_FreeCoin";
		else if ( rewardTypeString == "Chip" )
			t_rewardTypeString = "GrantItem_FreeChip";
		else if ( rewardTypeString == "Item" )
			t_rewardTypeString = "GrantItem_InventoryItem";
		else if ( rewardTypeString == "Paid_Coin" )
			t_rewardTypeString = "GrantItem_PaidCoin";
		else if ( rewardTypeString == "Paid_Chip" )
			t_rewardTypeString = "GrantItem_PaidChip";
		else if ( rewardTypeString == "Gem" )
			t_rewardTypeString = "GrantItem_FreeGem";
		else if ( rewardTypeString == "Paid_Gem" )
			t_rewardTypeString = "GrantItem_PaidGem";
		else if ( rewardTypeString == "Class" )
			t_rewardTypeString = "GrantItem_BenefitTier";
		else if ( rewardTypeString == "KickoutTicket" )
			t_rewardTypeString = "GrantItem_KickTicket";
		else
			t_rewardTypeString = "GrantItem_" + rewardTypeString;
		General::GrantItemKind rewardType = static_cast< General::GrantItemKind >( General::GrantItemKind_descriptor()->FindValueByName( t_rewardTypeString )->number() );
		_quest.set_grant_kind( rewardType );

		uint64 reward_item_count = std::wcstoull( rowData->m_vecField[ 5 ].c_str() , &endPtr , 10 ); // 10진수로 변환

		// 오류 처리
		if ( *endPtr != L'\0' ) {
			// 변환 중 오류 발생 시 처리
			// 오류 처리에 대한 예를 추가하세요
		}

		m_quests.insert( std::pair<uint32 , General::TaskProgress>( _quest.task_id() , _quest ) );
		m_quest_reward_map.insert(std::pair<uint32, uint64>( _quest.task_id(), reward_item_count ));

		switch ( achiveType )
		{
		case General::TaskCategory::TaskCategory_Mission:
			m_daily_missions.insert( std::pair<uint32 , General::TaskProgress>( _quest.task_id() , _quest ) );
			break;
		case General::TaskCategory::TaskCategory_LoungeMission:
			m_lounge_missions.insert( std::pair<uint32 , General::TaskProgress>( _quest.task_id() , _quest ) );
			break;
		case General::TaskCategory::TaskCategory_Achievement:
		{
			// 그룹아이디가 있는지 여부에 따라	분기
			if ( _quest.task_group_id() == 0 )
				m_achievements.insert( std::pair<uint32 , General::TaskProgress>( _quest.task_id() , _quest ) );
			else {

				auto iter = m_group_achievements.find( _quest.task_group_id() );
				if ( iter == m_group_achievements.end() )
				{
					std::vector<General::TaskProgress> quests;
					quests.push_back( _quest );
					m_group_achievements.insert(std::pair<int32 , std::vector<General::TaskProgress>>( _quest.task_group_id() , quests ));
				}
				else
				{
					iter->second.push_back( _quest );
				}
			}
		}
		break;
		}

		std::string serializedData;
		google::protobuf::util::MessageToJsonString( _quest , &serializedData );
		TraceA( serializedData );
	}

	std::string errorInfo = std::format( "LoadMission m_quests load complete row count {}" , m_quests.size() );
	TraceA( errorInfo );

	NetLib::cSingleton<NetLib::csvFileLoader>::DeleteInstance();
}

void cDataLoader::LotteryReward( const TCHAR* fileName )
{
	TCHAR csvFileFullPath[ MAX_PATH ];
	_sntprintf_s( csvFileFullPath , MAX_PATH , _T( "%s\\%s" ) , m_DataDirectory , fileName );

	auto fileLoader = NetLib::cSingleton<NetLib::csvFileLoader>::GetInstance();

	fileLoader->Create( csvFileFullPath );
	fileLoader->Open();
	int recordCount = fileLoader->GetRecordCount();

	auto record_1 = fileLoader->GetRecord( 0 ); // 컬럼데이터 구조
	int columnCount = record_1->m_vecField.size(); // end 는 빼고 로딩해야함

	TraceA( "LotteryReward load start" );

	Server::LotteryData _lotteryData;

	for ( int row = 1; row < recordCount; ++row ) {
		auto rowData = fileLoader->GetRecord( row );
		int colCount = rowData->m_vecField.size();

		_lotteryData.Clear();

		// ID
		std::string idString( rowData->m_vecField[ 0 ].begin() , rowData->m_vecField[ 0 ].end() );
		if ( idString.size() == 0 )
			continue;

		// MONEY TYPE
		std::string moneyTypeString = "AssetKind_" + string( rowData->m_vecField[ 1 ].begin() , rowData->m_vecField[ 1 ].end() );
		General::AssetKind moneyType = static_cast< General::AssetKind >( General::AssetKind_descriptor()->FindValueByName( moneyTypeString )->number() );

		// VALUE
		wchar_t* endPtr; // std::wcstoull 함수에서 변환 중 오류 발생 시 오류 위치를 알려주는 포인터
		uint64 money_value = std::wcstoull( rowData->m_vecField[ 2 ].c_str() , &endPtr , 10 ); // 10진수로 변환

		// 오류 처리
		if ( *endPtr != L'\0' ) {
			// 변환 중 오류 발생 시 처리
			// 오류 처리에 대한 예를 추가하세요
		}

		// RATE ( 1백만분의 1 )
		endPtr = nullptr;
		uint64 rate_value = std::wcstoull( rowData->m_vecField[ 3 ].c_str() , &endPtr , 10 );

		// 오류 처리
		if ( *endPtr != L'\0' ) {
			// 변환 중 오류 발생 시 처리
			// 오류 처리에 대한 예를 추가하세요
		}

		_lotteryData.set_rate( rate_value );
		_lotteryData.set_money_value( money_value );
		_lotteryData.set_money_type( moneyType );

		m_lottery_datas.push_back( _lotteryData );

		std::string serializedData;
		google::protobuf::util::MessageToJsonString( _lotteryData , &serializedData );
		TraceA( serializedData );
	}

	std::string errorInfo = std::format( "LotteryReward load complete row count {}" , m_lottery_datas.size() );
	TraceA( errorInfo );

	NetLib::cSingleton<NetLib::csvFileLoader>::DeleteInstance();
}

void cDataLoader::LoadItemProduct( const TCHAR* fileName )
{
	TCHAR csvFileFullPath[ MAX_PATH ];
	_sntprintf_s( csvFileFullPath , MAX_PATH , _T( "%s\\%s" ) , m_DataDirectory , fileName );

	auto fileLoader = NetLib::cSingleton<NetLib::csvFileLoader>::GetInstance();

	fileLoader->Create( csvFileFullPath );
	fileLoader->Open();
	int recordCount = fileLoader->GetRecordCount();

	auto record_1 = fileLoader->GetRecord( 0 ); // 컬럼데이터 구조
	int columnCount = record_1->m_vecField.size(); // end 는 빼고 로딩해야함

	// ID				,	PRICE		  ,	REWARD ( / 로 아이템 구분 ) 	
	// ITEM_AVATAR_1_30D,	GEMMOND : 1000,	PICK : AVATAR_1_DAY30 / CHIP : 100800000000


	TraceA( "LoadItemProduct load start" );

	Server::ShopProduct _productData;

	for ( int row = 1; row < recordCount; ++row ) {
		auto rowData = fileLoader->GetRecord( row );
		int colCount = rowData->m_vecField.size();

		_productData.Clear();

		// ID
		std::string idString( rowData->m_vecField[ 0 ].begin() , rowData->m_vecField[ 0 ].end() );
		if ( idString.size() == 0 )
			continue;

		_productData.set_id( idString );

		wchar_t* endPtr; // std::wcstoull 함수에서 변환 중 오류 발생 시 오류 위치를 알려주는 포인터
		

		// PRICE
		std::string priceString( rowData->m_vecField[ 1 ].begin() , rowData->m_vecField[ 1 ].end() );

		size_t colonPos = priceString.find( ':' );
		if ( colonPos != std::string::npos ) {
			// If ':' is found, extract the key-value pair and add it to the result
			std::string key = priceString.substr( 0 , colonPos );
			std::string value = priceString.substr( colonPos + 1 );

			if ( key.compare( "GEM" ) == 0 ) {
				_productData.set_buy_money_type( General::AssetKind::AssetKind_Gem );

				uint64 money_value = std::stoull( value.c_str() ); // 10진수로 변환
				_productData.set_money_value( money_value );
			}
			else
				// 데이터 로딩 오류
				throw std::invalid_argument( "Invalid data format: PRICE Type Error" );
		}
		else {
			// 데이터 로딩 오류
			throw std::invalid_argument( "Invalid data format: '/' not found." );
		}
		
		// ITEMID 아바타인 경우에 사용된다.
		std::string itemIdString( rowData->m_vecField[ 2 ].begin() , rowData->m_vecField[ 2 ].end() );
		StringUtil::RemoveSpaces( itemIdString );
		int item_id = 0;
		if ( itemIdString.size() != 0 ) {
			item_id = atoi( itemIdString.c_str() );
		}
		
		// ACTIVATE 사용 유무 1만 사용한다.
		std::string activateString( rowData->m_vecField[ 3 ].begin() , rowData->m_vecField[ 3 ].end() );
		StringUtil::RemoveSpaces( activateString );
		if ( activateString.size() != 0 ) {
			_productData.set_isactivated( true );
		}

		// REWARD
		std::string rewardString( rowData->m_vecField[ 4 ].begin() , rowData->m_vecField[ 4 ].end() );

		// 첫번째 : 을 찾아서 구분을 한다.
		size_t pos = 0;
		size_t semiPos = rewardString.find( ':' , pos );
		std::string semiString = rewardString.substr( 0 , semiPos );

		// TICKET 상품
		if ( semiString.compare( "KICKOUTTICKET" ) == 0 )
		{
			//_productData.set_item_id( 111 ); // KICKOUT 아이템 아이디 타입
			

			std::string value = rewardString.substr( semiPos + 1 );
			int32 item_count = atoi( value.c_str() ); // 10진수로 변환
			//_productData.set_item_count( item_count );

			_productData.set_kick_out_ticket_count( item_count );
		}
		// 멤버쉽 Class 상품
		else if ( semiString.compare( "MEMBERSHIP" ) == 0 )
		{
			pos = 0;
			size_t slashPos = rewardString.find( '/' , pos );

			std::vector<std::string> classProducts;
			classProducts.push_back( rewardString.substr( 0 , slashPos ) );
			classProducts.push_back( rewardString.substr( slashPos + 1 , rewardString.size() ) );

			for ( auto& classProduct : classProducts ) {

				size_t colonPos = classProduct.find( ':' );
				if ( colonPos != std::string::npos ) {
					// If ':' is found, extract the key-value pair and add it to the result
					std::string key = classProduct.substr( 0 , colonPos );
					std::string value = classProduct.substr( colonPos + 1 );

					if ( key.compare( "MEMBERSHIP" ) == 0 ) {
						
						if ( value.compare( "STANDARD" ) == 0 ) {
							_productData.set_member_ship_class( General::BenefitTier::BenefitTier_Standard ); // 멤버쉽 클라스 REGULAR 아이템 아이디
						}
						else if ( value.compare( "PREMIUM" ) == 0 ) {
							_productData.set_member_ship_class( General::BenefitTier::BenefitTier_Premium ); // 멤버쉽 클라스 TOP  아이템 아이디
						}
					}
					else if ( key.compare( "PERIODSEC" ) == 0 ) {

						int day = atoi( value.c_str() ) / 3600 / 24;
						_productData.set_membership_add_days( day );
					}

				}
			}
		}
		// 아바타 상품
		else if ( semiString.compare( "PICK" ) == 0 )
		{
			pos = 0;
			size_t slashPos = rewardString.find( '/' , pos );

			std::vector<std::string> classProducts;
			classProducts.push_back( rewardString.substr( 0 , slashPos ) );
			classProducts.push_back( rewardString.substr( slashPos + 1 , rewardString.size() ) );

			for ( auto& classProduct : classProducts ) {

				size_t colonPos = classProduct.find( ':' );
				if ( colonPos != std::string::npos ) {
					// If ':' is found, extract the key-value pair and add it to the result
					std::string key = classProduct.substr( 0 , colonPos );
					std::string value = classProduct.substr( colonPos + 1 );

					if ( key.compare( "PICK" ) == 0 ) {

						//std::string avatar_id = ExtractAvatarId( value );
						int add_days = ExtractNumberAfterDay( value );
						
						_productData.set_avatar_id( item_id );
						//_productData.set_avatar_id( atoi( avatar_id.c_str() ) );
						_productData.set_avatar_add_days( add_days );
					}
					else if ( key.compare( "CHIP" ) == 0 ) {

						char* endPtr; // std::wcstoull 함수에서 변환 중 오류 발생 시 오류 위치를 알려주는 포인터
						uint64 money_value = std::strtoull( value.c_str() , &endPtr , 10 ); // 10진수로 변환

						//uint32 money_value = atoi( value.c_str() );
						_productData.set_paid_chips( money_value );
					}

				}
			}
		}
		
		m_item_products.insert(std::pair<std::string , Server::ShopProduct>( _productData.id(), _productData ));

		std::string serializedData;
		google::protobuf::util::MessageToJsonString( _productData , &serializedData );
		TraceA( serializedData );
	}

	std::string errorInfo = std::format( "LoadItemProduct load complete row count {}" , m_item_products.size() );
	TraceA( errorInfo );

	NetLib::cSingleton<NetLib::csvFileLoader>::DeleteInstance();
}

void cDataLoader::LoadStoreProduct( const TCHAR* fileName )
{
	TCHAR csvFileFullPath[ MAX_PATH ];
	_sntprintf_s( csvFileFullPath , MAX_PATH , _T( "%s\\%s" ) , m_DataDirectory , fileName );

	auto fileLoader = NetLib::cSingleton<NetLib::csvFileLoader>::GetInstance();

	fileLoader->Create( csvFileFullPath );
	fileLoader->Open();
	int recordCount = fileLoader->GetRecordCount();

	auto record_1 = fileLoader->GetRecord( 0 ); // 컬럼데이터 구조
	int columnCount = record_1->m_vecField.size(); // end 는 빼고 로딩해야함

	// ID				,	PRICE		  ,	REWARD ( / 로 아이템 구분 ) 	
	// ITEM_AVATAR_1_30D,	GEMMOND : 1000,	PICK : AVATAR_1_DAY30 / CHIP : 100800000000


	TraceA( "LoadStoreProduct load start" );

	Server::ShopProduct _productData;

	for ( int row = 1; row < recordCount; ++row ) {
		auto rowData = fileLoader->GetRecord( row );
		int colCount = rowData->m_vecField.size();

		_productData.Clear();

		// ID
		std::string idString( rowData->m_vecField[ 0 ].begin() , rowData->m_vecField[ 0 ].end() );
		if ( idString.size() == 0 )
			continue;

		_productData.set_id( idString );

		// PRICE
		std::string priceString( rowData->m_vecField[ 1 ].begin() , rowData->m_vecField[ 1 ].end() );
		int32 price = atoi( priceString.c_str() ); // 10진수로 변환
		_productData.set_price( price );

		// REWARD
		std::string rewardString( rowData->m_vecField[ 2 ].begin() , rowData->m_vecField[ 2 ].end() );
		
		// 첫번째 : 을 찾아서 구분을 한다.
		size_t pos = 0;
		size_t semiPos = rewardString.find( ':' , pos );
		std::string semiString = rewardString.substr( 0 , semiPos );\
		size_t listItemDeciderPos = rewardString.find( '/' , 0 );

		// GEM 상품
		if ( semiString.compare( "PAIDGEM" ) == 0 && listItemDeciderPos == std::string::npos )
		{
			std::string value = rewardString.substr( semiPos + 1 );
			int32 item_count = atoi( value.c_str() ); // 10진수로 변환
			_productData.set_paid_gem( item_count );
		}
		// 멤버쉽 Class 상품
		else if ( semiString.compare( "MEMBERSHIP" ) == 0 )
		{
			pos = 0;
			size_t slashPos = rewardString.find( '/' , pos );

			std::vector<std::string> classProducts;
			classProducts.push_back( rewardString.substr( 0 , slashPos ) );
			classProducts.push_back( rewardString.substr( slashPos + 1 , rewardString.size() ) );

			for ( auto& classProduct : classProducts ) {

				size_t colonPos = classProduct.find( ':' );
				if ( colonPos != std::string::npos ) {
					// If ':' is found, extract the key-value pair and add it to the result
					std::string key = classProduct.substr( 0 , colonPos );
					std::string value = classProduct.substr( colonPos + 1 );

					if ( key.compare( "MEMBERSHIP" ) == 0 ) {

						if ( value.compare( "STANDARD" ) == 0 ) {
							_productData.set_member_ship_class( General::BenefitTier::BenefitTier_Standard ); // 멤버쉽 클라스 REGULAR 아이템 아이디
						}
						else if ( value.compare( "PREMIUM" ) == 0 ) {
							_productData.set_member_ship_class( General::BenefitTier::BenefitTier_Premium ); // 멤버쉽 클라스 TOP  아이템 아이디
						}
					}
					else if ( key.compare( "PERIODSEC" ) == 0 ) {

						int day = atoi( value.c_str() ) / 3600 / 24;
						_productData.set_membership_add_days( day );
					}

				}
			}
		}
		// 묶음 아이템 인경우
		else if ( semiString.compare( "PAIDGEM" ) == 0 && listItemDeciderPos != std::string::npos )
		{
			pos = 0;
			size_t slashPos = rewardString.find( '/' , pos );

			std::vector<std::string> classProducts;
			classProducts.push_back( rewardString.substr( 0 , slashPos ) );
			classProducts.push_back( rewardString.substr( slashPos + 1 , rewardString.size() ) );

			for ( auto& classProduct : classProducts ) {

				size_t colonPos = classProduct.find( ':' );
				if ( colonPos != std::string::npos ) {
					// If ':' is found, extract the key-value pair and add it to the result
					std::string key = classProduct.substr( 0 , colonPos );
					std::string value = classProduct.substr( colonPos + 1 );

					char* endPtr; // std::wcstoull 함수에서 변환 중 오류 발생 시 오류 위치를 알려주는 포인터
					uint64 item_count = std::strtoull( value.c_str() , &endPtr , 10 ); // 10진수로 변환

					//int32 item_count = atoi( value.c_str() ); // 10진수로 변환

					if ( key.compare( "PAIDGEM" ) == 0 ) {
						_productData.set_paid_gem( item_count );
					}
					else if ( key.compare( "CHIP" ) == 0 ) {

						_productData.set_paid_chips( item_count );
					}

				}
			}
		}

		// PRDID_G
		std::string google_product_id( rowData->m_vecField[ 3 ].begin() , rowData->m_vecField[ 3 ].end() );
		_productData.set_google_product_id( google_product_id );

		// PRDID_A
		std::string apple_product_id( rowData->m_vecField[ 4 ].begin() , rowData->m_vecField[ 4 ].end() );
		_productData.set_apple_product_id( apple_product_id );

		// PRDID_O
		std::string onestore_product_id( rowData->m_vecField[ 5 ].begin() , rowData->m_vecField[ 5 ].end() );
		_productData.set_onestore_product_id( onestore_product_id );

		m_google_store_products.insert( std::pair<std::string , Server::ShopProduct>( google_product_id , _productData ) );
		m_google_store_products_by_id.insert( std::pair<std::string , Server::ShopProduct>( _productData.id() , _productData));

		m_apple_store_products.insert( std::pair<std::string , Server::ShopProduct>( apple_product_id , _productData ) );
		m_onetstore_store_products.insert( std::pair<std::string , Server::ShopProduct>( onestore_product_id , _productData ) );

		std::string serializedData;
		google::protobuf::util::MessageToJsonString( _productData , &serializedData );
		TraceA( serializedData );
	}

	std::string errorInfo = std::format( "LoadStoreProduct load complete store item count {}" , m_google_store_products.size() );
	TraceA( errorInfo );

	NetLib::cSingleton<NetLib::csvFileLoader>::DeleteInstance();
}

void cDataLoader::LoadAttendanceReward( const TCHAR* fileName )
{
	TCHAR csvFileFullPath[ MAX_PATH ];
	_sntprintf_s( csvFileFullPath , MAX_PATH , _T( "%s\\%s" ) , m_DataDirectory , fileName );

	auto fileLoader = NetLib::cSingleton<NetLib::csvFileLoader>::GetInstance();

	fileLoader->Create( csvFileFullPath );
	fileLoader->Open();
	int recordCount = fileLoader->GetRecordCount();

	auto record_1 = fileLoader->GetRecord( 0 ); // 컬럼데이터 구조
	int columnCount = record_1->m_vecField.size(); // end 는 빼고 로딩해야함

	TraceA( "LoadAttendanceReward load start" );

	Server::ShopProduct _productData;
	for ( int row = 1; row < recordCount; ++row ) {
		auto rowData = fileLoader->GetRecord( row );
		int colCount = rowData->m_vecField.size();
		if ( colCount < 2 ) continue;

		_productData.Clear();

		std::string idString( rowData->m_vecField[ 0 ].begin() , rowData->m_vecField[ 0 ].end() );
		if ( idString.empty() ) continue;
		_productData.set_id( idString );

		std::string rewardString( rowData->m_vecField[ 1 ].begin() , rowData->m_vecField[ 1 ].end() );
		StringUtil::RemoveSpaces( rewardString );

		size_t colonPos = rewardString.find( ':' );
		if ( colonPos == std::string::npos ) continue;

		std::string key = rewardString.substr( 0 , colonPos );
		std::string value = rewardString.substr( colonPos + 1 );

		if ( key.compare( "CHIP" ) == 0 ) {
			char* endPtr;
			uint64 chip_value = std::strtoull( value.c_str() , &endPtr , 10 );
			_productData.set_paid_chips( chip_value );
		}
		else if ( key.compare( "PICK" ) == 0 ) {
			std::string avatarIdStr = ExtractAvatarId( value );
			int avatar_id = atoi( avatarIdStr.c_str() );
			int add_days = ExtractNumberAfterDay( value );
			_productData.set_avatar_id( avatar_id );
			_productData.set_avatar_add_days( add_days );
		}
		m_attendance_rewards.push_back( _productData );
	}

	std::string errorInfo = std::format( "LoadAttendanceReward load complete store item count {}" , m_attendance_rewards.size() );
	TraceA( errorInfo );

	NetLib::cSingleton<NetLib::csvFileLoader>::DeleteInstance();
}

Server::Channel cDataLoader::GetChannelById( const std::string _id )
{
	auto iter = m_channelList.find( _id );
	if ( iter == m_channelList.end() )
		return Server::Channel::default_instance();

	return iter->second;
}

Server::Channel cDataLoader::FindChannel( const General::PlayCategory& gameType , const General::AssetKind& moneyType , const uint64& seedMoneyValue )
{
	for ( auto seedMoney : m_channelList )
	{
		if ( seedMoney.second.game_type() == gameType &&
			seedMoney.second.money_type() == moneyType &&
			seedMoney.second.seed_money() == seedMoneyValue )
			return seedMoney.second;
	}
	return Server::Channel::default_instance();
}

DWORD cDataLoader::convert_unicode_to_ansi_string( 
	__out std::string& ansi ,
	__in const wchar_t* unicode ,
	__in const size_t unicode_size )
{

	DWORD error = 0;

	do {

		if ( ( nullptr == unicode ) || ( 0 == unicode_size ) ) {
			error = ERROR_INVALID_PARAMETER;
			break;
		}

		ansi.clear();

		//        
		// getting required cch.
		//         

		int required_cch = ::WideCharToMultiByte(
			CP_ACP ,
			0 ,
			unicode ,
			static_cast< int >( unicode_size ) ,
			nullptr ,
			0 ,
			nullptr ,
			nullptr );

		if ( 0 == required_cch ) {
			error = ::GetLastError();
			break;
		}

		//        
		// allocate.        
		//         


		ansi.resize( required_cch );

		//        
		// convert.        
		//         

		if ( 0 == ::WideCharToMultiByte(
			CP_ACP ,
			0 ,
			unicode ,
			static_cast< int >( unicode_size ) ,
			const_cast< char* >( ansi.c_str() ) ,
			static_cast< int >( ansi.size() ) ,
			nullptr ,
			nullptr
		) )
		{
			error = ::GetLastError();
			break;
		}

	} while ( false );

	return error;
}

DWORD cDataLoader::convert_unicode_to_utf8_string( __out std::string& utf8 , __in const wchar_t* unicode , __in const size_t unicode_size ) {
	DWORD error = 0;     do {
		if ( ( nullptr == unicode ) || ( 0 == unicode_size ) ) { error = ERROR_INVALID_PARAMETER;            break; }         utf8.clear();         
		//        // getting required cch.        //         
		int required_cch = ::WideCharToMultiByte(                                
			CP_UTF8,                                
			WC_ERR_INVALID_CHARS,                                
			unicode, 
			static_cast<int>(unicode_size),                                
			nullptr, 
			0,                                
			nullptr, 
			nullptr                                
		);         
		
		if (0 == required_cch) {            
			error = ::GetLastError();            
			break;        
		}         
		
		//        // allocate.        //         
		// 
		utf8.resize(required_cch);         
		
		//        // convert.        //         
		// 
		
		if (0 == ::WideCharToMultiByte(                    
			CP_UTF8,                    
			WC_ERR_INVALID_CHARS,                    
			unicode, 
			static_cast<int>(unicode_size),                    
			const_cast<char*>(utf8.c_str()), 
			static_cast<int>(utf8.size()),                    
			nullptr, 
			nullptr                    
		)) 
		{            
			error = ::GetLastError();            
			break;        
		}     
	} while (false);     

	return error;
}

// 입력된 문자열에서 "AVATAR_" 다음의 "_" 사이의 문자열을 추출하는 함수
std::string cDataLoader::ExtractAvatarId( const std::string& input )
{
	// "AVATAR_"의 위치를 찾음
	size_t startPos = input.find( "AVATAR_" );
	if ( startPos == std::string::npos ) {
		// "AVATAR_"가 없으면 빈 문자열 반환
		return "";
	}

	// "AVATAR_" 이후의 문자열에서 다음 "_"의 위치를 찾음
	size_t endPos = input.find( "_" , startPos + 7 );
	if ( endPos == std::string::npos ) {
		// 다음 "_"가 없으면 빈 문자열 반환
		return "";
	}

	// "AVATAR_" 이후의 문자열에서 다음 "_"까지의 부분 문자열을 추출하여 반환
	return input.substr( startPos + 7 , endPos - startPos - 7 );
}

int cDataLoader::ExtractNumberAfterDay( const std::string& input )
{
	// "DAY"의 위치를 찾음
	size_t dayPos = input.find( "DAY" );
	if ( dayPos == std::string::npos ) {
		// "DAY"가 없으면 0 반환
		return 0;
	}

	// "DAY" 이후의 문자열에서 숫자 부분을 추출하여 반환
	std::string numberStr;
	for ( size_t i = dayPos + 3; i < input.size(); ++i ) {
		if ( std::isdigit( input[ i ] ) ) {
			numberStr += input[ i ];
		}
		else {
			// 숫자가 아닌 문자가 나오면 루프를 종료하고 숫자를 반환
			break;
		}
	}

	// 추출된 숫자 부분을 정수로 변환하여 반환
	return std::stoi( numberStr );
}

void cDataLoader::LoadSystemData( const TCHAR* fileName )
{
	TCHAR csvFileFullPath[ MAX_PATH ];
	_sntprintf_s( csvFileFullPath , MAX_PATH , _T( "%s\\%s" ) , m_DataDirectory , fileName );

	auto fileLoader = NetLib::cSingleton<NetLib::csvFileLoader>::GetInstance();

	fileLoader->Create( csvFileFullPath );
	fileLoader->Open();
	int recordCount = fileLoader->GetRecordCount();

	auto record_1 = fileLoader->GetRecord( 0 );
	int columnCount = record_1->m_vecField.size();

	TraceA( "LoadSystemData load start" );

	for ( int row = 1; row < recordCount; ++row ) {
		auto rowData = fileLoader->GetRecord( row );
		int colCount = rowData->m_vecField.size();

		// ID
		int _id = _wtoi( rowData->m_vecField[ 0 ].c_str() );
		if ( _id == 0 )
			continue;

		int index = 2;
		while ( true )
		{
			if ( record_1->m_vecField[ index ] == L"End" )
				break;

			std::string DataValue( rowData->m_vecField[ index ].begin() , rowData->m_vecField[ index ].end() );
			m_SystemData[ _id ].push_back( DataValue );
			++index;
		}
	}

	std::string errorInfo = std::format( "LoadSystemData load complete row count {}" , m_channelList.size() );
	TraceA( errorInfo );

	NetLib::cSingleton<NetLib::csvFileLoader>::DeleteInstance();
}





//void cDataLoader::LoadPinballMissRate( const TCHAR* fileName )
//{
//	TCHAR csvFileFullPath[ MAX_PATH ];
//	_sntprintf_s( csvFileFullPath , MAX_PATH , _T( "%s\\%s" ) , m_DataDirectory , fileName );
//
//	auto fileLoader = NetLib::cSingleton<NetLib::csvFileLoader>::GetInstance();
//
//	fileLoader->Create( csvFileFullPath );
//	fileLoader->Open();
//	int recordCount = fileLoader->GetRecordCount();
//
//	TraceA( "LoadPinballMissRate load start" );
//
//	// 헤더 스킵
//	for ( int row = 1; row < recordCount; ++row )
//	{
//		auto rowData = fileLoader->GetRecord( row );
//		int colCount = rowData->m_vecField.size();
//		if ( colCount < 3 )
//			continue;
//
//		// 컬럼 순서: ballCount, missRate1, missRate2
//		int ballCount = _wtoi( rowData->m_vecField[ 0 ].c_str() );
//		int missRate1 = _wtoi( rowData->m_vecField[ 1 ].c_str() );
//		int missRate2 = _wtoi( rowData->m_vecField[ 2 ].c_str() );
//
//		missRateMap[ ballCount ] = missRate1;
//		missRateMap2[ ballCount ] = missRate2;
//	}
//
//	std::string log = std::format( "LoadPinballMissRate load complete. row count: {}" , recordCount - 1 );
//	TraceA( log );
//
//	NetLib::cSingleton<NetLib::csvFileLoader>::DeleteInstance();
//}




