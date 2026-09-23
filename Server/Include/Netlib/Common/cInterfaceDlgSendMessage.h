#pragma once
#include "Netlib.h"

BEGIN_NETLIB

class cInterfaceDlgSendMessage
{
public:
	virtual void SendMessageToListBox(EStringColor eColor, TCHAR* szFormat, ...)
	{

	}

	virtual void SendLogMessage(EStringColor eColor, TCHAR* szFormat) = 0;

public:
	cInterfaceDlgSendMessage() {}
	virtual ~cInterfaceDlgSendMessage() {}
};

END_NETLIB