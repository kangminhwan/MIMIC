#pragma once

#include "TableServerHeader.h"

#include <iostream>
#include <cmath>

// 하나의 건에 대해 투표는 1개만 가능
class cVote
{
private:
	std::map<uint64 , uint64> m_voted_players;
	int32 m_voteIndex;
	General::BallotKind m_voteType;
	int32 m_playersCount;
	int32 m_majorityCount;
	int32 m_againstCount;
	int32 m_agreeCount;
	uint64 m_voteRequesterPlayerIdx;
	uint64 m_issuerPlayerIdx;
	bool m_executed_action;
	uint64 m_vote_expiry_time;

public:
	cVote (const int32 voteIndex, const General::BallotKind voteType, int32 playersCount, const uint64 issuerPlayerIdx, const uint64 voteRequesterPlayerIdx)
	{
		m_voteIndex = voteIndex;
		m_playersCount = playersCount + 1;
		m_voteType = voteType;
		m_voted_players.insert(std::pair<uint64, uint64>( voteRequesterPlayerIdx , voteRequesterPlayerIdx ));
		m_majorityCount = static_cast< int >( std::ceil( static_cast< double >( playersCount - 1 ) / 2 ) ); // 과반수를 미리 설정한다. Issuer 는 빼고 카운트 한다.
		m_voteRequesterPlayerIdx = voteRequesterPlayerIdx;
		m_issuerPlayerIdx = issuerPlayerIdx;
		m_executed_action = false;
		m_vote_expiry_time = ::GetTickCount64() + 16000; // 16초 정도로 잡아 둔다.
		m_againstCount = 0;
		m_agreeCount = 1;
	}

	inline uint64 GetRequesterPlayerIdx() { return m_voteRequesterPlayerIdx; } const
	inline uint64 GetIssuerPlayerIdx() { return m_issuerPlayerIdx; } const
	inline int32 GetVoteIndex() { return m_voteIndex; } const
	inline General::BallotKind GetVoteType() { return m_voteType; } const
	inline int32 GetAgreeCount() { return m_agreeCount; } const
	inline int32 GetAgainstCount() { return m_againstCount; } const
	inline int32 GetRemainSecond()
	{
		if ( ::GetTickCount64() > m_vote_expiry_time )
			return 0;
		else
			return static_cast< int32 >( std::ceil( static_cast< double >( ( m_vote_expiry_time - ::GetTickCount64() ) ) / 1000 ) );
	}

	bool PlayerVote( const uint64 playerIdx , const bool isAgree )
	{
		auto iter = m_voted_players.find( playerIdx );
		if ( iter != m_voted_players.end() )
			return false;

		// expire time 이 지나면 투표 반영하지 않느다.
		if ( ::GetTickCount64() > m_vote_expiry_time )
			return false;

		if ( isAgree )
			++m_agreeCount;
		else
			++m_againstCount;

		m_voted_players.insert( std::pair<uint64 , uint64>( playerIdx , playerIdx ) );
		return true;
	}

	inline bool isSuccessVote() { return m_agreeCount >= m_majorityCount; }

	// 투표에 이어지는 액션을 취할지 여부, 1번 뿐이 못한다.
	inline void ExecuteAction()
	{
		if ( false == m_executed_action )
			m_executed_action = true;
	}

	inline bool isExcecutable() { return m_executed_action == false; }

	inline bool isExpired() {
		return ::GetTickCount64() > m_vote_expiry_time;
	}

//private:
	cVote() {}
};

class cVoteSystem
{

	friend class cHoldem;


private:
	int32 m_currentVoteIndex;
	std::map<int32 , cVote> m_voteMap;
	std::map<uint64 , uint64> m_voteRequesterMap;	// 1번의 투표가 처리가 완료되면, 더 이상 하지 못함
	std::vector<uint64> m_kickedPlayers;			// Kick 되었던 플레이어는 최대 5명 까지 저장해 둔다.

public:
	void InitVoteSystem()
	{
		m_currentVoteIndex = 0;
		m_voteRequesterMap.clear();
		m_voteMap.clear();
		m_kickedPlayers.clear();
	}

	virtual General::ResultCode StartVote( const General::BallotKind voteType, int32 playersCount, const uint64 issuerPlayerIdx, const uint64 voteRequesterPlayerIdx )
	{
		++m_currentVoteIndex;

		auto iter = m_voteRequesterMap.find( voteRequesterPlayerIdx );
		if ( iter != m_voteRequesterMap.end() )
			return General::ResultCode::Result_BallotAlreadySubmitted;

		cVote vote( m_currentVoteIndex, voteType, playersCount , issuerPlayerIdx, voteRequesterPlayerIdx );
		m_voteMap.insert(std::pair<int32 , cVote>( m_currentVoteIndex , vote ));

		return General::ResultCode::Result_Success;
	}

	virtual void OnVoteAction( cVote& voteAction ) 
	{ 
		voteAction.ExecuteAction();

		if ( voteAction.GetVoteType() == General::BallotKind::BallotKind_KickOut ) {

			// 킥당한 유저가 5명인 경우 1개를 삭제한다.
			while ( m_kickedPlayers.size() >= 4 ) {
				auto iter = m_kickedPlayers.begin();
				m_kickedPlayers.erase( iter );
			}

			uint64 issuerPlayerIdx = voteAction.GetIssuerPlayerIdx();
			m_kickedPlayers.push_back( issuerPlayerIdx );
		}
	}

	virtual void OnVoteResult( cVote& voteAction, const bool& isSuccess ) = 0;

	cVote* GetCurrentVote()
	{
		auto rIter = m_voteMap.rbegin();
		if ( rIter == m_voteMap.rend() )
			return nullptr;

		return &rIter->second;
	}

	cVote* GetVote( const int32 voteIndex )
	{
		auto iter = m_voteMap.find( voteIndex );
		if ( iter == m_voteMap.end() )
			return nullptr;

		if ( iter->second.isExpired() )
			return nullptr;

		return &iter->second;
	}

	bool PlayerVote( const int32 voteIndex , const uint64 playerIdx , const bool isAgree )
	{
		auto iter = m_voteMap.find(voteIndex);
		if ( iter == m_voteMap.end() )
			return false;

		return iter->second.PlayerVote( playerIdx , isAgree );
	}

	// 플레이어가 방에서 나갈때 호출한다.
	// 발의를 다시 할수 있도록 만들어 주어야 한다.
	void OnVoteSystemPlayerRoomOut(const uint64 playerIdx)
	{
		auto iter = m_voteRequesterMap.find( playerIdx );
		if ( iter != m_voteRequesterMap.end() )
			m_voteRequesterMap.erase(iter);
	}

	// 과반수가 넘었으면 투표 액션 실행
	bool VoteActionByPlayerVote( const int32 voteIndex  )
	{
		auto iter = m_voteMap.find( voteIndex );
		if ( iter == m_voteMap.end() )
			return false;

		if ( iter->second.isSuccessVote() ) {
			OnVoteAction( iter->second );
			return true;
		}
		
		return false;
	}

	// 결과 처리시에만 호출 하도록 변경
	void OnResultVoteProcess()
	{
		for ( auto votePair : m_voteMap ) {

			// expire 체크
			/*if ( votePair.second.isExpired() )
				continue;*/
			
			// 실행한적이 있는지 확인 
			if ( false == votePair.second.isExcecutable() )
				continue;

			// 과반이 넘은 투표에 대해서 딱 1번만 Action 을 실행한다.
			if ( votePair.second.isSuccessVote() ) {
				OnVoteAction( votePair.second );
			}
		}
	}

	// expired 된 투표가 있으면 삭제 하고 통보 해준다.
	void OnCheckVoteExpired()
	{
		std::vector<int32> expiredVotes;

		for ( auto& votePair : m_voteMap ) {

			// expire 되지 않았으면
			if ( false == votePair.second.isExpired() )
				continue;

			// 투표가 성공 했으면
			if ( votePair.second.isSuccessVote() )
				continue;

			expiredVotes.push_back( votePair.first );
		}

		for ( auto& voteIndex : expiredVotes ) {

			auto iter = m_voteMap.find( voteIndex );
			if ( iter != m_voteMap.end() ) {

				// expired 통보
				bool isSuccess = false;
				OnVoteResult( iter->second, isSuccess );

				// 투표 요청 했던 사람의 요청 기록 삭제
				auto requesterIdx = iter->second.GetRequesterPlayerIdx();
				auto reqIter = m_voteRequesterMap.find( requesterIdx );
				if ( reqIter != m_voteRequesterMap.end() ) {
					m_voteRequesterMap.erase( reqIter );
				}

				// 삭제
				m_voteMap.erase( iter );
			}


		}
	}

	// 이방에서 Kick 된 유저 인지 확인 한다.
	bool OnCheckKickedPlayer( const uint64& playerIdx) 
	{
		for ( auto kickedPlayerIdx : m_kickedPlayers ) {
			if ( kickedPlayerIdx == playerIdx )
				return true;
		}

		return false;
	}
};