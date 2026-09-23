#pragma once

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/Server.pb.h"

#include <atlcoll.h>

/*
로우 바둑이 베팅 로직이 복잡하여
클라스 따로 분리해서 처리한다.
*/

// 1개의 베팅 스텝마다 유저의 모든 베팅을 저장한다.
// Bet_1 는 베팅 리스트 사이즈가 2개까지
// Bet_2, Bet_3, Bet_4 은 3개까지 가능하다.
#define BETTING_ROUND_VEC std::map<int , std::vector<General::TableAction>>

class cBettingChecker
{
	friend class cHoldem;
private:
	std::map<Server::PlayPhase , BETTING_ROUND_VEC*> m_bettingStocks;

public:
	virtual void CreateStockers();
	virtual void Delete();
	virtual void Clear();

	virtual bool Bet( Server::PlayPhase bettingStep , const int betRound , General::TableAction betting);
	virtual bool StepBbingCheck( Server::PlayPhase bettingStep );
	virtual bool IsStepFirstBet( Server::PlayPhase bettingStep );
	virtual General::TableAction GetPenultimateBet( Server::PlayPhase bettingStep , const int betRound, bool& gotoNextStep);
	virtual bool RoundHasBetting( Server::PlayPhase bettingStep , const int betRound);
};