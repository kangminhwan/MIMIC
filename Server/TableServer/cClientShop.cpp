#include "cClientSession.h"
//#include "cVirtualSession.h"
//#include "../Include/Netlib/UdpModule/cUDPSession.h"
//#include "../Include/Netlib/IOCP/cIocpContext.h"
//#include "../Include/Netlib/Common/cSingleton.h"
//#include "../Include/Netlib/Queue/cLogQueue.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

//#include "cGameRoom.h"
//#include "cGameRoomManager.h"
//#include "cProtoUtil.h"
//#include "cDataLoader.h"
#include "TimeUtils.h"

#include "Query.h"

#include <iostream>
#include <format>
#include <future>
#include <sstream>

/*
message ShopProduct
{
	string id = 1;										// id
	General.AssetKind buy_money_type = 2;				// 구매 재화 타입
	uint64 money_value = 3;								// 구매 금액

	// 상품을 전체 때려 박는다. 현재는 아바타와, 칩뿐이 없다.
	int32 item_id = 6;									// 아이템 타입, 아바타 타입 에 따라 참조가 달라지니 주의
	int32 item_count = 7;								// 수량
	int32 item_add_days = 8;							// 아바타 expire time 시간 증가 ( 일수 )
	uint64 paid_chips = 9;								// 유료 칩 수량
	uint64 paid_coin = 10;								// 유료 코인 수량 ( 현재는 없음 )
	int32 avatar_id = 11;								// 구매 아바타 아이디
	int32 avatar_add_days = 12;							// 아바타의 착용 시간 증가 ( 일수 )
	uint32 paid_gem = 13;								// 유료 다이아
}

*/

BOOL cClientSession::PlayerUpdateAsync()
{
	std::future<BOOL> result = QueryManager::PlayerUpdateByQuery( m_player , m_playerExt );
	result.wait();
	return result.get();
}

// 구매가 가능한지 체크 한다.
General::ResultCode cClientSession::CheckCondition( const Server::ShopProduct& shopProduct )
{
	// 멤버쉽 구매
	if ( shopProduct.member_ship_class() != General::BenefitTier::BenefitTier_None ) {

		if ( GetMemberShipClass() == General::BenefitTier::BenefitTier_Basic )
			return General::ResultCode::Result_Success;

		// 기간이 지났다면 구매 가능하다.
		if ( MemberShipExpired() )
			return General::ResultCode::Result_Success;

		// 현재 멤버쉽이 기간이 남아 있다면 다른 멤버쉽 구매가 불가능하다.
		if ( GetMemberShipClass() != shopProduct.member_ship_class() )
			return General::ResultCode::Result_MembershipStillActive;

	}
    // Chip limit check
    if ( shopProduct.paid_chips() > 0 ) {
        uint64 MAX_CHIP_LIMIT = 0;
        if ( shopProduct.member_ship_class() != General::BenefitTier::BenefitTier_None )
            MAX_CHIP_LIMIT = GetMaxHoldingChip( shopProduct.member_ship_class() );
        else
            MAX_CHIP_LIMIT = GetMaxHoldingChip();

        uint64 currentChip = GetChip();
        uint64 newChipAmount = currentChip + shopProduct.paid_chips();

        // If total chip amount exceeds limit after purchase, block purchase
        if ( newChipAmount > MAX_CHIP_LIMIT ) {
            return General::ResultCode::Result_ChipCapacityReached;
        }
    }

	return General::ResultCode::Result_Success;
}

// 보상은 유료 재화로 전부 처리 한다.
// 상점 구매 상품은 바로 지급한다.
// Server::ProductData 은 구매내역 로그를 위해 리턴한다.
Server::ProductData cClientSession::BuyShop( const Server::ShopProduct& shopProduct , bool& updateMoney , bool& updatePlayer , bool& updateAvatar , bool& updateMailBox , std::string& productDetail )
{
    Server::ProductData _product_data;

    updateMoney = false;
    updateAvatar = false;
    updatePlayer = false;
    updateMailBox = false;

    const uint64& playerIdx = GetPlayerIdx();

    std::ostringstream logStream;
    logStream << "Purchase Details: ";

    // ITEM 상품 지급, KickOut Ticket 뿐이 없다.
    if ( shopProduct.item_id() != 0 ) {

        AddKickOutTicket( shopProduct.item_count() );
        std::future<BOOL> result = QueryManager::PlayerUpdateByQuery( m_player , m_playerExt );
        result.wait();

        if ( FALSE == result.get() ) {
            // 플레이어 클래스 갱신 실패
        }
        logStream << "\n- KickOut Ticket: " << shopProduct.item_count();
    }

    // 유료 칩 구매
    if ( shopProduct.paid_chips() ) {

        // 클래스 구매로 획득하는 칩은 우편한 지급
        if ( shopProduct.member_ship_class() != General::BenefitTier::BenefitTier_None ) {
            std::string getLimitTimeString = TimeUtils::GetCurrentKSTDateTimeString( 7 * 24 );
            std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateMailBoxInsertQuery( playerIdx , General::GrantItemKind::GrantItem_FreeChip , General::InboxReason::InboxReason_PurchaseReward , shopProduct.paid_chips() , 0 , "",0 , getLimitTimeString ) );
            result.wait();

            if ( FALSE == result.get() ) {
                // Failed to send mail
            }

            auto valueDescriptor = General::BenefitTier_descriptor()->FindValueByNumber( shopProduct.member_ship_class() );
            std::string memberShipClassString = valueDescriptor->name();

            logStream << "\n- MemberShipClass: " << memberShipClassString;
        }
        else {

            uint64 currentPaidChip = this->GetChip();
            this->SetChip( General::PlayCategory::PlayCategory_None , currentPaidChip + shopProduct.paid_chips() );
            updateMoney = true;
            updatePlayer = true;
            _product_data.set_chip( shopProduct.paid_chips() );

            logStream << "\n- Paid Chips: " << shopProduct.paid_chips();
        }
    }

    // 유료 코인 구매
    if ( shopProduct.paid_coin() ) {

        uint64 currentPaidCoin = this->GetCoin();
        this->SetCoin( General::PlayCategory::PlayCategory_None , currentPaidCoin + shopProduct.paid_coin() );
        updateMoney = true;
        updatePlayer = true;
        _product_data.set_coin( shopProduct.paid_coin() );

        logStream << "\n- Paid Coin: " << shopProduct.paid_coin();
    }

    // 유료 다이아 구매
    if ( shopProduct.paid_gem() ) {

        uint64 currentPaidGem = this->GetPaidGem();
        this->SetPaidGem( currentPaidGem + shopProduct.paid_gem() );
        updateMoney = true;
        updatePlayer = true;
        _product_data.set_gem( shopProduct.paid_gem() );

        logStream << "\n- Paid Gem: " << shopProduct.paid_gem();
    }

    // 강퇴권 구매
    if ( shopProduct.kick_out_ticket_count() ) {

        uint32 cur_ticket_cnt = this->GetKickOutTicketCount();
        this->SetKickOutTicketCount( cur_ticket_cnt + shopProduct.kick_out_ticket_count() );
        std::future<BOOL> result = QueryManager::PlayerUpdateByQuery( m_player , m_playerExt );
        result.wait();

        if ( FALSE == result.get() ) {
            // 강퇴권 갱신 실패
        }
        else {
            updatePlayer = true;
        }
        _product_data.set_kickout_ticket( shopProduct.kick_out_ticket_count() );

        logStream << "\n- KickOutTicket Count: " << shopProduct.kick_out_ticket_count();
    }

    // 아바타 구매
    if ( shopProduct.avatar_id() ) {

        if ( hasAvatar( shopProduct.avatar_id() ) ) {

            auto iter = m_avatars.find( shopProduct.avatar_id() );
            auto& avatar = iter->second;

            std::string expiry_string = avatar.expires_at();
            std::time_t expiry_time = TimeUtils::StringToTimeTM( expiry_string );
            std::time_t now = std::time( nullptr );

            // 재화 지급
            if ( expiry_time > now ) {

                // 기간 연장
                std::time_t new_expiry_time = TimeUtils::AddDays( expiry_time , shopProduct.avatar_add_days() );
                std::string _new = TimeUtils::TMToString( new_expiry_time );
                avatar.set_expires_at( _new );

                if ( FALSE == QueryManager::PlayerUpdateAvatar( GetPlayerIdx() , shopProduct.avatar_id() , _new ) ) {
                    // 아바타 갱신 실패 로그 출력
                }
                else {
                    updateAvatar = true;
                    _product_data.set_avatar_id( shopProduct.avatar_id() );
                }
            }
            else {

                // 오늘 부터 ~ 기간 처리
                std::time_t new_expiry_time = TimeUtils::AddDays( now , shopProduct.avatar_add_days() );
                std::string _new = TimeUtils::TMToString( new_expiry_time );
                avatar.set_expires_at( _new );

                if ( FALSE == QueryManager::PlayerUpdateAvatar( GetPlayerIdx() , shopProduct.avatar_id() , _new ) ) {
                    // 아바타 갱신 실패 로그 출력
                }
                else {
                    updateAvatar = true;
                    _product_data.set_avatar_id( shopProduct.avatar_id() );
                }
            }
        }
        else {
            // 없으면 안되는데???
        }
        logStream << "\n- Avatar ID: " << shopProduct.avatar_id();
    }

    // 멤버쉽 구매
    if ( shopProduct.member_ship_class() != General::BenefitTier::BenefitTier_None ) {

        std::string expiry_string = m_player.membership_expires_at();
        std::time_t expiry_time = TimeUtils::StringToTimeTM( expiry_string );
        std::time_t now = std::time( nullptr );

        switch ( GetMemberShipClass() )
        {
        case General::BenefitTier::BenefitTier_Basic:
        {
            // 멤버쉽 변경
            m_player.set_membership_enabled( true );
            m_player.set_membership_tier( shopProduct.member_ship_class() );

            // 오늘 부터 ~ 기간 처리
            std::time_t new_expiry_time = TimeUtils::AddDays( now , shopProduct.membership_add_days() );
            std::string _new = TimeUtils::TMToString( new_expiry_time );
            m_player.set_membership_expires_at( _new );

            std::future<BOOL> result = QueryManager::PlayerUpdateByQuery( m_player , m_playerExt );
            result.wait();

            if ( FALSE == result.get() ) {
                // 플레이어 클래스 갱신 실패
            }
            else {
                updatePlayer = true;
                _product_data.set_member_ship_class( shopProduct.member_ship_class() );
            }

            // 칩 보상 지급
            // 레귤러 130억
            // 탕ㅂ 240 억
            uint64 reward_money = 0;
            if ( shopProduct.member_ship_class() == General::BenefitTier::BenefitTier_Standard ) {
                reward_money = 13000000000;
            }
            else if ( shopProduct.member_ship_class() == General::BenefitTier::BenefitTier_Premium ) {
                reward_money = 24000000000;
            }

            if ( reward_money > 0 ) {
                std::string getLimitTimeString = TimeUtils::GetCurrentKSTDateTimeString( 24 * 30 );
                std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateMailBoxInsertQuery( playerIdx , General::GrantItemKind::GrantItem_FreeChip , General::InboxReason::InboxReason_PurchaseReward , reward_money , 0 , "" , 0 , getLimitTimeString ) );
                result.wait();

                if ( FALSE == result.get() ) {
                    // Failed to send mail
                }
                else {
                    updateMailBox = true;
                    _product_data.set_chip( reward_money );
                }
            }
        }
        break;
        case General::BenefitTier::BenefitTier_Standard:
        case General::BenefitTier::BenefitTier_Premium:
        {
            if ( shopProduct.member_ship_class() == GetMemberShipClass() ) {

                if ( expiry_time > now ) {

                    // 기간 연장
                    std::time_t new_expiry_time = TimeUtils::AddDays( expiry_time , shopProduct.membership_add_days() );
                    std::string _new = TimeUtils::TMToString( new_expiry_time );
                    m_player.set_membership_expires_at( _new );

                    std::future<BOOL> result = QueryManager::PlayerUpdateByQuery( m_player , m_playerExt );
                    result.wait();

                    if ( FALSE == result.get() ) {
                        // 플레이어 클래스 갱신 실패
                    }
                    else {
                        updatePlayer = true;
                        _product_data.set_member_ship_class( shopProduct.member_ship_class() );
                    }
                }
                else {

                    // 오늘 부터 ~ 기간 처리
                    std::time_t new_expiry_time = TimeUtils::AddDays( now , shopProduct.membership_add_days() );
                    std::string _new = TimeUtils::TMToString( new_expiry_time );
                    m_player.set_membership_expires_at( _new );

                    std::future<BOOL> result = QueryManager::PlayerUpdateByQuery( m_player , m_playerExt );
                    result.wait();

                    if ( FALSE == result.get() ) {
                        // 플레이어 클래스 갱신 실패
                    }
                    else {
                        updatePlayer = true;
                        _product_data.set_member_ship_class( shopProduct.member_ship_class() );
                    }
                }
            }
            else {
                // 멤버쉽이 있는데, 다른 멤버쉽 구매 => 구매 처리 못함
            }

            // 칩 보상 지급
            // 레귤러 130억
            // 탕ㅂ 240 억
            uint64 reward_money = 0;
            if ( shopProduct.member_ship_class() == General::BenefitTier::BenefitTier_Standard ) {
                reward_money = 13000000000;
            }
            else if ( shopProduct.member_ship_class() == General::BenefitTier::BenefitTier_Premium ) {
                reward_money = 24000000000;
            }

            if ( reward_money > 0 ) {

                std::string getLimitTimeString = TimeUtils::GetCurrentKSTDateTimeString( 7 * 30 );
                std::future<BOOL> result = QueryManager::PlayerExecuteQueryAsync( E_DB_TYPE::E_DB_TYPE_SHARD , QueryManager::GenerateMailBoxInsertQuery( playerIdx , General::GrantItemKind::GrantItem_FreeChip , General::InboxReason::InboxReason_PurchaseReward , reward_money , 0 , "" , 0 , getLimitTimeString ) );
                result.wait();

                if ( FALSE == result.get() ) {
                    // Failed to send mail
                }
                else {
                    updateMailBox = true;
                    _product_data.set_chip( reward_money );
                }
            }
        }
        break;
        }

        auto valueDescriptor = General::BenefitTier_descriptor()->FindValueByNumber( shopProduct.member_ship_class() );
        std::string memberShipClassString = valueDescriptor->name();

        logStream << "\n- Membership Class: " << memberShipClassString;
    }

    if ( updateMoney ) {

        if ( FALSE == QueryManager::PlayerMoneyUpdate( GetPlayerIdx() , GetCoin() , GetChip() , GetRakeBack() , GetGem() , 0 , 0 , GetPaidGem() ) ) {
            // 재화 갱신 실패 로그 출력
        }
    }

    // 최종 구매 내역
    productDetail = logStream.str();

    return _product_data;
}
