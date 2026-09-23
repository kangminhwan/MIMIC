#pragma once

typedef struct _tag_channelchange
{
	int nThreadArray;
	int nTargetChannel;
	int nPreChannel;
	int64 RequestAID;

	_tag_channelchange()
	{
		nThreadArray = -1;
		nTargetChannel = -1;
		nPreChannel = -1;
		RequestAID = 0;
	}

	_tag_channelchange(int nThread, int nTarget, int nPre, int64 requestAID)
	{
		nThreadArray = nThread;
		nTargetChannel = nTarget;
		nPreChannel = nPre;
		RequestAID = requestAID;
	}

}CHANNELCHANGE;

typedef struct _tag_channelchangesuccess
{
	int nThreadArray;
	int nTargetChannel;
	int64 OutAID;

	_tag_channelchangesuccess()
	{
		nThreadArray = -1;
		nTargetChannel = -1;
		OutAID = 0;
	}

	_tag_channelchangesuccess(int nThread, int nChannel, int64 outaid)
	{
		nThreadArray = nThread;
		nTargetChannel = nChannel;
		OutAID = outaid;
	}

}CHANNELCHANGESUCCESS;

struct WebChangeCharacterSuccess
{
	UINT Entity;
	int64 AccountIDX;
	int64 HeroIDX;
	ULONGLONG llUniqueKey;
	char szNickName[protoutil::E_PROTO_UTIL_BUFFER::MAX_ACCOUNT_NICKNAME_LEN_A];
	char szSessionID[CSDef::EDef::MAX_SESSIONID_LENGTH];

	WebChangeCharacterSuccess()
	{
		memset(this, 0x00, sizeof(WebChangeCharacterSuccess));
	}

	WebChangeCharacterSuccess(UINT _uiEntity, int64 _account_idx, int64 _hero_idx, ULONGLONG _llUnique, const char* pszNickName, const char* pszSessionID)
	{
		Entity = _uiEntity;
		AccountIDX = _account_idx;
		HeroIDX = _hero_idx;
		llUniqueKey = _llUnique;

		StringCbCopyA(szNickName, sizeof(szNickName), pszNickName);
		StringCbCopyA(szSessionID, sizeof(szSessionID), pszSessionID);
	}
};

struct ST_SYS_NET_CHANGE_COMMAND_INDEX_CONTEXT
{
	UINT nCommand;
	int nDataLength;
	char ResponseData[protoutil::E_PROTO_UTIL_BUFFER::MAX_BUFFER_32K_LEN];

	ST_SYS_NET_CHANGE_COMMAND_INDEX_CONTEXT()
	{
		memset(this, 0x00, sizeof(ST_SYS_NET_CHANGE_COMMAND_INDEX_CONTEXT));
	}

	int GetDataLength()
	{
		return nDataLength + sizeof(nCommand) + sizeof(nDataLength);
	}

	bool ValidationCheck()
	{
		return nDataLength <= sizeof(ResponseData);
	}
};

struct GuildMemberCleanUpData
{
	__int64 _i64Account_idx;
	__int64 _i64Hero_idx;
	__int64 _i64Guild_idx;
};

struct GildMemberInsertData
{
	__int64 _i64Account_idx;
	__int64 _i64Hero_idx;
	__int64 _i64Guild_idx;

	unsigned __int64 _i64uniquekey;
	void* pContext;
	void* pClientSession;
};

struct GuildMemberContextChange
{
	__int64 _i64Account_idx;
	__int64 _i64Hero_idx;
	__int64 _i64Guild_idx;

	unsigned __int64 _i64uniquekey;
	void* pContext;
};

struct GuildMember
{
public:
	__int64 _i64Account_idx;
	__int64 _i64Hero_idx;
	unsigned __int64 _i64uniquekey;
	void* pContext;
	void* pClientSession;

public:
	void clear()
	{
		_i64Account_idx = _i64Hero_idx = 0;
		_i64uniquekey = 0;
		pContext = nullptr;
		pClientSession = nullptr;
	}

};

struct GuildInteraction
{
public:
	__int64 _i64TargetAccountIDX;
	__int64 _i64TargetHeroIDX;
	__int64 _i64Guild_idx;
	unsigned int Command;
	char _characterNicknameUTF8[32];

	GuildInteraction()
	{
		memset(_characterNicknameUTF8, 0x00, sizeof(_characterNicknameUTF8));
	}
};