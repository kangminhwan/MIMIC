#pragma once
#include "TableServerHeader.h"
#include "cDataLoader.h"


#include "../Include/Netlib/Common/cSingleton.h"

using std::string;


class cLevel
{
public:
	string _id;
	int _level;
	uint64 _exp;
	uint64 _reward_chip;
};


class cLevelManager
{
public:
	static bool CheckLevelUp( int& level, const uint64& beforeExp, const uint64& earnExp )
	{
		//auto bets = NetLib::cSingleton<cDataLoader>::GetInstance()->GetTotalBets();

	}


};