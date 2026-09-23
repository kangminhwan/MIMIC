#pragma once
#include "../Common/Netlib.h"

BEGIN_NETLIB

class cInterfaceIocpContext;
class cSessionManager;
class cPacketStack;
class cDisPatcher
{
protected:
	//cPacket* m_cPacket;
	//cBuffer* m_pBuffer;

	NetLib::cPacketStack* m_pPacketStack;

	std::queue<cInterfaceIocpContext*> m_ErrorSession;
	std::queue<cInterfaceIocpContext*> m_SocketErrorSession;

private:
	cSessionManager*	m_pSessionManager;

private:
	void Init();
	void Destroy();

public:
	void SendPacket(cInterfaceIocpContext* pContext, UINT nCommand, BYTE* pData, UINT nLength);
	bool SendRequest(cInterfaceIocpContext* pContext, const BYTE* pPacket, const UINT unLength);
	void BroadCastToSessionsType(const Sessions  type, UINT nCommand, BYTE* pData, UINT nLength);
	void BroadCastToServerType(const E_SERVER_TYPE  type, UINT nCommand, BYTE* pData, UINT nLength);
	void BroadCastErrorSessionClear();

	cInterfaceIocpContext* GetContextPtr(UINT nEntity);
	cDisPatcher* GetDisPatcherPointer() { return this; }

#if defined(VIRTUAL_NAGLE_ON)
	void RunVitualNagle();
#endif

public:
	cDisPatcher();
	virtual ~cDisPatcher();
};

END_NETLIB