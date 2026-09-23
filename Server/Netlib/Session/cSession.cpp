#include "../../Include/Netlib/Session/cSession.h"
#include "../../Include/Netlib/Common/cInterfaceIocpContext.h"

NetLib::cSession::cSession()
{
	Init();
}


NetLib::cSession::~cSession()
{
	Destroy();
}

void NetLib::cSession::Init()
{
	SetSessionType(Sessions::SESSION_NONE);
	SetSessionStatus(E_SESSION_STATUS::E_SESSION_STATUS_NONE);
	SetServerType(E_SERVER_TYPE::NONE_SERVER);
	SetContext(nullptr);
	m_allocatedslot = 0;
	m_llalollocatedTableslot = 0;
	m_pConnectionInfo = nullptr;
	m_ullPendingTime = 0;
	//m_pUDPSession = nullptr;
}

void NetLib::cSession::Clear()
{
	Init();
}

void NetLib::cSession::Destroy()
{
}

size_t NetLib::cSession::CodedSessionInfo(char* pCodedBuffer, size_t nBufferSize)
{
	if (pCodedBuffer == nullptr || nBufferSize == 0)
	{
		return 0;
	}

	size_t nCodedSize = 0;

	char* pszIP = nullptr;
	char szIPv4[CSDef::EDef::MAX_IP_ADDRESS_LEN] = { 0, };

	if (m_pContext != nullptr)
	{
		if (m_pContext->IsUseIPv6())
		{
			pszIP = m_pContext->GetIPv6();
		}
		else
		{
			UINT uiIP = m_pContext->GetIP();
			inet_ntop(AF_INET, &uiIP, szIPv4, sizeof(szIPv4));
			pszIP = szIPv4;
		}
	}


	StringCbPrintfA(pCodedBuffer, nBufferSize, "Session Type [ %s ] IP [ %s ]", G_SESSION_NAME[m_eSessionType], pszIP == nullptr ? "null" : pszIP);

	StringCbLengthA(pCodedBuffer, nBufferSize, &nCodedSize);

	return nCodedSize;
}

//void NetLib::cSession::SetUDPSession(NetLib::cUDPSession* pUDPSession)
//{
//	m_pUDPSession = pUDPSession;
//}
//
//NetLib::cUDPSession* NetLib::cSession::GetUDPSession()
//{
//	return m_pUDPSession;
//}