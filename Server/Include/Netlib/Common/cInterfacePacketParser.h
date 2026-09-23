#pragma once
#include "Netlib.h"

BEGIN_NETLIB

class cInterfaceIocpContext;
class cUDPDispatcher;
class mRedis;
class cCommandQueueElement;

class cInterfacePacketParser
{
public:
	virtual void WebProcess(cInterfaceIocpContext* pContext, UINT nCommand, BYTE* pData, UINT nLength, UINT nThreadIndex) = 0;
	virtual void WebProcess(UINT nID, UINT nCommand, BYTE* pData, UINT nLength, UINT nThreadIndex) = 0;
	virtual void Process(cUDPDispatcher* pUDPDispatcher, UINT nCommand, UINT Entity, UINT uiPacketSeq, cCommandQueueElement* pCommandQueueElement, BYTE* pData, UINT nLength, UINT nThreadIndex) = 0;
	virtual void Process(UINT nID, UINT nCommand, BYTE* pData, UINT nLength) = 0;
	virtual void Process(cInterfaceIocpContext* pContext, UINT nCommand, BYTE* pData, UINT nLength, UINT nThreadIndex) = 0;
	virtual void Process(UINT nID, UINT nCommand, BYTE* pData, UINT nLength, UINT nThreadIndex, mRedis* pRedis) = 0;

	virtual void NotifyContextLogout(UINT Entity) = 0;

	cInterfacePacketParser() {}
	virtual ~cInterfacePacketParser() {}
};

END_NETLIB