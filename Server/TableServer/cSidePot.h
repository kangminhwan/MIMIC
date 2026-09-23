#pragma once

#include "../../ProtocolBuffer/cpp/General.pb.h"

#include "TableServerHeader.h"

class cClientSession;
class cSidePot
{
public:
	uint64 m_beforePotBaseAmount;				// 이전 사이드 팟의 금액
	uint64 m_baseAmount;						// 이 사이드 팟의 금액
	//uint64 m_perSideMoney;					// 이 사이드 팟의 기준 금액
	uint64 m_sideMoneyTotal;					// 이 사이드팟에 누적된 총 금액
	uint64 m_distributedMoneyTotal;					// 플레이어가 가져간 금액 누적

	std::map<uint64 , uint64> m_distributers;	// 최종 결과 처리에서 m_sideMoney 보다 금액이 크면 권한을 가진 플레이어 이다.
	std::map<uint64 , uint64> m_winners;		// 사이드팟에 누적된 돈을 가져간 사람들을 저장해 놓는다.
	std::map<uint64 , uint64> m_sidePlayers;	// 사이드 발생 플레이어

public:
	cSidePot( const uint64 beforePotSideMoney, const uint64 sideMoney );

	// 베팅한 금액으로 이 사이드팟에 권한이 있는지 판단한다.
	void AddDistibuter( const uint64 playerIdx , const uint64 beforeSidePotBaseAmount , const uint64 betMoney );
	void IncreaseSidePlayerCount( const uint64 playerIdx );
	double GetPlayerMoney( const uint64 playerIdx );
	double GetLoserMoney();
	bool IsDistributer( const uint64 playerIdx );
	bool AddWinner( const uint64 playerIdx , const uint64 betMoney );
	void AddSidePlayer( const uint64 playerIdx );
	inline bool DistribteDone() { return m_sideMoneyTotal == 0; }
	bool areSidePotOwners( const std::vector<uint64> playerIdxList );
	bool isSidePotOwner( const uint64 playerIdx );
	void DistributedMoney( const uint64 distributedMoney );
	inline bool hasWinner() { return m_winners.size() > 0; }
	bool isWinner( const uint64 playerIdx );
	inline bool isCompleted() { return (m_distributedMoneyTotal+1) >= m_sideMoneyTotal; }

private:
	cSidePot() {}
};

// 금액별로 생성되어 있는 사이드 팟에 각 플레이어별로 가져갈 돈을 미리 계산해 둔다.
// 플레이어가 돈을 가져가게 되는 경우에만 사이드 목록을 전체 검색해서 나눠줄 돈만 가져가면 된다.
// 당연히 돈을 분배할 권한이 있는지는 체크 되어야 한다. ( Pot 이 남아 있는 경우라면 )
class cSidePotManager
{
protected:
	friend class IGame;

protected:
	std::map<uint64, cSidePot*> m_sidePots; // 금액별 사이드 팟으로 변경 ( key : side money, value : side pot )

	// operation
protected:
	void Clear();
	bool CreateSidePot( const uint64 playerIdx , const uint64 sideMoney );
	bool CreateHiddenSidePot( const uint64 playerIdx , const uint64 playerBetMoney, const uint64 lastBetMoney );
	void PushSidePotOnResult( const uint64 playerIdx , const uint64 sideMoney );
	bool WinnersHasSide( std::vector<uint64> winner_player_idx_list );
	cSidePot* GetSide( uint64 playerIdx );

	inline std::map<uint64 , cSidePot*> GetSides() { return m_sidePots; }
	std::map<uint64 , cSidePot*> GetSides( std::vector<uint64> playerIdxList );
	bool HasSidePotOwner( const uint64 playerIdx );

	// 이 함수는 m_sidePots 리턴 되도록 최종족으로 변경 되어야 함
	inline std::vector<cSidePot*> GetAllSides() {
		std::vector<cSidePot*> allSides;
		for ( auto side : m_sidePots ) {
			allSides.push_back( side.second );
		}
		return allSides;
	}
	inline bool HasSidePot() { return m_sidePots.size() > 0; }
	//uint64 GetAccumulatedMoney( uint64 playerIdx );
	void DeleteSidePots( const std::vector<uint64>& sideKeys );
	uint64 CurCompletedSidePotMoney();
};