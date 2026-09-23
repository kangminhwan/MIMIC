#pragma once
#include "cBaseThread.h"

BEGIN_NETLIB

class cWebThread : public cBaseThread
{
private:
	class cInterfacePacketParser* m_pInterfacePacketParser;

protected:
	virtual void Process(UINT uiThreadArray) override;

public:
	void Init();
	void Destroy();

	void PushWebThreadExit();
	bool WaitThreadTemination();

public:
	void SetParserInstancePtr(class cInterfacePacketParser* pInterface);
	class cInterfacePacketParser* GetparserInstancePtr() const;

public:
	cWebThread();
	~cWebThread();
};

END_NETLIB