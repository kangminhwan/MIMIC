#include "cClientSession.h"
#pragma execution_character_set("utf-8")

#include "../Include/Netlib/Common/cInterfaceIocpContext.h"
//#include "../Include/Netlib/Queue/cCommandQueue.h"
//#include "../Include/Netlib/Queue/cLogQueue.h"
//#include "../Include/Netlib/Session/cSession.h"
#include "../Include/Netlib/IOCP/cIocpContext.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
//#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

#include "cAssetLog.h"
#include "Query.h"

#include <future>

cAssetLog::~cAssetLog()
{
	AfterLogs();

	// 머니 로그 저장
	WriteLog();
}

void cAssetLog::BeforeLogs()
{
	if ( m_pClientSession == nullptr ) return;

	m_before_player.CopyFrom( m_pClientSession->GetPlayerRef() );
}

void cAssetLog::AfterLogs()
{
	if ( m_pClientSession == nullptr ) return;


}

void cAssetLog::WriteLog()
{
	if ( m_pClientSession == nullptr ) return;

	const General::ParticipantProfile&  player = m_pClientSession->GetPlayer();

	auto context = m_pClientSession->GetContext();
	std::string ipaddr = m_pClientSession->GetIp(); //context != nullptr ? ::ConvertIP( context->GetIP() ) : "";

	int64 get_gem = player.wallet_gems() >= m_before_player.wallet_gems() ? player.wallet_gems() - m_before_player.wallet_gems() : 0; // 획득한 무료 다이아
	int64 get_coin = player.wallet_coins() >= m_before_player.wallet_coins() ? player.wallet_coins() - m_before_player.wallet_coins() : 0; // 획득한 코인
	int64 get_chips = player.wallet_chips() >= m_before_player.wallet_chips() ? player.wallet_chips() - m_before_player.wallet_chips() : 0; // 획득한 칩
	int64 use_coin = m_before_player.wallet_coins() >= player.wallet_coins() ? 0 : m_before_player.wallet_coins() - player.wallet_coins(); // 사용한 코인
	int get_kickoutticket = player.kick_ticket_balance() >= m_before_player.kick_ticket_balance() ? player.kick_ticket_balance() - m_before_player.kick_ticket_balance() : 0; // 획득한 강제 퇴장권

	switch ( m_code )
	{
	case 30101: // 캐시상품구매
	{
		std::string product_id = "";

		switch ( m_pClientSession->GetMarket() )
		{
		case General::StoreChannel::StoreChannel_GooglePlay:
			product_id = m_shop_product.google_product_id();
		break;
		case General::StoreChannel::StoreChannel_AppleAppStore:
			product_id = m_shop_product.apple_product_id();
		break;
		case General::StoreChannel::StoreChannel_OneStore:
			product_id = m_shop_product.onestore_product_id();
		break;
		}

		std::string membershipclass;
		switch ( player.membership_tier() )
		{
		case General::BenefitTier::BenefitTier_Basic:
			membershipclass = "Normal";
			break;
		case General::BenefitTier::BenefitTier_Standard:
			membershipclass = "Standard";
			break;
		case General::BenefitTier::BenefitTier_Premium:
			membershipclass = "Premium";
			break;
		default:
			membershipclass = "None";
			break;
		}

		std::future<BOOL> insertAssetLogResult = QueryManager::InsertAssetLog(
			m_code , // int code ,                  // 로그 코드
			m_pClientSession->GetPlatformGuid() , // const std::string & uid ,               // UID
			"" , // const std::string & cmd ,               // 시퀀스
			ipaddr , // IP 정보
			"" , // const std::string & pf ,                // 스토어 플랫폼(2)
			"" , // const std::string & device ,            // 디바이스 모델 정보
			"" , // const std::string & idpcode ,           // 로그인 플랫폼
			"" , // const std::string & osver ,             // 디바이스 OS버전
			player.paid_gems() - m_before_player.paid_gems() , // int64_t a_pgem ,                           // 획득한 유료 다이아
			0 , // int64_t a_fgem ,                           // 획득한 무료 다이아
			0 , // int64_t u_gem ,                            // 사용한 다이아 총 수량
			0 , // int64_t u_fgem ,                           // 사용한 무료 다이아 수량
			0 , // int64_t u_pgem ,                           // 사용한 유료 다이아 수량
			player.wallet_gems() + player.paid_gems() , // int64_t gem ,                              // 보유 다이아 (유료+무료)
			player.wallet_gems() , // int64_t gem_f ,                            // 보유 무료 다이아
			player.paid_gems() , // int64_t gem_p ,                            // 보유 유료 다이아
			get_chips , // int64_t a_chip ,                       // 획득한 칩
			player.wallet_chips() + player.vault_chips() , // int64_t chip ,                         // 보유 칩 (소지+금고)
			player.wallet_chips() , // int64_t chip_g ,                       // 소지한 칩
			player.vault_chips() , // int64_t chip_s ,                       // 금고 보유 칩
			0 , // int64_t a_coin ,                       // 획득한 코인 수량
			0 , // int64_t u_coin ,                           // 사용한 코인 수량
			0 , // int64_t coin ,                         // 보유 코인 (소지+금고)
			0 , // int64_t coin_g ,                       // 소지한 코인
			0 , // int64_t coin_s ,                       // 금고 보유 코인
			0 , // int kickoutticket ,                    // 보유 라운지 강제 퇴장권 수량
			0 , // int a_kickoutticket ,                  // 획득한 라운지 강제 퇴장권 수량
			0 , // int64_t discard_chip ,                 // 초과 칩 삭제
			0 , // int64_t discard_coin ,                 // 초과 코인 삭제
			0 , /*필수 */ // int price ,                            // 구매한 상품의 가격
			0 , // int refill_amount ,                    // 리필 금액
			0 , // int refill_today ,                     // 당일 리필 횟수
			"" , /*필수 */ // const std::string & reward ,            // 설정된 보상
			"" , /*필수 */ // const std::string & rwd_avatar ,        // 획득한 아바타
			 membershipclass , /*필수 */ // const std::string & rwd_membership ,    // 획득한 멤버십
			0 , // int amount ,                           // 이동 수량
			"" , // const std::string & move ,              // 이동 경로
			0 , // int rakeback ,                         // 레이크백 통장 남은 수량
			product_id , /*필수 */ // const std::string & rwd_data ,          // 보상 획득 데이터(사유)
			m_productDetail , /*필수 */ // const std::string & rwd_list ,          // 획득한 보상 리스트
			product_id , /*필수 */ // const std::string & rwd_type ,          // 보상 타입
			m_pClientSession->GetGameVersion() , // const std::string & ver ,               // 접속한 게임 버전
			m_product_orderID); // const std::string & etc);              // 기타

		insertAssetLogResult.wait();
	}
	break;
	case 30102: // 다이아로 상품 구매
	{
		std::string product_id = m_shop_product.id();

		std::ostringstream osAvataData;
		if ( m_shop_product.avatar_id() > 0 )
		{
			osAvataData << m_shop_product.avatar_id() << "*" << m_shop_product.avatar_add_days();
		}

		std::future<BOOL> insertAssetLogResult = QueryManager::InsertAssetLog(
			m_code , // int code ,                  // 로그 코드
			m_pClientSession->GetPlatformGuid() , // const std::string & uid ,               // UID
			"" , // const std::string & cmd ,               // 시퀀스
			ipaddr , // IP 정보
			"" , // const std::string & pf ,                // 스토어 플랫폼(2)
			"" , // const std::string & device ,            // 디바이스 모델 정보
			"" , // const std::string & idpcode ,           // 로그인 플랫폼
			"" , // const std::string & osver ,             // 디바이스 OS버전
			0 , // int64_t a_pgem ,                           // 획득한 유료 다이아
			0 , // int64_t a_fgem ,                           // 획득한 무료 다이아
			m_before_player.paid_gems() - player.paid_gems() + m_before_player.wallet_gems() - player.wallet_gems() , // int64_t u_gem ,                            // 사용한 다이아 총 수량
			m_before_player.wallet_gems() - player.wallet_gems() , // int64_t u_fgem ,                           // 사용한 무료 다이아 수량
			m_before_player.paid_gems() - player.paid_gems() , // int64_t u_pgem ,                           // 사용한 유료 다이아 수량
			player.wallet_gems() + player.paid_gems() , // int64_t gem ,                              // 보유 다이아 (유료+무료)
			player.wallet_gems() , // int64_t gem_f ,                            // 보유 무료 다이아
			player.paid_gems() , // int64_t gem_p ,                            // 보유 유료 다이아
			get_chips , // int64_t a_chip ,                       // 획득한 칩
			player.wallet_chips() + player.vault_chips() , // int64_t chip ,                         // 보유 칩 (소지+금고)
			player.wallet_chips() , // int64_t chip_g ,                       // 소지한 칩
			player.vault_chips() , // int64_t chip_s ,                       // 금고 보유 칩
			0 , // int64_t a_coin ,                       // 획득한 코인 수량
			0 , // int64_t u_coin ,                           // 사용한 코인 수량
			0 , // int64_t coin ,                         // 보유 코인 (소지+금고)
			0 , // int64_t coin_g ,                       // 소지한 코인
			0 , // int64_t coin_s ,                       // 금고 보유 코인
			player.kick_ticket_balance() , // int kickoutticket ,                    // 보유 라운지 강제 퇴장권 수량
			get_kickoutticket , // int a_kickoutticket ,                  // 획득한 라운지 강제 퇴장권 수량
			0 , // int64_t discard_chip ,                 // 초과 칩 삭제
			0 , // int64_t discard_coin ,                 // 초과 코인 삭제
			m_shop_product.money_value() , /*필수 */ // int price ,                            // 구매한 상품의 가격
			0 , // int refill_amount ,                    // 리필 금액
			0 , // int refill_today ,                     // 당일 리필 횟수
			"" , /*필수 */ // const std::string & reward ,            // 설정된 보상
			osAvataData.str() , /*필수 */ // const std::string & rwd_avatar ,        // 획득한 아바타
			"" , /*필수 */ // const std::string & rwd_membership ,    // 획득한 멤버십
			0 , // int amount ,                           // 이동 수량
			"" , // const std::string & move ,              // 이동 경로
			0 , // int rakeback ,                         // 레이크백 통장 남은 수량
			m_shop_product.id() , /*필수 */ // const std::string & rwd_data ,          // 보상 획득 데이터(사유)
			m_productDetail , /*필수 */ // const std::string & rwd_list ,          // 획득한 보상 리스트
			m_shop_product.id() , /*필수 */ // const std::string & rwd_type ,          // 보상 타입
			m_pClientSession->GetGameVersion() , // const std::string & ver ,               // 접속한 게임 버전
			m_productDetail ); // const std::string & etc);              // 기타


		insertAssetLogResult.wait();
	}
	break;
	case 30103: // 미션 보상 획득
	{
		std::future<BOOL> insertAssetLogResult = QueryManager::InsertAssetLog(
			m_code , // int code ,                  // 로그 코드
			m_pClientSession->GetPlatformGuid() , // const std::string & uid ,               // UID
			"" , // const std::string & cmd ,               // 시퀀스
			ipaddr , // IP 정보
			"" , // const std::string & pf ,                // 스토어 플랫폼(2)
			"" , // const std::string & device ,            // 디바이스 모델 정보
			"" , // const std::string & idpcode ,           // 로그인 플랫폼
			"" , // const std::string & osver ,             // 디바이스 OS버전
			0 , // int64_t a_pgem ,                           // 획득한 유료 다이아
			0 , // int64_t a_fgem ,                           // 획득한 무료 다이아
			0 , // int64_t u_gem ,                            // 사용한 다이아 총 수량
			0 , // int64_t u_fgem ,                           // 사용한 무료 다이아 수량
			0 , // int64_t u_pgem ,                           // 사용한 유료 다이아 수량
			0 , // int64_t gem ,                              // 보유 다이아 (유료+무료)
			0 , // int64_t gem_f ,                            // 보유 무료 다이아
			0 , // int64_t gem_p ,                            // 보유 유료 다이아
			get_chips , // int64_t a_chip ,                       // 획득한 칩
			player.wallet_chips() + player.vault_chips() , // int64_t chip ,                         // 보유 칩 (소지+금고)
			player.wallet_chips() , // int64_t chip_g ,                       // 소지한 칩
			player.vault_chips() , // int64_t chip_s ,                       // 금고 보유 칩
			get_coin , // int64_t a_coin ,                       // 획득한 코인 수량
			0 , // int64_t u_coin ,                           // 사용한 코인 수량
			player.wallet_coins() + player.vault_coins() , // int64_t coin ,                         // 보유 코인 (소지+금고)
			player.wallet_coins() , // int64_t coin_g ,                       // 소지한 코인
			player.vault_coins() , // int64_t coin_s ,                       // 금고 보유 코인
			0 , // int kickoutticket ,                    // 보유 라운지 강제 퇴장권 수량
			0 , // int a_kickoutticket ,                  // 획득한 라운지 강제 퇴장권 수량
			0 , // int64_t discard_chip ,                 // 초과 칩 삭제
			0 , // int64_t discard_coin ,                 // 초과 코인 삭제
			0 , /*필수 */ // int price ,                            // 구매한 상품의 가격
			0 , // int refill_amount ,                    // 리필 금액
			0 , // int refill_today ,                     // 당일 리필 횟수
			"" , /*필수 */ // const std::string & reward ,            // 설정된 보상
			"" , /*필수 */ // const std::string & rwd_avatar ,        // 획득한 아바타
			"" , /*필수 */ // const std::string & rwd_membership ,    // 획득한 멤버십
			0 , // int amount ,                           // 이동 수량
			"" , // const std::string & move ,              // 이동 경로
			0 , // int rakeback ,                         // 레이크백 통장 남은 수량
			m_quest_id , /*필수 */ // const std::string & rwd_data ,          // 보상 획득 데이터(사유)
			m_rwd_list , /*필수 */ // const std::string & rwd_list ,          // 획득한 보상 리스트
			m_reward_type , /*필수 */ // const std::string & rwd_type ,          // 보상 타입
			m_pClientSession->GetGameVersion() , // const std::string & ver ,               // 접속한 게임 버전
			"" ); // const std::string & etc);              // 기타

		insertAssetLogResult.wait();
	}
	break;
	case 30104: // 업적 보상 획득
	{
		std::future<BOOL> insertAssetLogResult = QueryManager::InsertAssetLog(
			m_code , // int code ,                  // 로그 코드
			m_pClientSession->GetPlatformGuid() , // const std::string & uid ,               // UID
			"" , // const std::string & cmd ,               // 시퀀스
			ipaddr , // IP 정보
			"" , // const std::string & pf ,                // 스토어 플랫폼(2)
			"" , // const std::string & device ,            // 디바이스 모델 정보
			"" , // const std::string & idpcode ,           // 로그인 플랫폼
			"" , // const std::string & osver ,             // 디바이스 OS버전
			0 , // int64_t a_pgem ,                           // 획득한 유료 다이아
			0 , // int64_t a_fgem ,                           // 획득한 무료 다이아
			0 , // int64_t u_gem ,                            // 사용한 다이아 총 수량
			0 , // int64_t u_fgem ,                           // 사용한 무료 다이아 수량
			0 , // int64_t u_pgem ,                           // 사용한 유료 다이아 수량
			0 , // int64_t gem ,                              // 보유 다이아 (유료+무료)
			0 , // int64_t gem_f ,                            // 보유 무료 다이아
			0 , // int64_t gem_p ,                            // 보유 유료 다이아
			get_chips , // int64_t a_chip ,                       // 획득한 칩
			player.wallet_chips() + player.vault_chips() , // int64_t chip ,                         // 보유 칩 (소지+금고)
			player.wallet_chips() , // int64_t chip_g ,                       // 소지한 칩
			player.vault_chips() , // int64_t chip_s ,                       // 금고 보유 칩
			get_coin , // int64_t a_coin ,                       // 획득한 코인 수량
			0 , // int64_t u_coin ,                           // 사용한 코인 수량
			player.wallet_coins() + player.vault_coins() , // int64_t coin ,                         // 보유 코인 (소지+금고)
			player.wallet_coins() , // int64_t coin_g ,                       // 소지한 코인
			player.vault_coins() , // int64_t coin_s ,                       // 금고 보유 코인
			0 , // int kickoutticket ,                    // 보유 라운지 강제 퇴장권 수량
			0 , // int a_kickoutticket ,                  // 획득한 라운지 강제 퇴장권 수량
			0 , // int64_t discard_chip ,                 // 초과 칩 삭제
			0 , // int64_t discard_coin ,                 // 초과 코인 삭제
			0 , /*필수 */ // int price ,                            // 구매한 상품의 가격
			0 , // int refill_amount ,                    // 리필 금액
			0 , // int refill_today ,                     // 당일 리필 횟수
			"" , /*필수 */ // const std::string & reward ,            // 설정된 보상
			"" , /*필수 */ // const std::string & rwd_avatar ,        // 획득한 아바타
			"" , /*필수 */ // const std::string & rwd_membership ,    // 획득한 멤버십
			0 , // int amount ,                           // 이동 수량
			"" , // const std::string & move ,              // 이동 경로
			0 , // int rakeback ,                         // 레이크백 통장 남은 수량
			m_quest_id , /*필수 */ // const std::string & rwd_data ,          // 보상 획득 데이터(사유)
			m_rwd_list , /*필수 */ // const std::string & rwd_list ,          // 획득한 보상 리스트
			m_reward_type , /*필수 */ // const std::string & rwd_type ,          // 보상 타입
			m_pClientSession->GetGameVersion() , // const std::string & ver ,               // 접속한 게임 버전
			"" ); // const std::string & etc);              // 기타

		insertAssetLogResult.wait();
	}
	break;
	case 30105: // 내가방-보관함 메시지 획득
	{
		std::ostringstream ossMailID;
		std::ostringstream ossAvataData;
		std::ostringstream ossItem;
		std::ostringstream ossType;
		std::ostringstream ossReward;

		ossMailID << m_mailIndex;

		int64_t chip = 0;
		int64_t coin = 0;
		int64_t gem = 0;
		int64_t paidgem = 0;

		switch ( m_mailInfo.bounty_kind() )
		{
		case General::GrantItemKind::GrantItem_FreeCoin:
		{
			ossType << "Coin";
			coin = m_mailInfo.cnt();
			ossReward << "Coin:";
			ossReward << coin;
			get_coin = coin;
		}
		break;
		case General::GrantItemKind::GrantItem_FreeChip:
		{
			ossType << "Chip";
			chip = m_mailInfo.cnt();
			ossReward << "Chip:";
			ossReward << chip;
			get_chips = chip;
		}
		break;
		case General::GrantItemKind::GrantItem_Avatar:
		{
			ossType << "Avata";
			const int& avatar_id = m_mailInfo.item_no();
			int period = m_mailInfo.span() == 0 ? 7 : m_mailInfo.span();

			ossAvataData << m_mailInfo.item_no();
			ossAvataData << "*";
			ossAvataData << period;

			ossReward << "Avata:";
			ossReward << ossAvataData.str();

		}
		break;
		case General::GrantItemKind::GrantItem_InventoryItem:
			break;
		case General::GrantItemKind::GrantItem_KickTicket:
		{
			ossType << "KickoutTicket";
			int64 ticketcount = m_mailInfo.cnt();

			ossReward << "KickoutTicket" << "*" << ticketcount;
		}
		break;
		case General::GrantItemKind::GrantItem_FreeGem:
		{
			ossType << "GEM";
			gem = m_mailInfo.cnt();
			ossReward << "GEM:";
			ossReward << gem;
			get_gem = gem;
		}
		break;
		case General::GrantItemKind::GrantItem_PaidGem:
		{
			ossType << "PaidGEM";
			paidgem = m_mailInfo.cnt();
			ossReward << "PaidGEM:";
			ossReward << paidgem;
		}
		break;
		}

		std::ostringstream ossMailType;
		switch ( m_mailInfo.inbox_kind() )
		{
		case General::InboxReason::InboxReason_Attendance:
			ossMailType << "Attendance";
			break;
		case General::InboxReason::InboxReason_LunchPush:
			ossMailType << "Lunch_Push";
			break;
		case General::InboxReason::InboxReason_DinnerPush:
			ossMailType << "Dinner_Push";
			break;
		case General::InboxReason::InboxReason_ReturnGift:
			ossMailType << "Return_Gift";
			break;
		case General::InboxReason::InboxReason_GoodsPurchase:
			ossMailType << "BuyGoods";
			break;
		case General::InboxReason::InboxReason_PushReward:
			ossMailType << "Push";
			break;
		case General::InboxReason::InboxReason_BalanceLimit:
			ossMailType << "MoneyLimit";
			break;
		case General::InboxReason::InboxReason_EventReward:
			ossMailType << "Event";
			break;
		case General::InboxReason::InboxReason_MaintenanceReward:
			ossMailType << "Maintenance";
			break;
		case General::InboxReason::InboxReason_Purchase:
			ossMailType << "Buy";
			break;
		case General::InboxReason::InboxReason_PurchaseReward:
			ossMailType << "BuyReward";
			break;
		case General::InboxReason::InboxReason_LevelUpReward:
			ossMailType << "LevelUp";
			break;
		case General::InboxReason::InboxReason_AdminTool:
			ossMailType << "Lunch_Push";
			break;
		}

		std::future<BOOL> insertAssetLogResult = QueryManager::InsertAssetLog(
			m_code , // int code ,                  // 로그 코드
			m_pClientSession->GetPlatformGuid() , // const std::string & uid ,               // UID
			"" , // const std::string & cmd ,               // 시퀀스
			ipaddr , // IP 정보
			"" , // const std::string & pf ,                // 스토어 플랫폼(2)
			"" , // const std::string & device ,            // 디바이스 모델 정보
			"" , // const std::string & idpcode ,           // 로그인 플랫폼
			"" , // const std::string & osver ,             // 디바이스 OS버전
			paidgem , // int64_t a_pgem ,                           // 획득한 유료 다이아
			get_gem , // int64_t a_fgem ,                           // 획득한 무료 다이아
			0 , // int64_t u_gem ,                            // 사용한 다이아 총 수량
			0 , // int64_t u_fgem ,                           // 사용한 무료 다이아 수량
			0 , // int64_t u_pgem ,                           // 사용한 유료 다이아 수량
			player.wallet_gems() + player.paid_gems() , // int64_t gem ,                              // 보유 다이아 (유료+무료)
			player.wallet_gems() , // int64_t gem_f ,                            // 보유 무료 다이아
			player.paid_gems() , // int64_t gem_p ,                            // 보유 유료 다이아
			get_chips , // int64_t a_chip ,                       // 획득한 칩
			player.wallet_chips() + player.vault_chips() , // int64_t chip ,                         // 보유 칩 (소지+금고)
			player.wallet_chips() , // int64_t chip_g ,                       // 소지한 칩
			player.vault_chips() , // int64_t chip_s ,                       // 금고 보유 칩
			get_coin , // int64_t a_coin ,                       // 획득한 코인 수량
			use_coin , // int64_t u_coin ,                           // 사용한 코인 수량
			player.wallet_coins() + player.vault_coins() , // int64_t coin ,                         // 보유 코인 (소지+금고)
			player.wallet_coins() , // int64_t coin_g ,                       // 소지한 코인
			player.vault_coins() , // int64_t coin_s ,                       // 금고 보유 코인
			0 , // int kickoutticket ,                    // 보유 라운지 강제 퇴장권 수량
			0 , // int a_kickoutticket ,                  // 획득한 라운지 강제 퇴장권 수량
			0 , // int64_t discard_chip ,                 // 초과 칩 삭제
			0 , // int64_t discard_coin ,                 // 초과 코인 삭제
			0 , /*필수 */ // int price ,                            // 구매한 상품의 가격
			0 , // int refill_amount ,                    // 리필 금액
			0 , // int refill_today ,                     // 당일 리필 횟수
			ossReward.str() , /*필수 */ // const std::string & reward ,            // 설정된 보상
			ossAvataData.str() , /*필수 */ // const std::string & rwd_avatar ,        // 획득한 아바타
			"" , /*필수 */ // const std::string & rwd_membership ,    // 획득한 멤버십
			0 , // int amount ,                           // 이동 수량
			"" , // const std::string & move ,              // 이동 경로
			0 , // int rakeback ,                         // 레이크백 통장 남은 수량
			"" , /*필수 */ // const std::string & rwd_data ,          // 보상 획득 데이터(사유)
			ossReward.str() , /*필수 */ // const std::string & rwd_list ,          // 획득한 보상 리스트
			ossMailType.str() , /*필수 */ // const std::string & rwd_type ,          // 보상 타입
			m_pClientSession->GetGameVersion() , // const std::string & ver ,               // 접속한 게임 버전
			ossMailID.str() ); // const std::string & etc);              // 기타

		insertAssetLogResult.wait();
	}
	break;
	case 30106: // 복권 보상
	{
		string rwd_List = "";
		switch ( m_money_type )
		{
		case General::AssetKind::AssetKind_Coin:
		{
			m_reward_type = "CoinTicket";
			rwd_List = std::format( "Coin: {}" , m_charge_amount );
		}
		break;
		case General::AssetKind::AssetKind_Chip:
		{
			m_reward_type = "ChipTicket";
			rwd_List = std::format( "Chip: {}" , m_charge_amount );
		}
		break;
		}

		std::future<BOOL> insertAssetLogResult = QueryManager::InsertAssetLog(
			m_code , // int code ,                  // 로그 코드
			m_pClientSession->GetPlatformGuid() , // const std::string & uid ,               // UID
			"" , // const std::string & cmd ,               // 시퀀스
			ipaddr , // IP 정보
			"" , // const std::string & pf ,                // 스토어 플랫폼(2)
			"" , // const std::string & device ,            // 디바이스 모델 정보
			"" , // const std::string & idpcode ,           // 로그인 플랫폼
			"" , // const std::string & osver ,             // 디바이스 OS버전
			0 , // int64_t a_pgem ,                           // 획득한 유료 다이아
			0 , // int64_t a_fgem ,                           // 획득한 무료 다이아
			0 , // int64_t u_gem ,                            // 사용한 다이아 총 수량
			0 , // int64_t u_fgem ,                           // 사용한 무료 다이아 수량
			0 , // int64_t u_pgem ,                           // 사용한 유료 다이아 수량
			0 , // int64_t gem ,                              // 보유 다이아 (유료+무료)
			0, // int64_t gem_f ,                            // 보유 무료 다이아
			0 , // int64_t gem_p ,                            // 보유 유료 다이아
			get_chips , // int64_t a_chip ,                       // 획득한 칩
			player.wallet_chips() + player.vault_chips() , // int64_t chip ,                         // 보유 칩 (소지+금고)
			player.wallet_chips() , // int64_t chip_g ,                       // 소지한 칩
			player.vault_chips() , // int64_t chip_s ,                       // 금고 보유 칩
			get_coin , // int64_t a_coin ,                       // 획득한 코인 수량
			0 , // int64_t u_coin ,                           // 사용한 코인 수량
			player.wallet_coins() + player.vault_coins() , // int64_t coin ,                         // 보유 코인 (소지+금고)
			player.wallet_coins() , // int64_t coin_g ,                       // 소지한 코인
			player.vault_coins() , // int64_t coin_s ,                       // 금고 보유 코인
			0 , // int kickoutticket ,                    // 보유 라운지 강제 퇴장권 수량
			0 , // int a_kickoutticket ,                  // 획득한 라운지 강제 퇴장권 수량
			0 , // int64_t discard_chip ,                 // 초과 칩 삭제
			0 , // int64_t discard_coin ,                 // 초과 코인 삭제
			0 , /*필수 */ // int price ,                            // 구매한 상품의 가격
			0 , // int refill_amount ,                    // 리필 금액
			0 , // int refill_today ,                     // 당일 리필 횟수
			"" , /*필수 */ // const std::string & reward ,            // 설정된 보상
			"" , /*필수 */ // const std::string & rwd_avatar ,        // 획득한 아바타
			"" , /*필수 */ // const std::string & rwd_membership ,    // 획득한 멤버십
			0 , // int amount ,                           // 이동 수량
			"" , // const std::string & move ,              // 이동 경로
			0 , // int rakeback ,                         // 레이크백 통장 남은 수량
			"" , /*필수 */ // const std::string & rwd_data ,          // 보상 획득 데이터(사유)
			rwd_List , /*필수 */ // const std::string & rwd_list ,          // 획득한 보상 리스트  Coin : 0 / Chip : 0
			m_reward_type , /*필수 */ // const std::string & rwd_type ,          // 보상 타입
			m_pClientSession->GetGameVersion() , // const std::string & ver ,               // 접속한 게임 버전
			"" ); // const std::string & etc);              // 기타

		insertAssetLogResult.wait();
	}
	break;
	case 30201: // 칩 리필 보상
	{
		int platform_code;
		std::future<BOOL> platform_code_result = QueryManager::GetPlayerPlatformAsync( m_pClientSession->GetAccountGuid() , m_pClientSession->GetPlatformGuid() , platform_code );
		platform_code_result.wait();

		if ( FALSE == platform_code_result.get() ) {
		}
		string platformString = General::AccessChannelType_Name( static_cast< General::AccessChannelType > ( platform_code ) );

		auto valueDescriptor = General::StoreChannel_descriptor()->FindValueByNumber( m_pClientSession->GetMarket() );
		std::string marketString = valueDescriptor->name();

		std::string etc = std::to_string( m_all_refill ) + " / " + std::to_string( m_use_refill );

		std::future<BOOL> insertAssetLogResult = QueryManager::InsertAssetLog(
			m_code , // int code ,                  // 로그 코드
			m_pClientSession->GetPlatformGuid() , // const std::string & uid ,               // UID
			"" , // const std::string & cmd ,               // 시퀀스
			ipaddr , // IP 정보
			marketString , // const std::string & pf ,                // 스토어 플랫폼(2)
			m_pClientSession->GetDeviceInfo() , // const std::string & device ,            // 디바이스 모델 정보
			platformString , // const std::string & idpcode ,           // 로그인 플랫폼
			"" , // const std::string & osver ,             // 디바이스 OS버전
			0 , // int64_t a_pgem ,                           // 획득한 유료 다이아
			0 , // int64_t a_fgem ,                           // 획득한 무료 다이아
			0 , // int64_t u_gem ,                            // 사용한 다이아 총 수량
			0 , // int64_t u_fgem ,                           // 사용한 무료 다이아 수량
			0 , // int64_t u_pgem ,                           // 사용한 유료 다이아 수량
			0 , // int64_t gem ,                              // 보유 다이아 (유료+무료)
			0 , // int64_t gem_f ,                            // 보유 무료 다이아
			0 , // int64_t gem_p ,                            // 보유 유료 다이아
			0 , // int64_t a_chip ,                       // 획득한 칩
			player.wallet_chips() + player.vault_chips() , // int64_t chip ,                         // 보유 칩 (소지+금고)
			player.wallet_chips() , // int64_t chip_g ,                       // 소지한 칩
			player.vault_chips() , // int64_t chip_s ,                       // 금고 보유 칩
			0 , // int64_t a_coin ,                       // 획득한 코인 수량
			0 , // int64_t u_coin ,                           // 사용한 코인 수량
			0 , // int64_t coin ,                         // 보유 코인 (소지+금고)
			0 , // int64_t coin_g ,                       // 소지한 코인
			0 , // int64_t coin_s ,                       // 금고 보유 코인
			0 , // int kickoutticket ,                    // 보유 라운지 강제 퇴장권 수량
			0 , // int a_kickoutticket ,                  // 획득한 라운지 강제 퇴장권 수량
			0 , // int64_t discard_chip ,                 // 초과 칩 삭제
			0 , // int64_t discard_coin ,                 // 초과 코인 삭제
			0 , /*필수 */ // int price ,                            // 구매한 상품의 가격
			m_refill_amount , // int refill_amount ,                    // 리필 금액
			m_refill_today , // int refill_today ,                     // 당일 리필 횟수
			"" , /*필수 */ // const std::string & reward ,            // 설정된 보상
			"" , /*필수 */ // const std::string & rwd_avatar ,        // 획득한 아바타
			"" , /*필수 */ // const std::string & rwd_membership ,    // 획득한 멤버십
			0 , // int amount ,                           // 이동 수량
			"" , // const std::string & move ,              // 이동 경로
			0 , // int rakeback ,                         // 레이크백 통장 남은 수량
			"" , /*필수 */ // const std::string & rwd_data ,          // 보상 획득 데이터(사유)
			"" , /*필수 */ // const std::string & rwd_list ,          // 획득한 보상 리스트
			"" , /*필수 */ // const std::string & rwd_type ,          // 보상 타입
			m_pClientSession->GetGameVersion() , // const std::string & ver ,               // 접속한 게임 버전
			etc ); // const std::string & etc);              // 기타

		insertAssetLogResult.wait();
	}
	break;
	case 30202: // 코인 리필 보상
	{
		// playerPlatform
		int platform_code;
		std::future<BOOL> platform_code_result = QueryManager::GetPlayerPlatformAsync( m_pClientSession->GetAccountGuid() , m_pClientSession->GetPlatformGuid() , platform_code );
		platform_code_result.wait();

		if ( FALSE == platform_code_result.get() ) {
		}
		string platformString = General::AccessChannelType_Name( static_cast< General::AccessChannelType > ( platform_code ) );

		// 마켓
		auto valueDescriptor = General::StoreChannel_descriptor()->FindValueByNumber( m_pClientSession->GetMarket() );
		std::string marketString = valueDescriptor->name();

		std::string etc = std::to_string( m_all_refill ) + " / " + std::to_string( m_use_refill );

		std::future<BOOL> insertAssetLogResult = QueryManager::InsertAssetLog(
			m_code , // int code ,                  // 로그 코드
			m_pClientSession->GetPlatformGuid() , // const std::string & uid ,               // UID
			"" , // const std::string & cmd ,               // 시퀀스
			ipaddr , // IP 정보
			marketString , // const std::string & pf ,                // 스토어 플랫폼(2)
			m_pClientSession->GetDeviceInfo() , // const std::string & device ,            // 디바이스 모델 정보
			platformString , // const std::string & idpcode ,           // 로그인 플랫폼
			"" , // const std::string & osver ,             // 디바이스 OS버전
			0 , // int64_t a_pgem ,                           // 획득한 유료 다이아
			0 , // int64_t a_fgem ,                           // 획득한 무료 다이아
			0 , // int64_t u_gem ,                            // 사용한 다이아 총 수량
			0 , // int64_t u_fgem ,                           // 사용한 무료 다이아 수량
			0 , // int64_t u_pgem ,                           // 사용한 유료 다이아 수량
			0 , // int64_t gem ,                              // 보유 다이아 (유료+무료)
			0 , // int64_t gem_f ,                            // 보유 무료 다이아
			0 , // int64_t gem_p ,                            // 보유 유료 다이아
			0 , // int64_t a_chip ,                       // 획득한 칩
			0 , // int64_t chip ,                         // 보유 칩 (소지+금고)
			0 , // int64_t chip_g ,                       // 소지한 칩
			0 , // int64_t chip_s ,                       // 금고 보유 칩
			0 , // int64_t a_coin ,                       // 획득한 코인 수량
			0 , // int64_t u_coin ,                           // 사용한 코인 수량
			player.wallet_coins() + player.vault_coins() , // int64_t coin ,                         // 보유 코인 (소지+금고)
			player.wallet_coins() , // int64_t coin_g ,                       // 소지한 코인
			player.vault_coins() , // int64_t coin_s ,                       // 금고 보유 코인
			0 , // int kickoutticket ,                    // 보유 라운지 강제 퇴장권 수량
			0 , // int a_kickoutticket ,                  // 획득한 라운지 강제 퇴장권 수량
			0 , // int64_t discard_chip ,                 // 초과 칩 삭제
			0 , // int64_t discard_coin ,                 // 초과 코인 삭제
			0 , /*필수 */ // int price ,                            // 구매한 상품의 가격
			m_refill_amount , // int refill_amount ,                    // 리필 금액
			m_refill_today , // int refill_today ,                     // 당일 리필 횟수
			"" , /*필수 */ // const std::string & reward ,            // 설정된 보상
			"" , /*필수 */ // const std::string & rwd_avatar ,        // 획득한 아바타
			"" , /*필수 */ // const std::string & rwd_membership ,    // 획득한 멤버십
			0 , // int amount ,                           // 이동 수량
			"" , // const std::string & move ,              // 이동 경로
			0 , // int rakeback ,                         // 레이크백 통장 남은 수량
			"" , /*필수 */ // const std::string & rwd_data ,          // 보상 획득 데이터(사유)
			"" , /*필수 */ // const std::string & rwd_list ,          // 획득한 보상 리스트
			"" , /*필수 */ // const std::string & rwd_type ,          // 보상 타입
			m_pClientSession->GetGameVersion() , // const std::string & ver ,               // 접속한 게임 버전
			etc ); // const std::string & etc);              // 기타

		insertAssetLogResult.wait();
	}
	break;
	case 30301: // 강퇴권 관련 로그
	{
		// 전후 비교해서 사용인지 취소인지 처리
		std::string etc_string;
		if ( m_before_player.kick_ticket_balance() >= player.kick_ticket_balance() )
			etc_string = "use kick_ticket_count";
		else
			etc_string = "cancel kick_ticket_count";

		std::future<BOOL> insertAssetLogResult = QueryManager::InsertAssetLog(
			m_code , // int code ,                  // 로그 코드
			m_pClientSession->GetPlatformGuid() , // const std::string & uid ,               // UID
			"" , // const std::string & cmd ,               // 시퀀스
			ipaddr , // IP 정보
			"" , // const std::string & pf ,                // 스토어 플랫폼(2)
			"" , // const std::string & device ,            // 디바이스 모델 정보
			"" , // const std::string & idpcode ,           // 로그인 플랫폼
			"" , // const std::string & osver ,             // 디바이스 OS버전
			0 , // int64_t a_pgem ,                           // 획득한 유료 다이아
			0 , // int64_t a_fgem ,                           // 획득한 무료 다이아
			0 , // int64_t u_gem ,                            // 사용한 다이아 총 수량
			0 , // int64_t u_fgem ,                           // 사용한 무료 다이아 수량
			0 , // int64_t u_pgem ,                           // 사용한 유료 다이아 수량
			0 , // int64_t gem ,                              // 보유 다이아 (유료+무료)
			0 , // int64_t gem_f ,                            // 보유 무료 다이아
			0 , // int64_t gem_p ,                            // 보유 유료 다이아
			0 , // int64_t a_chip ,                       // 획득한 칩
			0 , // int64_t chip ,                         // 보유 칩 (소지+금고)
			0 , // int64_t chip_g ,                       // 소지한 칩
			0 , // int64_t chip_s ,                       // 금고 보유 칩
			0 , // int64_t a_coin ,                       // 획득한 코인 수량
			0 , // int64_t u_coin ,                           // 사용한 코인 수량
			0 , // int64_t coin ,                         // 보유 코인 (소지+금고)
			0 , // int64_t coin_g ,                       // 소지한 코인
			0 , // int64_t coin_s ,                       // 금고 보유 코인
			player.kick_ticket_balance() , // int kickoutticket ,                    // 보유 라운지 강제 퇴장권 수량
			0 , // int a_kickoutticket ,                  // 획득한 라운지 강제 퇴장권 수량
			0 , // int64_t discard_chip ,                 // 초과 칩 삭제
			0 , // int64_t discard_coin ,                 // 초과 코인 삭제
			0 , // int price ,                            // 구매한 상품의 가격
			0 , // int refill_amount ,                    // 리필 금액
			0 , // int refill_today ,                     // 당일 리필 횟수
			"" , /*필수 */ // const std::string & reward ,            // 설정된 보상
			"" , /*필수 */ // const std::string & rwd_avatar ,        // 획득한 아바타
			"" , /*필수 */ // const std::string & rwd_membership ,    // 획득한 멤버십
			0 , // int amount ,                           // 이동 수량
			"" , // const std::string & move ,              // 이동 경로
			0 , // int rakeback ,                         // 레이크백 통장 남은 수량
			"" , /*필수 */ // const std::string & rwd_data ,          // 보상 획득 데이터(사유)
			"" , // const std::string & rwd_list ,          // 획득한 보상 리스트
			"" , // const std::string & rwd_type ,          // 보상 타입
			m_pClientSession->GetGameVersion() , // const std::string & ver ,               // 접속한 게임 버전
			etc_string ); // const std::string & etc);              // 기타

		insertAssetLogResult.wait();
	}
	break;
	case 30901: // 금고 이동
	{
		std::string move_string;
		int64 amount = 0;
		switch ( m_safebox_logtype )
		{
		case Server::SafeBoxLogType::SafeBoxLogType_Deposit_Chip: // 칩 보관
		{
			move_string = "입금 칩";
			amount = player.vault_chips() - m_before_player.vault_chips();
		}
		break;
		case Server::SafeBoxLogType::SafeBoxLogType_Withdraw_Chip: // 칩 출금
		{
			move_string = "출금 칩";
			amount = m_before_player.vault_chips() - player.vault_chips();
		}
		break;
		case Server::SafeBoxLogType::SafeBoxLogType_Deposit_Coin: // 코인 보관
		{
			move_string = "입금 코인";
			amount = player.vault_coins() - m_before_player.vault_coins();
		}
		break;
		case Server::SafeBoxLogType::SafeBoxLogType_Withdraw_Coin: // 코인 출금
		{
			move_string = "출금 코인";
			amount = m_before_player.vault_coins() - player.vault_coins();
		}
		break;
		}

		std::string product_id = m_shop_product.id();

		std::future<BOOL> insertAssetLogResult = QueryManager::InsertAssetLog(
			m_code , // int code ,                  // 로그 코드
			m_pClientSession->GetPlatformGuid() , // const std::string & uid ,               // UID
			"" , // const std::string & cmd ,               // 시퀀스
			ipaddr , // IP 정보
			"" , // const std::string & pf ,                // 스토어 플랫폼(2)
			"" , // const std::string & device ,            // 디바이스 모델 정보
			"" , // const std::string & idpcode ,           // 로그인 플랫폼
			"" , // const std::string & osver ,             // 디바이스 OS버전
			0 , // int64_t a_pgem ,                           // 획득한 유료 다이아
			0 , // int64_t a_fgem ,                           // 획득한 무료 다이아
			0 , // int64_t u_gem ,                            // 사용한 다이아 총 수량
			0 , // int64_t u_fgem ,                           // 사용한 무료 다이아 수량
			0 , // int64_t u_pgem ,                           // 사용한 유료 다이아 수량
			0 , // int64_t gem ,                              // 보유 다이아 (유료+무료)
			0 , // int64_t gem_f ,                            // 보유 무료 다이아
			0 , // int64_t gem_p ,                            // 보유 유료 다이아
			0 , // int64_t a_chip ,                       // 획득한 칩
			0 , // int64_t chip ,                         // 보유 칩 (소지+금고)
			player.wallet_chips() , // int64_t chip_g ,                       // 소지한 칩
			player.vault_chips() , // int64_t chip_s ,                       // 금고 보유 칩
			0 , // int64_t a_coin ,                       // 획득한 코인 수량
			0 , // int64_t u_coin ,                           // 사용한 코인 수량
			0 , // int64_t coin ,                         // 보유 코인 (소지+금고)
			player.wallet_coins() , // int64_t coin_g ,                       // 소지한 코인
			player.vault_coins() , // int64_t coin_s ,                       // 금고 보유 코인
			0 , // int kickoutticket ,                    // 보유 라운지 강제 퇴장권 수량
			0 , // int a_kickoutticket ,                  // 획득한 라운지 강제 퇴장권 수량
			0 , // int64_t discard_chip ,                 // 초과 칩 삭제
			0 , // int64_t discard_coin ,                 // 초과 코인 삭제
			0 , // int price ,                            // 구매한 상품의 가격
			0 , // int refill_amount ,                    // 리필 금액
			0 , // int refill_today ,                     // 당일 리필 횟수
			"" , // const std::string & reward ,            // 설정된 보상
			"" , // const std::string & rwd_avatar ,        // 획득한 아바타
			"" , // const std::string & rwd_membership ,    // 획득한 멤버십
			amount , // int amount ,                           // 이동 수량
			move_string, // const std::string & move ,              // 이동 경로
			//view_string.data() , // const std::string & move ,              // 이동 경로
			0 , // int rakeback ,                         // 레이크백 통장 남은 수량
			"" , // const std::string & rwd_data ,          // 보상 획득 데이터(사유)
			"" , // const std::string & rwd_list ,          // 획득한 보상 리스트
			"" , // const std::string & rwd_type ,          // 보상 타입
			m_pClientSession->GetGameVersion() , // const std::string & ver ,               // 접속한 게임 버전
			"" ); // const std::string & etc);              // 기타

		insertAssetLogResult.wait();
	}
	break;
	case 30911: // 레이크백 출금
	{
		std::string move_string = "출금 코인";
		int64 amount = m_before_player.rakeback_balance() - player.rakeback_balance();
		std::future<BOOL> insertAssetLogResult = QueryManager::InsertAssetLog(
			m_code , // int code ,                  // 로그 코드
			m_pClientSession->GetPlatformGuid() , // const std::string & uid ,               // UID
			"" , // const std::string & cmd ,               // 시퀀스
			ipaddr , // IP 정보
			"" , // const std::string & pf ,                // 스토어 플랫폼(2)
			"" , // const std::string & device ,            // 디바이스 모델 정보
			"" , // const std::string & idpcode ,           // 로그인 플랫폼
			"" , // const std::string & osver ,             // 디바이스 OS버전
			0 , // int64_t a_pgem ,                           // 획득한 유료 다이아
			0 , // int64_t a_fgem ,                           // 획득한 무료 다이아
			0 , // int64_t u_gem ,                            // 사용한 다이아 총 수량
			0 , // int64_t u_fgem ,                           // 사용한 무료 다이아 수량
			0 , // int64_t u_pgem ,                           // 사용한 유료 다이아 수량
			0 , // int64_t gem ,                              // 보유 다이아 (유료+무료)
			0 , // int64_t gem_f ,                            // 보유 무료 다이아
			0 , // int64_t gem_p ,                            // 보유 유료 다이아
			0 , // int64_t a_chip ,                       // 획득한 칩
			0 , // int64_t chip ,                         // 보유 칩 (소지+금고)
			0 , // int64_t chip_g ,                       // 소지한 칩
			0 , // int64_t chip_s ,                       // 금고 보유 칩
			0 , // int64_t a_coin ,                       // 획득한 코인 수량
			0 , // int64_t u_coin ,                           // 사용한 코인 수량
			player.wallet_coins() + player.vault_coins() , // int64_t coin ,                         // 보유 코인 (소지+금고)
			player.wallet_coins() , // int64_t coin_g ,                       // 소지한 코인
			player.vault_coins() , // int64_t coin_s ,                       // 금고 보유 코인
			0 , // int kickoutticket ,                    // 보유 라운지 강제 퇴장권 수량
			0 , // int a_kickoutticket ,                  // 획득한 라운지 강제 퇴장권 수량
			0 , // int64_t discard_chip ,                 // 초과 칩 삭제
			0 , // int64_t discard_coin ,                 // 초과 코인 삭제
			0 , // int price ,                            // 구매한 상품의 가격
			0 , // int refill_amount ,                    // 리필 금액
			0 , // int refill_today ,                     // 당일 리필 횟수
			"" , // const std::string & reward ,            // 설정된 보상
			"" , // const std::string & rwd_avatar ,        // 획득한 아바타
			"" , // const std::string & rwd_membership ,    // 획득한 멤버십
			amount , // int amount ,                           // 이동 수량
			move_string , // const std::string & move ,              // 이동 경로
			player.rakeback_balance(), // int rakeback ,                         // 레이크백 통장 남은 수량
			"" , // const std::string & rwd_data ,          // 보상 획득 데이터(사유)
			"" , // const std::string & rwd_list ,          // 획득한 보상 리스트
			"" , // const std::string & rwd_type ,          // 보상 타입
			m_pClientSession->GetGameVersion() , // const std::string & ver ,               // 접속한 게임 버전
			"" ); // const std::string & etc);              // 기타

		insertAssetLogResult.wait();
	}
	break;
	}
}