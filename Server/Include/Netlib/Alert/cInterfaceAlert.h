#pragma once
#include "../Common/Netlib.h"

BEGIN_NETLIB

class cInterfaceAlert
{
public:
	virtual bool SendAlert( const tstring& alertmessage ) = 0;
	virtual bool SendDumpAlert( const tstring& stackTrace ) = 0;

public:
	cInterfaceAlert() {}
	virtual ~cInterfaceAlert() {}
};

END_NETLIB