#pragma once
#include "cBaseThread.h"

BEGIN_NETLIB

class cInterfacePacketParser;
class cInterfaceAlert;
class cCommandThread : public cBaseThread
{
protected:
	cInterfaceAlert* m_interfaceAlert;

public:
	cInterfacePacketParser* m_pInterfacePacketParser;

protected:
	virtual void Process(UINT ThreadArray) override;

public:
	void	Init();
	void	Destroy();

	void	SetParserInstancePtr(cInterfacePacketParser* pInterface);
	void SetAlert( cInterfaceAlert* pInterfaceAlert );
	cInterfacePacketParser* GetParserInstancePtr() const;
	
public:
	cCommandThread();
	virtual ~cCommandThread();
};

END_NETLIB