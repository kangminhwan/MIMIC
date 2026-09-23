#pragma once
#include "../Common/Netlib.h"

BEGIN_NETLIB

class cUdpSocket
{
protected:
	SOCKET m_hUdpSocket;

public:
	void Init();
	void Destroy();

public:
	virtual void Disconnect();

public:
	BOOL CreateSocket();
	void CloseSocket();
	SOCKET GetSockHandle();

	bool ExtendSockBuffer();

public:
	cUdpSocket();
	~cUdpSocket();
};

END_NETLIB