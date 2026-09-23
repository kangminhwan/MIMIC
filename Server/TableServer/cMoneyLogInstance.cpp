#include "cClientSession.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
//#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

#include "cMoneyLogInstance.h"

#include "Query.h"

#include <future>

cMoneyLogInstance::~cMoneyLogInstance()
{
	AfterLogs();

	// 머니 로그 저장
	WriteMoneyLog();
}

void cMoneyLogInstance::BeforeLogs()
{
	if ( m_pClientSession == nullptr ) return;

	m_chip_before = m_pClientSession->GetChip();

	m_coin_before = m_pClientSession->GetCoin();

	m_gem_before = m_pClientSession->GetGem();;

	m_paid_gem_before = m_pClientSession->GetPaidGem();
}

void cMoneyLogInstance::AfterLogs()
{
	if ( m_pClientSession == nullptr ) return;

	m_chip_after = m_pClientSession->GetChip();

	m_coin_after = m_pClientSession->GetCoin();

	m_gem_after = m_pClientSession->GetGem();

	m_paid_gem_after = m_pClientSession->GetPaidGem();
}

void cMoneyLogInstance::WriteMoneyLog()
{
	if ( m_pClientSession == nullptr ) return;

	// 재화 별로 저장한다.

	if ( m_chip_before != m_chip_after ) {

		int64 getMoney = 0;
		if ( m_chip_after > m_chip_before ) {
			getMoney = static_cast< int64 >( m_chip_after - m_chip_before );
		}
		else {
			getMoney = -static_cast< int64 >( m_chip_before - m_chip_after );
		}

		// 머니 로그
		std::string target_identifier; // 로우바둑이, 홀덤으로 재화 소진 또는 획득일 경우 대상 UID 기록\r\n* 상점에서의 획득 또는 소진일 경우 구매한 상품 ID 기록\r\n* 미션, 업적이벤트, 업적, 우편함 일경우 해당 log code  기록 (event_type과 동일)\r\n(소진일 경우 승자 UID 1개 출력/획득 일 경우 패배 UID 최대 8개 기록)',
		auto result = QueryManager::MoneyLogInsert(
		( int ) m_money_event_type ,
		( int ) General::AssetKind::AssetKind_Chip ,
		m_pClientSession->GetAccountGuid() ,
		m_pClientSession->GetPlatformGuid() ,
		m_pClientSession->GetNickName() ,
		m_pClientSession->GetIp() ,
		getMoney > 0 ? getMoney : 0 ,
		getMoney < 0 ? getMoney : 0 ,
		( int ) General::PlayCategory::PlayCategory_None ,
		( int ) General::SlotCatalog::PM_SLOT_None ,
		target_identifier );

		result.wait();

		if ( FALSE == result.get() ) {

			// 로그 저장 실패
		}
	}

	if ( m_coin_before != m_coin_after ) {

		int64 getMoney = 0;
		if ( m_coin_after > m_coin_before ) {
			getMoney = static_cast< int64 >( m_coin_after - m_coin_before );
		}
		else {
			getMoney = -static_cast< int64 >( m_coin_before - m_coin_after );
		}

		// 머니 로그
		std::string target_identifier; // 로우바둑이, 홀덤으로 재화 소진 또는 획득일 경우 대상 UID 기록\r\n* 상점에서의 획득 또는 소진일 경우 구매한 상품 ID 기록\r\n* 미션, 업적이벤트, 업적, 우편함 일경우 해당 log code  기록 (event_type과 동일)\r\n(소진일 경우 승자 UID 1개 출력/획득 일 경우 패배 UID 최대 8개 기록)',
		auto result = QueryManager::MoneyLogInsert(
		( int ) m_money_event_type ,
		( int ) General::AssetKind::AssetKind_Coin ,
		m_pClientSession->GetAccountGuid() ,
		m_pClientSession->GetPlatformGuid() ,
		m_pClientSession->GetNickName() ,
		m_pClientSession->GetIp() ,
		getMoney > 0 ? getMoney : 0 ,
		getMoney < 0 ? getMoney : 0 ,
		( int ) General::PlayCategory::PlayCategory_None ,
		( int ) General::SlotCatalog::PM_SLOT_None ,
		target_identifier );

		result.wait();

		if ( FALSE == result.get() ) {

			// 로그 저장 실패
		}
	}

	if ( m_gem_before != m_gem_after ) {

		int64 getMoney = 0;
		if ( m_gem_after > m_gem_before ) {
			getMoney = static_cast< int64 >( m_gem_after - m_gem_before );
		}
		else {
			getMoney = -static_cast< int64 >( m_gem_before - m_gem_after );
		}

		// 머니 로그
		std::string target_identifier; // 로우바둑이, 홀덤으로 재화 소진 또는 획득일 경우 대상 UID 기록\r\n* 상점에서의 획득 또는 소진일 경우 구매한 상품 ID 기록\r\n* 미션, 업적이벤트, 업적, 우편함 일경우 해당 log code  기록 (event_type과 동일)\r\n(소진일 경우 승자 UID 1개 출력/획득 일 경우 패배 UID 최대 8개 기록)',
		auto result = QueryManager::MoneyLogInsert(
		( int ) m_money_event_type ,
		( int ) General::AssetKind::AssetKind_Gem ,
		m_pClientSession->GetAccountGuid() ,
		m_pClientSession->GetPlatformGuid() ,
		m_pClientSession->GetNickName() ,
		m_pClientSession->GetIp() ,
		getMoney > 0 ? getMoney : 0 ,
		getMoney < 0 ? getMoney : 0 ,
		( int ) General::PlayCategory::PlayCategory_None ,
		( int ) General::SlotCatalog::PM_SLOT_None ,
		target_identifier );

		result.wait();

		if ( FALSE == result.get() ) {

			// 로그 저장 실패
		}
	}


}