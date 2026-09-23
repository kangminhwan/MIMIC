#pragma once
#include "TableServerHeader.h"

#include "..\..\ProtocolBuffer\cpp\Server.pb.h"

#include "cLevelManager.h"
//#include "cPinballManager.h"
enum SystemDataID
{
	None = 0 ,
	live_or_patch = 1 ,

	max ,
};

enum SystemDataValue
{
	Data1 = 0 ,
	Data2 = 1 ,
	Data3 = 2 ,
};



class cDataLoader
{
private:
	std::map<int , std::vector<std::string>> m_SystemData;
	std::set<string>  Vip_Uid_List;
	std::set<string>  Vip_LOG_List;

	TCHAR m_DataDirectory[MAX_PATH];
	std::map<std::wstring, int> m_configDataMap;
	std::map<General::PlayCategory , int> m_channelDistribution;												// 게임 타입별 로비 서버 분산
	std::map<std::string , Server::Channel> m_channelList;
	std::map<General::LobbyButtonPreset , std::vector<General::TableAction>> m_buttonList;


	std::vector<Server::CasSymbolId> m_symbolDataList;											// 심볼 데이터 전체 목록




	std::vector<Server::Level> m_Levels;														// 레벨 테이블 로딩

	std::vector<General::AvatarProfile> m_avatars;														// 아바타 데이터 로딩
	std::vector<General::AvatarProfile> m_free_avatars;													// 프리 아바타 데이터 로딩
	std::map<int , General::AvatarProfile> m_avatar_map;												// 아바타 맵

	std::map< uint32 , General::TaskProgress> m_quests;													// 미션, 라운지 미션, 업적 전체 목록
	std::map< uint32 , uint64 > m_quest_reward_map;												// 퀘스트 리워드 맵

	std::map<int32 , General::TaskProgress> m_daily_missions;											// 일별 미션
	std::map<int32 , General::TaskProgress> m_lounge_missions;											// 라운지 미션
	std::map<int32 , General::TaskProgress> m_achievements;												// 업적
	std::map<int32 , std::vector<General::TaskProgress>> m_group_achievements;							// 연속 업적

	std::vector<Server::LotteryData> m_lottery_datas;											// 무료 충전소 데이터

	std::map<std::string , Server::ShopProduct> m_item_products;								// 상점 : 다이아로 구매 가능한 상품 목록
	std::map<std::string , Server::ShopProduct> m_google_store_products;						// 상점 : 구글 인앱결제 구매 가능한 상품 목록
	std::map<std::string , Server::ShopProduct> m_google_store_products_by_id;					// 상점 : 구글 인앱결제 구매 가능한 상품 목록, 템플릿 id로 정렬
	std::map<std::string , Server::ShopProduct> m_apple_store_products;							// 상점 : 애플 인앱결제 구매 가능한 상품 목록
	std::map<std::string , Server::ShopProduct> m_onetstore_store_products;						// 상점 : 원스토어 인앱결제 구매 가능한 상품 목록

	std::vector<Server::ShopProduct> m_attendance_rewards;													// 출석 보상 칩

	std::map<Server::BaselineConfigKey , uint64> m_basicSettings;									// 기본 셋팅 데이터
	std::map<Server::BaselineConfigKey , std::vector<uint64>> m_ListArraySettings;

	// 해적 데이터







	General::InboxState m_mail_state;

	std::map<string , string> system_data;
	int dailyExpiredMonth;

	std::map<int , int> missRateMap;			//핀볼 미스레이트
	std::map<int , int> missRateMap2;			//핀볼 미스레이트
	//std::map<int , int> m_pinball_ratio;

	//PmNet::LoggedMarble m_recoded_pinball;
	////std::unordered_set<uint64_t> m_records;
	//std::set<std::string> m_15ball_records;
	//std::set<std::string> m_8ball_records;
	//std::set<Common::PinballType> m_PinballLockList;
public:
	cDataLoader();
	void LoadSystemDataDB();
	/*void LoadPinball();
	void LoadPinballLock();
	bool isPinballLock( const Common::PinballType& pinballType );
	bool LoadPinballFromS3( const std::string& url , PmNet::LoggedMarble& outPinball );*/
	//void LoadPinballMissRate( const TCHAR* fileName );
	
	void LoadGameConfig( const TCHAR* fileName );
	void LoadBasicSetting( const TCHAR* fileName );
	void LoadChannelList( const TCHAR* fileName );
	void LoadChannelData( const TCHAR* fileName );
	void LoadRestrictedWord( const TCHAR* fileName );
	void LoadButtonList( const TCHAR* fileName );
	
	void LoadSystemData( const TCHAR* fileName );








	int GetDailyExpiredMonth() { return dailyExpiredMonth; }
	void SetDailyExpiredMonth(int _month) { dailyExpiredMonth = _month; }
	
	string GetSystemData( const std::string data_type ) { 
		if ( system_data.find( data_type ) != system_data.end() )
			return system_data[ data_type ];
		else
			return"";
	}
	std::map<string , string>& GetSystemDataRef() { return system_data; }


	// Slot Sort 에 있는 Slot 들만 Ranking Data 를 넣어준다.


	// Pay 데이터 공통 처리






	// 슬롯 캐스케이딩 릴 Cascading_Reel

	//void LoadCasReelFreeGrandData( const TCHAR* fileName );



	
	// 슬롯 클러스터 릴 Culster_Pays_Reel

	//void LoadClustReelFreeGrandData( const TCHAR* fileName );


	// 슬롯 라인 릴 Line_Pay_Reel








#pragma region 슬롯 캐스케이딩 오버레이 릴





#pragma endregion 슬롯 캐스케이딩 오버레이 릴

	void LoadLevelData( const TCHAR* fileName );
	void LoadAvatar( const TCHAR* fileName );

	void LoadMission( const TCHAR* fileName );

	void LotteryReward( const TCHAR* fileName );

	// 상점 데이터 로딩
	void LoadItemProduct( const TCHAR* fileName );
	void LoadStoreProduct( const TCHAR* fileName );
	
	// 출석 데이터 로딩
	void LoadAttendanceReward(const TCHAR* fileName);

	static DWORD convert_unicode_to_ansi_string( __out std::string& ansi , __in const wchar_t* unicode , __in const size_t unicode_size );
	static DWORD convert_unicode_to_utf8_string( __out std::string& utf8 , __in const wchar_t* unicode , __in const size_t unicode_size );
	static std::string ExtractAvatarId( const std::string& input );
	static int ExtractNumberAfterDay( const std::string& input );

public:
	std::string GetSystemData( SystemDataID id , SystemDataValue value ) { return m_SystemData[ id ][ value ]; }

	Server::Channel GetChannelById( const std::string _id );
	Server::Channel FindChannel( const General::PlayCategory& gameType , const General::AssetKind& moneyType, const uint64& seedMoneyValue );
	std::map<std::string , Server::Channel> GetChannelList() { return m_channelList; }

	std::set<std::string>& GetVipList() { return Vip_Uid_List; }
	std::set<std::string>& GetVipLOGList() { return Vip_LOG_List; }

	std::map<General::PlayCategory , int> GetChannelDistribution() { return m_channelDistribution; }



	inline const std::vector<uint64>& GetListArrayValues(const Server::BaselineConfigKey& defaultType) {
		static const std::vector<uint64> empty;
		auto iter = m_ListArraySettings.find(defaultType);
		if ( iter == m_ListArraySettings.end() ) {
			return empty;
		}
		return iter->second;
	}










	






	// 캐스케이딩 릴 심볼 갯수당 보상 금액


	// 클러스터 릴 심볼 갯수당 보상 금액






#pragma region 해적릴













	// 심볼 갯수당 보상 금액








#pragma endregion 해적릴

	inline General::AvatarProfile GetAvatar( const int& avatar_id ) {

		General::AvatarProfile _avatar = General::AvatarProfile::default_instance();

		auto iter = m_avatar_map.find( avatar_id );
		if ( iter != m_avatar_map.end() ) {
			_avatar = iter->second;
		}
		return _avatar;
	}

	inline std::vector<General::AvatarProfile> GetAvatars() { return m_avatars; }
	inline std::vector<General::AvatarProfile> GetFreeAvatars() { return m_free_avatars; }

	inline std::vector<Server::Level> GetLevelList() { return m_Levels; }

					// 스캐터 갯수에 따른 프리스핀 카운트 목록
			// 프리스핀중에 터진 스캐터 갯수에 따른 프리스핀 보너스 갯수 목록
 // 그랜드 프리스핀중에 터진 스캐터 갯수에 따른 프리스핀 보너스 갯수 목록

					// 클러스터 릴 스캐터 갯수에 따른 프리스핀 카운트 목록
				// 클러스터 릴 프리스핀중에 터진 스캐터 갯수에 따른 프리스핀 보너스 갯수 목록
	// 클러스터 릴 그랜드 프리스핀중에 터진 스캐터 갯수에 따른 프리스핀 보너스 갯수 목록

					// 스캐터 갯수에 따른 프리스핀 카운트 목록
			// 프리스핀중에 터진 스캐터 갯수에 따른 프리스핀 보너스 갯수 목록
// 그랜드 프리스핀중에 터진 스캐터 갯수에 따른 프리스핀 보너스 갯수 목록

	inline std::map<uint32 , General::TaskProgress> GetQuests() { return m_quests; }
	inline std::map<uint32 , uint64> GetQuestRewards() { return m_quest_reward_map; }
	inline std::map<int32 , General::TaskProgress> GetDailyMissons() { return m_daily_missions; }
	inline std::map<int32 , General::TaskProgress> GetLoungeMissions() { return m_lounge_missions; }
	inline std::map<int32 , General::TaskProgress> GetAchievements() { return m_achievements; }
	inline std::map<int32 , std::vector<General::TaskProgress>> GetGroupAchievements() { return m_group_achievements; }

	inline std::vector<Server::LotteryData> GetLotteryDatas() { return m_lottery_datas; }

	inline std::map<std::string , Server::ShopProduct> GetItemProducts() { return m_item_products; }
	inline std::map<std::string , Server::ShopProduct> GetGoogleStoreProducts() { return m_google_store_products; }
	inline std::map<std::string , Server::ShopProduct> GetGoogleStoreProductsById() { return m_google_store_products_by_id; }
	inline std::map<std::string , Server::ShopProduct> GetAppleStoreProducts() { return m_apple_store_products; }
	inline std::map<std::string , Server::ShopProduct> GetOneStoreStoreProducts() { return m_onetstore_store_products; }

	inline const Server::ShopProduct& GetAttendanceReward( const int days ) const {
		static const Server::ShopProduct empty;
		if ( days < 1 || days > static_cast< int >( m_attendance_rewards.size() ) ) return empty;
		return m_attendance_rewards[ days - 1 ];
	}

	inline int32 TotalAttendanceDays() { return static_cast< int32 >( m_attendance_rewards.size() ); }

	inline uint64 GetDefaultValue(const Server::BaselineConfigKey& defaultType) {
		auto iter = m_basicSettings.find(defaultType);
		if ( iter == m_basicSettings.end() ) {
			return 0;
		}
		return iter->second;
	}

public:
	static bool LoadingEnd;
	static bool SystemLoadingEnd;

	int LowBadukiPlayerCount;
	int	LowBadukiWait;
	int LowBadukiTurnMinimumWait;
	int LowBadukiCardChangeDelay;
	int RecentlyHistoryCount;
	int LowBadukiBetWait;
	int	CardChangeWait;
	int	BaseDistributionCardWaitTwo;
	int	BaseDistributionCardWaitThree;
	int	BaseDistributionCardWaitFour;	
	int BaseDistributionCardWaitFive;
	int	PlayerTurnResWait;
	int	PlayResultWaitGiveUp;
	int	PlayResultWaitTwo;
	int	PlayResultWaitThree;
	int	PlayResultWaitFour;
	int	PlayResultWaitFive;





public:
	General::InboxState GetMailState() { return m_mail_state; }
	void SetMailState( General::InboxState mail_state ) { m_mail_state = mail_state; }

};

