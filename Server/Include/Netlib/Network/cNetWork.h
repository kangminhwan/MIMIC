#pragma once
#include "../Common/Netlib.h"

BEGIN_NETLIB

class cServerSocket;
class cIocpContext;
class cInterfaceIocpContext;
class cConnectorQueueElem;
class cNetWork
{
protected:
	std::vector<cServerSocket*>	m_cServerSocketList;

	int		nConnectedUserCount;

public:
	void	Init();
	void	Destroy();

	bool	NetworkStartup();
	void	NetworkCleanup();
public:
	bool	CreateServerIPv4Socket(WORD unPort, int iBacklog, UINT& nID, bool bUsingOutputBufferIntoAcceptRequest);
	bool	CreateServerIPv6Socket(WORD unPort, int iBacklog, UINT& nID, bool bUsingOutputBufferIntoAcceptRequest);
	void	DestroyServerSocket();

	bool	AcceptRequest(UINT nID);
	bool	BackLogAcceptRequest(UINT nID);
	void	AcceptCompleted(cIocpContext* pContext);

	LONG	AcceptCountReport();

public:
	void	FinishAndAccept(cIocpContext* pIocpContext);
	void	FinishErrorContext(cIocpContext* pIocpContext);
	void	FinishForApp(cInterfaceIocpContext* pContext)
	{
		FinishAndAccept(reinterpret_cast<cIocpContext*>(pContext));
	}

public:
	int		GetConnectedUserCount() { return nConnectedUserCount; }

	cIocpContext* PopContext();

public:
	cNetWork();
	~cNetWork();
};

END_NETLIB