#pragma once
#include "../Common/Netlib.h"

BEGIN_NETLIB

class cOverlapped
{
public:
	WSAOVERLAPPED	m_Overlapped;
	WORD			m_Operation;

public:
	void	Clean();

	void	SetOperation(E_IO_OPERATION eOperation);
	WORD	GetOperation();

public:
	cOverlapped();
	virtual ~cOverlapped();
};

END_NETLIB