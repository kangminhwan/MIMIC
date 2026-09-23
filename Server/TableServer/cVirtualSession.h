#pragma once
#include "TableServerHeader.h"
#include "cClientSession.h"

#include "../Include/Netlib/Session/cSession.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

#include <atlcoll.h>
#include <thread>
#include <future>

//class cVirtualSession : public NetLib::cSession , public cBlackjackCard
class cVirtualSession : public cClientSession
{

	friend class cClientSession;

private:
	cClientSession* m_owner;
	uint64 m_playerIdx;

public:
	inline uint64 GetChip() override { return m_owner->GetChip(); }
	inline void SetChip( General::PlayCategory gameType , uint64 chip ) override { m_owner->SetChip( gameType , chip ); }
	inline bool MinusChip( uint64 chip , bool checkOnly ) override { return m_owner->MinusChip( chip , checkOnly ); }
	inline uint64 GetLostMoney() override { return m_owner->GetLostMoney(); }
	inline uint64 GetVirtualLostMoney() override { return m_owner->GetVirtualLostMoney(); }
	inline void PlusVirtualLostMoney( const uint64 money ) override { m_owner->PlusVirtualLostMoney( money ); }
	inline void InitLostMoney() override {}
	inline void InitVirtualLostMoney() override {}
	inline uint64 GetCoin() override { return m_owner->GetCoin(); }
	inline void SetCoin( General::PlayCategory gameType , uint64 coin ) override { m_owner->SetCoin( gameType , coin ); }
	//inline void SetCoin( uint64 coin ) override {}
	inline bool MinusCoin( uint64 coin , bool checkOnly ) override { return m_owner->MinusCoin( coin , checkOnly ); }

	uint64 GetOwnerPlayerIdx() override { return m_owner->GetPlayerIdx(); }
	bool IsOwnerConnected() override { return m_owner != nullptr && m_owner->GetContext() != nullptr; }

public:
	bool SendRequest( const UINT nCommand , google::protobuf::Message& _message , General::ResultCode errorCode , std::string& errorMessage ) override {
		if ( m_owner != nullptr )
			return m_owner->SendRequest( nCommand , _message , errorCode , errorMessage );
		return false;
	}

public:
	void SessionLogout( UINT Entity , BOOL bForce = 0 ) override {}

public:
	cVirtualSession( cClientSession* owner , const uint64& playerIdx ) {
		m_owner = owner;
		m_playerIdx = playerIdx;

		m_player.set_member_id( playerIdx );
		
		m_player.set_owner_member_id( owner->GetPlayerIdx() );
		m_player.set_blackjack_sub_player( true );

	}

private:
	cVirtualSession();
};