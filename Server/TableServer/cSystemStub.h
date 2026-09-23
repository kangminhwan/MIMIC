#pragma once
#include "IStub.h"

// 서버 메시지 처리
// 스케쥴 잡 처리
class cSystemStub : public IStub
{
private:
	virtual void bindmethod(std::function<void(NetLib::cInterfaceIocpContext* /*pContext*/, UINT /*nCommand*/, BYTE* /*pData*/, UINT /*nLength*/, UINT /*nThreadIndex*/)>* fp) override {}
	virtual void bindmethod( std::function<void( UINT /*nID*/ , UINT /*nCommand*/ , BYTE* /*pData*/ , UINT /*nLength*/ , UINT /*nThreadIndex*/ )>* fp ) override;

	virtual void bindmethod(std::function<void(NetLib::cUDPDispatcher* /*pUDPDispatcher*/, UINT /*nCommand*/, UINT /*Entity*/, UINT /*uiPacketSeq*/, NetLib::cCommandQueueElement* /*pCommandQueueElement*/, BYTE* /*pData*/, UINT /*nLength*/, UINT /*nThreadIndex*/)>* fp) override {}

private:
	void SYS_SCHEDULE_MSG_FP(UINT nID, UINT nCommand, BYTE* pData, UINT nLength, UINT nThreadIndex);
	//void UpdateHeroSessionExpire(UINT nThreadIndex);

	void SYS_NET_SESSION_LOG_OUT(UINT nID , UINT nCommand , BYTE* pData , UINT nLength , UINT nThreadIndex);

public:
	cSystemStub();
	virtual ~cSystemStub();
};

