#include "../../Include/Netlib/Queue/cWebQueueElement.h"
#include "../../Include/Netlib/IOCP/cIocpContext.h"


NetLib::cWebQueueElement::cWebQueueElement(const int BufferSize) :
	cBaseQueueElement(BufferSize)
{
	Init();
}


NetLib::cWebQueueElement::~cWebQueueElement()
{
	Destroy();
}

void NetLib::cWebQueueElement::Init()
{
	MakeInitialize();
}

void NetLib::cWebQueueElement::Destroy()
{
	MakeInitialize();
}

void NetLib::cWebQueueElement::MakeInitialize()
{
	cBaseQueueElement::Reset();
	m_nID = 0;
	m_nCommand = 0;
	m_pContext = nullptr;
}

BOOL NetLib::cWebQueueElement::CopyData(const NetLib::cIocpContext* pContext,
										const UINT nCommand,
										const BYTE* lpBuffer,
										const UINT nLength)
{
	if(cBaseQueueElement::CopyData(lpBuffer, nLength))
	{
		m_pContext = const_cast<NetLib::cIocpContext*>(pContext);
		m_nCommand = nCommand;
		m_nID = 0;
		return TRUE;
	}
	return FALSE;
}

BOOL NetLib::cWebQueueElement::CopyData(const UINT nID,
										const UINT nCommand,
										const BYTE* lpBuffer,
										const UINT nLength)
{
	if(cBaseQueueElement::CopyData(lpBuffer, nLength))
	{
		m_nID = nID;
		m_nCommand = nCommand;
		m_pContext = nullptr;
		return TRUE;
	}
	return FALSE;
}

UINT NetLib::cWebQueueElement::GetID() const
{
	return m_nID;
}

UINT NetLib::cWebQueueElement::GetCommand() const
{
	return m_nCommand;
}
NetLib::cIocpContext* NetLib::cWebQueueElement::GetContext() const
{
	return m_pContext;
}