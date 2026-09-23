#pragma once
#include "CommonFlag.h"
#include "CommonDefine.h"

typedef int int32;
typedef unsigned int uint32;
typedef __int64 int64;
typedef unsigned __int64 uint64;
typedef unsigned short WORD;
typedef unsigned char BYTE;

struct GOOGLE_PROTOBUF_BUFFER
{
	GOOGLE_PROTOBUF_BUFFER() { Clear(); }
	void Clear() { memset(this, 0x00, sizeof(GOOGLE_PROTOBUF_BUFFER)); }

	BYTE DataBuffer[protoutil::E_PROTO_UTIL_BUFFER::MAX_BUFFER_64K_LEN];
	BYTE SerializeBuffer[protoutil::E_PROTO_UTIL_BUFFER::MAX_BUFFER_64K_LEN];
};

struct WebRequestData
{
	ULONGLONG _i64ContextUniqueKey;
	int nDataLength;
	char RequestData[protoutil::E_PROTO_UTIL_BUFFER::MAX_BUFFER_32K_LEN];

	WebRequestData()
	{
		memset(this, 0x00, sizeof(WebRequestData));
	}

	bool SetData(ULONGLONG _ContextUniqueKey, BYTE* pData, UINT nLength)
	{
		if (pData == nullptr || nLength > sizeof(RequestData))
		{
			return false;
		}

		_i64ContextUniqueKey = _ContextUniqueKey;
		nDataLength = static_cast<int>(nLength);
		//StringCbCopyA(RequestData, sizeof(RequestData), reinterpret_cast<char*>(pData));
		memcpy(RequestData, pData, nLength);

		return true;
	}

	int GetDataLength()
	{
		return nDataLength + sizeof(_i64ContextUniqueKey) + sizeof(nDataLength);
	}

	bool ValidationCheck()
	{
		return nDataLength <= sizeof(RequestData);
	}
};

struct WebResponseData
{
	ULONGLONG _i64ContextUniqueKey;
	int nDataLength;
	int server_error_code;
	char ResponseData[protoutil::E_PROTO_UTIL_BUFFER::MAX_BUFFER_32K_LEN];

	WebResponseData()
	{
		memset(this, 0x00, sizeof(WebResponseData));
	}

	bool SetData(ULONGLONG _ContextUniqueKey, const char* pData, size_t nLength, int _server_error_code = 80000)
	{
		if (pData == nullptr ||
			nLength > sizeof(ResponseData))
		{
			return false;
		}

		_i64ContextUniqueKey = _ContextUniqueKey;
		nDataLength = static_cast<int>(nLength);
		server_error_code = _server_error_code;
		//nDataLength = nLength + sizeof(llContextUniqueKey) + sizeof(size_t);
		//StringCbCopyA(ResponseData, sizeof(ResponseData), pData);
		memcpy(ResponseData, pData, nLength);

		return true;
	}

	int GetDataLength()
	{
		return nDataLength + sizeof(_i64ContextUniqueKey) + sizeof(nDataLength) + sizeof(server_error_code);
	}

	bool ValidationCheck()
	{
		return nDataLength <= sizeof(ResponseData);
	}
};

#pragma pack(1)

// 매칭 서버에서 사용
struct ConnectionServerInfo
{
	ConnectionServerInfo()
	{
		Clear();
	}

	void Clear()
	{
		eServerType = SIGNEDINTMAX;
		_i64AllocatedKey = -1;

		uiMaxUserCnt = UNSIGNEDINTMAX;
		uiCurUserCnt = UNSIGNEDINTMAX;

		uiMaxRoomCnt = UNSIGNEDINTMAX;
		uiCurRoomCnt = UNSIGNEDINTMAX;
	}

	void operator=(const ConnectionServerInfo& _info)
	{
		this->eServerType = _info.eServerType;
		this->_i64AllocatedKey = _info._i64AllocatedKey;

		this->uiMaxUserCnt = _info.uiMaxUserCnt;
		this->uiCurUserCnt = _info.uiCurUserCnt;

		this->uiMaxRoomCnt = _info.uiMaxRoomCnt;
		this->uiCurRoomCnt = _info.uiCurRoomCnt;
	}

	int eServerType;
	int64 _i64AllocatedKey;

	UINT uiMaxUserCnt;
	UINT uiCurUserCnt;

	UINT uiMaxRoomCnt;
	UINT uiCurRoomCnt;
};

struct stServerInfo
{
	WORD usServerType;
	UINT PORT;

	void ServerInfoClear()
	{
		usServerType = 0;
		PORT = 0;
	}
};

struct stMatchInfo
{
	/*
	* ConstantTemplate 에 ContentType의 값이 들어갑니다.
	*/
	int nMatchType;

	/*
	* nRankTierID 이 값은 게임서버에서 보내옵니다. ArenaRankTemplate 에있는 ID 값입니다.
	* nMinRankTierID, nMaxRankTierID 이 것에 대한 설명은 밑에 설명이 있습니다.
	*/
	int nRankPoint;		// 팀 설정할때 필요한 변수 입니다.
	int nMinRankPoint;
	int nMaxRankPoint;

	/*
	* nExpansionCnt4RankTier => Rank 확장 카운트
	* nExpansionCoefficient 확장 계수
	* nRankTier 기반으로 nMinRankTier 값과 nMaxRankTier 값이 늘어나거나 줄어듭니다.
	*/
	int nExpansionCnt4RankTier;
	int nExpansionCoefficient;

	/*
	* nExpansionCnt4RankTier 최대 커질수 있는 제한값
	*/
	int nExpansionMaxCnt4RankTier;

	/*
	* 매칭 확장 기준 시간
	*/
	int nExpansionTime;

	// 해당 게임 타입의 맥스플레이어
	int nMaxPlayer;

	// 팀 입니다. 매칭 서버에서 정해줍니다.
	int nTeam;
	int nStartPosIndex;

	//BYTE pvpotherhero[protoutil::E_PROTO_UTIL_BUFFER::MAX_BUFFER_128_LEN];	//	게임 서버의 cAccount.h => m_pvpotherhero 와 공유하고있습니다.

	void MatchInfoClear()
	{
		nMatchType = 0;

		nRankPoint = 0;
		nMinRankPoint = 0;
		nMaxRankPoint = 0;

		nExpansionCnt4RankTier = 0;
		nExpansionCoefficient = 0;

		nExpansionMaxCnt4RankTier = 0;

		nExpansionTime = 0;

		nMaxPlayer = 0;

		nTeam = 0;
		nStartPosIndex = 0;

		//memset(pvpotherhero, 0x00, sizeof(pvpotherhero));
	}
};

struct Req_Server_Match : stServerInfo, stMatchInfo
{
protected:
	Req_Server_Match()
	{
		Req_Server_Match_Clear();
	}

public:
	Req_Server_Match(WORD _usServerType, int _nMatchType, int64 _i64AllocatedSlot, int64 _i64AccountIDX, int nCurRankPoint, int _nExpansionCoefficient, int _nExpansionMaxCnt4RankTier, int _nExpansionTime, int _nMaxPlayer, UINT _PORT)
	{
		Req_Server_Match_Clear();

		// stServerInfo Value
		usServerType = _usServerType;
		PORT = _PORT;

		//stMatchInfo Value
		nMatchType = _nMatchType;
		nRankPoint = nCurRankPoint;
		nExpansionCoefficient = _nExpansionCoefficient;
		nExpansionMaxCnt4RankTier = _nExpansionMaxCnt4RankTier;
		nExpansionTime = _nExpansionTime;
		nMaxPlayer = _nMaxPlayer;

		i64Allocatedslot = _i64AllocatedSlot;
		i64AccountIDX = _i64AccountIDX;

		MatchingReqTimeTick = ::GetTickCount64();


		/*if (_pvpotherhero != nullptr)
		{
			memcpy(pvpotherhero, _pvpotherhero, sizeof(pvpotherhero));
		}*/
	}

	void Req_Server_Match_Clear()
	{
		ServerInfoClear();
		MatchInfoClear();

		MatchingReqTimeTick = 0;
		i64AccountIDX = 0;
		i64Allocatedslot = 0;
	}

	ULONGLONG MatchingReqTimeTick;

	int64 i64AccountIDX;
	int64 i64Allocatedslot;
};

struct Res_Server_Match : stServerInfo
{
	Res_Server_Match(WORD _usServerType, int _nMatchType, int64 _i64AccountIDX)
	{
		usServerType = _usServerType;
		nMatchType = _nMatchType;
		i64AccountIDX = _i64AccountIDX;
	}

	int nMatchType;
	int nResult;
	int64 i64AccountIDX;
};

struct MatchElement : Req_Server_Match
{
	MatchElement() :
		Req_Server_Match()
	{
		MatchingCurTimeTick = 0;
	}

	MatchElement(const MatchElement& _match) :
		Req_Server_Match(_match.usServerType, _match.nMatchType, _match.i64Allocatedslot, _match.i64AccountIDX, _match.nRankPoint, _match.nExpansionCoefficient, _match.nExpansionMaxCnt4RankTier, _match.nExpansionTime, _match.nMaxPlayer, _match.PORT)
	{
		nTeam = _match.nTeam;
		nStartPosIndex = _match.nStartPosIndex;
	}

	void Clear()
	{
		Req_Server_Match_Clear();

		MatchingCurTimeTick = 0;
	}

	ULONGLONG MatchingCurTimeTick;
};

struct Res_Match_Failed
{
	int nResult;
	int nServerType;
	int64 i64AccountIDX;
	int64 i64AllocatedSlot;
};

struct Match_Success
{
	Match_Success()
	{
		memset(_element, 0x00, sizeof(MatchElement) * 4);
		nMatchType = 0;
		nGameRoomNumber = 0;
		nManagedThread = 0;
	}

	int nMatchType;
	MatchElement _element[4];
	int nGameRoomNumber;
	int nManagedThread;
};

struct Match_Server_Change
{
	Match_Server_Change(MatchElement* _pElement, int _nRoomNumber, const char* szChangeServerIP, int nPort, int64 i64ServerKey)
	{
		memset(this, 0x00, sizeof(Match_Server_Change));
		memcpy(&_matchElement, _pElement, sizeof(MatchElement));
		strcpy_s(_szChangeServerIP, _countof(_szChangeServerIP), szChangeServerIP);
		nRoomNumber = _nRoomNumber;
		_nPort = nPort;
		_i64ServerKey = i64ServerKey;
		_nStartposIndex = _pElement->nStartPosIndex;
	}

	MatchElement _matchElement;
	char _szChangeServerIP[100];	// 이동할 서버의 dns, 가속기 적용후에는 가속기의 dns
	int _nPort;							// 이동할 서버의 port, 가속기 적용후에는 가속기의 port
	int nRoomNumber;				// 입장할 방번호
	int64 _i64ServerKey;				// 이동할 서버의 서버 Key

	// TCPPVPMatchingCompleteResponse 의 정보들 추가
	int _nAkamaiUdpPort;			// 이동할 서버의 방에 해당되는 가속기의 udp port, 여기서 셋팅 가능한지는 확인
	int _nStartposIndex;				//	참여한 방의 처음 시작위치
};

struct REQ_Match_Cancel
{
	REQ_Match_Cancel()
	{
		i64Allocatedslot = 0;
		i64AccountIDX = 0;
		nMatchType = 0;
		bMatchMustBeNotify = false;
	}

	int64 i64Allocatedslot;
	int64 i64AccountIDX;
	int nMatchType;
	bool bMatchMustBeNotify;
};

struct RES_Match_Cancel
{
	int64 i64AccountIDX;
	int nMatchType;
	bool bMatchCancel;
	bool bMustNotify;
};

#pragma pack()