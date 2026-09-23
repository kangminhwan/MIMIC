#pragma once
#include "TableServerHeader.h"

#include "cClientSession.h"

class cAssetLog
{
private:
	cClientSession* m_pClientSession;

	General::ParticipantProfile m_before_player;

	int m_code; // 애셋 코드 번호

	Server::ProductData m_shop_productData;
	Server::ShopProduct m_shop_product;
	std::string m_productDetail;

	Server::SafeBoxLogType m_safebox_logtype;

	std::string m_quest_id;
	std::string m_reward_type;

	int m_refill_amount;
	int m_refill_today;
	int m_all_refill;
	int m_use_refill;

	std::string m_mail_idx_list;
	std::string m_rwd_list;

	std::string m_charge_amount;

	int64_t m_mailIndex;
	PmNet::InboxDetail m_mailInfo;

	std::string m_product_orderID;

	General::AssetKind m_money_type;

private:
	cAssetLog() { m_pClientSession = nullptr; }

	void BeforeLogs();
	void AfterLogs();
	void WriteLog();

public:
	cAssetLog( cClientSession* pClientSession , const int& code ) {
		this->m_pClientSession = pClientSession;
		m_code = code;
		BeforeLogs();
	}
	~cAssetLog();

	void SetShopProduct( const Server::ShopProduct& shop_product , const std::string& productDetail ) {

		m_shop_product = shop_product;
		m_productDetail = productDetail;
	}

	void SetShopProductData( const Server::ProductData& shop_productData ) {
		m_shop_productData = shop_productData;
	}

	void SetSafeBoxLogType( const Server::SafeBoxLogType& safebox_logtype ) {

		m_safebox_logtype = safebox_logtype;
	}

	void SetRewardQuest(const std::string& quest_id, const std::string& reward_type , const std::string reward_list ) {
		m_quest_id = quest_id;
		m_reward_type = reward_type;
		m_rwd_list = reward_list;
	}

	void SetRefill(const int& refill_amount, const int& refill_today, const int& all_refill, const int& use_refill) {
		m_refill_amount = refill_amount;
		m_refill_today = refill_today;
		m_all_refill = all_refill;
		m_use_refill = use_refill;
	}

	void SetMailOpen(const std::string& mail_idx_list, const std::string& rwd_list){
		m_mail_idx_list = mail_idx_list;
		m_rwd_list = rwd_list;
	}

	void SetMailInfo(int index, PmNet::InboxDetail mailInfo )
	{
		m_mailIndex = index;
		m_mailInfo = mailInfo;
	}

	void SetChargeAmount(const General::AssetKind& money_type, const std::string& charge_amount){
		m_charge_amount = charge_amount;
		m_money_type = money_type;
	}

	void SetProductOrderID( const std::string& orderID )
	{
		m_product_orderID = orderID;
	}
};
//