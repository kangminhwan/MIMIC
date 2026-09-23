#pragma once
#include "cUDPIocpOv.h"

BEGIN_NETLIB

class cUDPSession;
class cUDPIOCompletionData
{
public:
	cUDPIocpOv		m_pUDPIocpOv;
	cUDPSession*	m_pUDPSession;

public:
	void		SetUDPSession(cUDPSession* udpSession) { m_pUDPSession = udpSession; }
	cUDPIocpOv*	GetUDPIocpOv() { return &m_pUDPIocpOv; }
	cUDPSession* GetUDPSession() { return m_pUDPSession; }
	void	Clear();

public:
	cUDPIOCompletionData();
	~cUDPIOCompletionData();
};

END_NETLIB