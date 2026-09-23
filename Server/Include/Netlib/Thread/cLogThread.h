#pragma once
#include "cBaseThread.h"

BEGIN_NETLIB

class cInterfaceIocpContext;
class cInterfaceAlert;
class cLogThread : public cBaseThread
{
protected:
	std::list<cInterfaceIocpContext*> m_LogObservers;
	//cPacketStack* m_pPacket;
	cInterfaceAlert* m_interfaceAlert;

public:
	int m_LogReCreateMinute;
	ULONGLONG m_llCreationTick;

public:
	void InsertObserver(cInterfaceIocpContext* pObserverContext);
	void SetAlert(cInterfaceAlert* pInterfaceAlert);
	inline cInterfaceAlert* GetAlertInterface() { return m_interfaceAlert; }
	
public:
	void	Init();
	void	Destroy();

protected:
	virtual void Process(UINT ThreadArray) override;

public:
	cLogThread();
	virtual ~cLogThread();
};

END_NETLIB