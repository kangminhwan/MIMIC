#include "../../Include/Netlib/IOCP/cIocpConnector.h"
#include "../../Include/Netlib/IOCP/cIocpContext.h"
#include "../../Include/Netlib/Network/cPacketStack.h"
#include "../../Include/Netlib/Common/cSingleton.h"
#include "../../Include/Netlib/Manager/ServerManager.h"
#include "../../Include/Netlib/Queue/cLogQueue.h"

NetLib::cIocpConnector::cIocpConnector()
{
	for (int n = 0; n < E_SERVER_TYPE::SERVER_TYPE_MAX; ++n)
	{
		m_connectedServerMaps[n].InitHashTable(CSDef::EDef::MAX_SERVER_HASHMAP_SIZE);
	}
}


NetLib::cIocpConnector::~cIocpConnector()
{
	Destroy();
}

void NetLib::cIocpConnector::Destroy()
{
	//ContextPool 에서 cIocpContext 메모리 해제 해주고 있습니다.

	for (int n = 0; n < E_SERVER_TYPE::SERVER_TYPE_MAX; ++n)
	{
		POSITION pos = m_connectedServerMaps[n].GetStartPosition();

		while (pos != nullptr)
		{
			ConnectorInfo* pConnectorInfo = m_connectedServerMaps[n].GetValueAt(pos);
			if(pConnectorInfo != nullptr)
			{
				delete pConnectorInfo;
			}

			m_connectedServerMaps[n].GetNext(pos);
		}
		m_connectedServerMaps[n].RemoveAll();
	}

	POSITION pos = m_myConnectorMap.GetStartPosition();

	while (pos)
	{
		ATL::CAtlMap<int64, NetLib::cIocpContext*>* _atlValue = m_myConnectorMap.GetValueAt(pos);
		POSITION OldPos = pos;
		m_myConnectorMap.GetNext(pos);
		m_myConnectorMap.RemoveAtPos(OldPos);
		if (_atlValue != nullptr)
		{
			_atlValue->RemoveAll();
			delete _atlValue;
		}
	}

	m_myConnectorMap.RemoveAll();
}

bool NetLib::cIocpConnector::InsertConnector(E_SERVER_TYPE eServerType, NetLib::cIocpContext* pIocpContext)
{
	if(pIocpContext == nullptr)
	{
		return false;
	}

	ATL::CAtlMap<E_SERVER_TYPE, ATL::CAtlMap<int64, NetLib::cIocpContext*>*>::CPair* pPair = m_myConnectorMap.Lookup(eServerType);
	if(pPair == nullptr)
	{
		ATL::CAtlMap<int64, NetLib::cIocpContext*>* atlmapConnector = new ATL::CAtlMap<int64, NetLib::cIocpContext*>();
		m_myConnectorMap.SetAt(eServerType, atlmapConnector);

		pPair = m_myConnectorMap.Lookup(eServerType);
		if (pPair == nullptr)
		{
			return false;
		}
	}

	pIocpContext->SetConnectorStatus(E_IOCP_CONNECTOR_STATUS::E_IOCP_CONNECTOR_STATUS_DISCONNECTED);

	uint32 uiIP = 0;
	inet_pton(AF_INET, pIocpContext->GetConnectorTargetIP(), &uiIP);
	uint32 uiPORT = static_cast<uint32>(pIocpContext->GetConnectorTargetPort());

	int64 key = 0;
	MakeServerMapKey(key, uiIP, uiPORT);

	ATL::CAtlMap<int64, NetLib::cIocpContext*>::CPair* pPair1 = pPair->m_value->Lookup(key);
	if (pPair1 != nullptr)
	{
		return false;
	}

	pPair->m_value->SetAt(key, pIocpContext);

	return true;
}

void NetLib::cIocpConnector::KeepConnect()
{
	NetLib::ServerManager* pServerManager = cSingleton<NetLib::ServerManager>::ExistsInstance();
	if(pServerManager == nullptr)
	{
		cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "cIocpConnector::KeepConnect() Failed. ServerManager is nullptr");
		return;
	}

	TServerConfiguration* pConfig = pServerManager->GetConfiguration();
	if(pConfig == nullptr)
	{
		cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "cIocpConnector::KeepConnect() Failed. TServerConfiguration is nullptr");
		return;
	}

	char* strPublicIP = pServerManager->GetPublicIP();
	if(strPublicIP == nullptr)
	{
		cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "cIocpConnector::KeepConnect() Failed. PublicIP is nullptr");
		return;
	}

	POSITION pos = m_myConnectorMap.GetStartPosition();
	while (pos != nullptr)
	{
		ATL::CAtlMap<int64, NetLib::cIocpContext*>* atlConnector = m_myConnectorMap.GetValueAt(pos);

		POSITION ConnectorPOS = atlConnector->GetStartPosition();
		while(ConnectorPOS != nullptr)
		{
			ATL::CAtlMap<int64, NetLib::cIocpContext*>::CPair* pPair = atlConnector->GetAt(ConnectorPOS);
			if (pPair == nullptr)
			{
				atlConnector->GetNext(ConnectorPOS);
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "cIocpConnector::KeepConnect Failed. ");
				continue;
			}


			NetLib::cIocpContext* pIocpContext = pPair->m_value;
			if(pIocpContext == nullptr)
			{
				atlConnector->GetNext(ConnectorPOS);
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, 
					"NetLib::cIocpConnector::KeepConnect IocpContext is nullptr");
				continue;
			}

			if (pIocpContext->GetConnectorStatus() != E_IOCP_CONNECTOR_STATUS::E_IOCP_CONNECTOR_STATUS_DISCONNECTED)
			{
				atlConnector->GetNext(ConnectorPOS);
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, 
					"NetLib::cIocpConnector::KeepConnect Connector Status[ %s ]",
					G_CONNECTOR_STATUS[pIocpContext->GetConnectorStatus()]);
				continue;
			}

			char* targetip = nullptr;
			UINT targetport = 0;

			targetip = pIocpContext->GetConnectorTargetIP();
			targetport = pIocpContext->GetConnectorTargetPort();
			if (targetip == nullptr || strlen(targetip) == 0 || targetport == 0)
			{
				atlConnector->GetNext(ConnectorPOS);
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, "NetLib::cIocpConnector::KeepConnect Failed. #1");
				continue;
			}

			BOOL bUseIPv6 = pIocpContext->IsUseIPv6();
			pIocpContext->ReCreateSocket(bUseIPv6);

			BOOL bPrivate = TRUE;
			pIocpContext->SetCrypt(true);

			//Req_Server_Connect reqConn;

			////// Connect 하면서 데이터를 보냅니다.
			////// 현재 매칭 서버의 경우 OutBuffer를 받는 Listen Socket만 열려 있기 때문에 다른 포트를 보내야합니다. (2017_06_15)
			//if (pConfig->byServerType == E_SERVER_TYPE::MATCHING_SERVER)
			//{
			//	reqConn.SetReqServerConnector(pConfig->byServerType, pConfig->SID_FOR_MANAGE, pConfig->GID_FOR_MANAGE, pConfig->wBufferServerPort, strPublicIP);
			//}
			//else
			//{
			//	reqConn.SetReqServerConnector(pConfig->byServerType, pConfig->SID_FOR_MANAGE, pConfig->GID_FOR_MANAGE, pConfig->wDefaultServerPort, strPublicIP);
			//}

			//////암호화를 해서 패킷을 보냅니다.
			//pIocpContext->MakeConnectPacket(reinterpret_cast<BYTE*>(&reqConn), sizeof(reqConn));

			if (pIocpContext->ConnectToServer(targetip, targetport, pIocpContext->GetConnectorIPHint(), pIocpContext, bPrivate) == FALSE)
			{
				// 실패하면 로그를 남깁니다.
				NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI, 
					"NetLib::cIocpConnector::KeepConnect Connect Error");
			}

			atlConnector->GetNext(ConnectorPOS);
		}

		m_myConnectorMap.GetNext(pos);
	}
}

BOOL NetLib::cIocpConnector::CheckServerConnected(E_SERVER_TYPE eServertype, uint32 _uiIP, uint32 _uiPORT)
{
	ATL::CAtlMap<E_SERVER_TYPE, ATL::CAtlMap<int64, NetLib::cIocpContext*>*>::CPair* pPair = m_myConnectorMap.Lookup(eServertype);
	if(pPair == nullptr)
	{
		return FALSE;
	}

	int64 _i64Key = 0;
	MakeServerMapKey(_i64Key, _uiIP, _uiPORT);

	ATL::CAtlMap<int64, NetLib::cIocpContext*>::CPair* pPair1 = pPair->m_value->Lookup(_i64Key);
	if (pPair1 == nullptr)
	{
		return FALSE;
	}

	NetLib::cIocpContext* pIocpContext = pPair1->m_value;
	if (pIocpContext == nullptr)
	{
		return FALSE;
	}

	if (pIocpContext->GetConnectorStatus() != E_IOCP_CONNECTOR_STATUS::E_IOCP_CONNECTOR_STATUS_CONNECTED)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL NetLib::cIocpConnector::CheckServerConnected(uint32 _uiIP, uint32 _uiPORT)
{
	int64 _i64Key = 0;
	MakeServerMapKey(_i64Key, _uiIP, _uiPORT);

	POSITION pos = m_myConnectorMap.GetStartPosition();
	while (pos)
	{
		ATL::CAtlMap<int64, NetLib::cIocpContext*>* _value = m_myConnectorMap.GetValueAt(pos);
		if (_value != nullptr)
		{
			ATL::CAtlMap<int64, NetLib::cIocpContext*>::CPair* pPair = _value->Lookup(_i64Key);
			if (pPair != nullptr)
			{
				return TRUE;
			}
		}

		m_myConnectorMap.GetNext(pos);
	}

	return FALSE;
}

BOOL NetLib::cIocpConnector::CheckServerConnected(char* _pszIP, uint32 _uiPORT)
{
	if (_pszIP == nullptr)
	{
		return FALSE;
	}
	uint32 uiIP = 0;
	inet_pton(AF_INET, _pszIP, &uiIP);

	return CheckServerConnected(uiIP, _uiPORT);
}

E_ERROR_SEND NetLib::cIocpConnector::SendPacket(E_SERVER_TYPE eServertype, UINT nCommand, BYTE* pData, UINT nLength)
{
	ATL::CAtlMap<E_SERVER_TYPE, ATL::CAtlMap<int64, NetLib::cIocpContext*>*>::CPair* pPair = m_myConnectorMap.Lookup(eServertype);
	if(pPair == nullptr)
	{
		return E_ERROR_SEND::E_ERROR_SEND_ERROR;
	}

	POSITION pos = pPair->m_value->GetStartPosition();
	while (pos)
	{
		NetLib::cIocpContext* pContext = pPair->m_value->GetValueAt(pos);
		if (pContext == nullptr)
		{
			return E_ERROR_SEND::E_ERROR_SEND_ERROR;
		}

		if (pContext->GetConnectorStatus() != E_IOCP_CONNECTOR_STATUS_CONNECTED)
		{
			return E_ERROR_SEND::E_ERROR_SEND_ERROR;
		}

		pPair->m_value->GetNext(pos);
	}

	pos = pPair->m_value->GetStartPosition();
	while (pos)
	{
		NetLib::cIocpContext* pIocpContext = pPair->m_value->GetValueAt(pos);

		NetLib::cPacketStack packet(CSNet::E_PROTOCOL::E_TCP);

#if defined(PACKET_ANALYZE_ON)
		packet.Make(nCommand, pData, nLength, pIocpContext->GetSendPacketCnt());
#else
		packet.MakeManualEncrypt(nCommand, pData, nLength, pIocpContext->GetSendPacketCnt());
#endif

		E_ERROR_SEND error = pIocpContext->SendRequest(packet.GetBuffer(), packet.GetLength());

		if (E_ERROR_SEND::E_ERROR_SEND_OK != error)
		{
			return error;
		}

		pPair->m_value->GetNext(pos);
	}

	return E_ERROR_SEND::E_ERROR_SEND_OK;
}

E_ERROR_SEND NetLib::cIocpConnector::TargetSendPacket(E_SERVER_TYPE eServertype, uint32 _uiIP, uint32 _uiPORT, UINT nCommand, BYTE* pData, UINT nLength)
{
	ATL::CAtlMap<E_SERVER_TYPE, ATL::CAtlMap<int64, NetLib::cIocpContext*>*>::CPair* pPair = m_myConnectorMap.Lookup(eServertype);
	if (pPair == nullptr)
	{
		return E_ERROR_SEND::E_ERROR_SEND_ERROR;
	}

	int64 _i64Key = 0;
	MakeServerMapKey(_i64Key, _uiIP, _uiPORT);
	ATL::CAtlMap<int64, NetLib::cIocpContext*>::CPair* pPair1 = pPair->m_value->Lookup(_i64Key);
	if (pPair1 == nullptr)
	{
		return E_ERROR_SEND::E_ERROR_SEND_ERROR;
	}

	NetLib::cIocpContext* pContext = pPair1->m_value;
	if (pContext == nullptr)
	{
		E_ERROR_SEND::E_ERROR_SEND_ERROR;
	}

	NetLib::cPacketStack packet(CSNet::E_PROTOCOL::E_TCP);

#if defined(PACKET_ANALYZE_ON)
	packet.Make(nCommand, pData, nLength, pContext->GetSendPacketCnt());
#else
	packet.MakeManualEncrypt(nCommand, pData, nLength, pContext->GetSendPacketCnt());
#endif

	E_ERROR_SEND error = pContext->SendRequest(packet.GetBuffer(), packet.GetLength());

	return error;
}

E_ERROR_SEND NetLib::cIocpConnector::TargetExceptSendPacket(E_SERVER_TYPE eServertype, int64 _i64AllocatedSlot, UINT nCommand, BYTE* pData, UINT nLength)
{
	cCSLock Lock(&m_ConnectionLocks);

	POSITION pos = m_connectedServerMaps[eServertype].GetStartPosition();

	while (pos != nullptr)
	{
		ATL::CAtlMap<__int64, ConnectorInfo*>::CPair* pPair = m_connectedServerMaps[eServertype].GetAt(pos);

		ConnectorInfo* info = pPair->m_value;
		if (info == nullptr)
			continue;

		if (info->allocatedslot == _i64AllocatedSlot)
			continue;

		NetLib::cIocpContext* pContext = reinterpret_cast<NetLib::cIocpContext*>(info->pServerContext);

		if (pContext != nullptr)
		{
			NetLib::cPacketStack packet(CSNet::E_PROTOCOL::E_TCP);

#if defined(PACKET_ANALYZE_ON)
			packet.Make(nCommand, pData, nLength, pContext->GetSendPacketCnt());
#else
			packet.MakeManualEncrypt(nCommand, pData, nLength, pContext->GetSendPacketCnt());
#endif

			E_ERROR_SEND error = pContext->SendRequest(packet.GetBuffer(), packet.GetLength());
		}

		m_connectedServerMaps[eServertype].GetNext(pos);
	}

	return E_ERROR_SEND::E_ERROR_SEND_OK;
}

E_ERROR_SEND NetLib::cIocpConnector::TargetSendPacket(E_SERVER_TYPE eServertype, int64 _i64AllocatedSlot, UINT nCommand, BYTE* pData, UINT nLength)
{
	NetLib::cIocpContext* pContext = nullptr;

	ATL::CAtlMap<E_SERVER_TYPE, ATL::CAtlMap<int64, NetLib::cIocpContext*>*>::CPair* pPair = m_myConnectorMap.Lookup(eServertype);
	if (pPair != nullptr)
	{
		ATL::CAtlMap<int64, NetLib::cIocpContext*>::CPair* pPair1 = pPair->m_value->Lookup(_i64AllocatedSlot);
		if (pPair1 != nullptr)
		{
			pContext = pPair1->m_value;
		}
	}

	if (pContext == nullptr)
	{
		cCSLock Lock(&m_ConnectionLocks);

		ATL::CAtlMap<__int64, ConnectorInfo*>::CPair* pPair = m_connectedServerMaps[eServertype].Lookup(_i64AllocatedSlot);
		if (pPair == nullptr)
		{
			return E_ERROR_SEND::E_ERROR_SEND_ERROR;
		}

		ConnectorInfo* info = pPair->m_value;

		pContext = reinterpret_cast<NetLib::cIocpContext*>(info->pServerContext);
		if (pContext == nullptr)
		{
			return E_ERROR_SEND::E_ERROR_SEND_ERROR;
		}
	}

	NetLib::cPacketStack packet(CSNet::E_PROTOCOL::E_TCP);

#if defined(PACKET_ANALYZE_ON)
	packet.Make(nCommand, pData, nLength, pContext->GetSendPacketCnt());
#else
	packet.MakeManualEncrypt(nCommand, pData, nLength, pContext->GetSendPacketCnt());
#endif

	E_ERROR_SEND error = pContext->SendRequest(packet.GetBuffer(), packet.GetLength());

	return error;
}

// 전송된 서버가 없으면 에러를 리턴해 준다.
E_ERROR_SEND NetLib::cIocpConnector::TargetAllSendPacket(E_SERVER_TYPE eServertype, UINT nCommand, BYTE* pData, UINT nLength)
{
	int sended_count = 0;

	ATL::CAtlMap<E_SERVER_TYPE, ATL::CAtlMap<int64, NetLib::cIocpContext*>*>::CPair* pPair = m_myConnectorMap.Lookup(eServertype);

	// 서버타입으로 보낼곳이 없다.
	if (pPair == nullptr && m_connectedServerMaps[eServertype].GetCount() == 0)
		return E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET;

	if (pPair != nullptr)
	{
		POSITION pos = pPair->m_value->GetStartPosition();

		while (pos)
		{
			ATL::CAtlMap<int64, NetLib::cIocpContext*>::CPair* pConnectorPair = pPair->m_value->GetAt(pos);

			NetLib::cIocpContext* pContext = pConnectorPair->m_value;

			NetLib::cPacketStack packet(CSNet::E_PROTOCOL::E_TCP);

#if defined(PACKET_ANALYZE_ON)
			packet.Make(nCommand, pData, nLength, pContext->GetSendPacketCnt());
#else
			packet.MakeManualEncrypt(nCommand, pData, nLength, pContext->GetSendPacketCnt());
#endif
			E_ERROR_SEND error = pContext->SendRequest(packet.GetBuffer(), packet.GetLength());
			if (error == E_ERROR_SEND_OK)
				++sended_count;

			pPair->m_value->GetNext(pos);
		}
	}
	else
	{
		cCSLock Lock(&m_ConnectionLocks);

		POSITION pos = m_connectedServerMaps[eServertype].GetStartPosition();

		while (pos)
		{
			ATL::CAtlMap<__int64, ConnectorInfo*>::CPair* pPair = m_connectedServerMaps[eServertype].GetAt(pos);

			ConnectorInfo* info = pPair->m_value;

			NetLib::cIocpContext* pContext = reinterpret_cast<NetLib::cIocpContext*>(info->pServerContext);

			if (pContext != nullptr)
			{
				NetLib::cPacketStack packet(CSNet::E_PROTOCOL::E_TCP);

#if defined(PACKET_ANALYZE_ON)
				packet.Make(nCommand, pData, nLength, pContext->GetSendPacketCnt());
#else
				packet.MakeManualEncrypt(nCommand, pData, nLength, pContext->GetSendPacketCnt());
#endif
				E_ERROR_SEND error = pContext->SendRequest(packet.GetBuffer(), packet.GetLength());
				if (error == E_ERROR_SEND_OK)
					++sended_count;
			}

			m_connectedServerMaps[eServertype].GetNext(pos);

		}
	}

	return sended_count > 0 ? E_ERROR_SEND::E_ERROR_SEND_OK : E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET;

}

// 자신에게 접속한 서버들에게 전송한다.
E_ERROR_SEND NetLib::cIocpConnector::BroadCastToConnectors(E_SERVER_TYPE eServertype, UINT nCommand, BYTE* pData, UINT nLength)
{
	int sended_count = 0;

	cCSLock Lock(&m_ConnectionLocks);

	POSITION pos = m_connectedServerMaps[eServertype].GetStartPosition();

	while (pos)
	{
		ATL::CAtlMap<__int64, ConnectorInfo*>::CPair* pPair = m_connectedServerMaps[eServertype].GetAt(pos);

		ConnectorInfo* info = pPair->m_value;

		NetLib::cIocpContext* pContext = reinterpret_cast<NetLib::cIocpContext*>(info->pServerContext);

		if (pContext != nullptr)
		{
			NetLib::cPacketStack packet(CSNet::E_PROTOCOL::E_TCP);

#if defined(PACKET_ANALYZE_ON)
			packet.Make(nCommand, pData, nLength, pContext->GetSendPacketCnt());
#else
			packet.MakeManualEncrypt(nCommand, pData, nLength, pContext->GetSendPacketCnt());
#endif

			E_ERROR_SEND error = pContext->SendRequest(packet.GetBuffer(), packet.GetLength());
			if (error == E_ERROR_SEND_OK)
				++sended_count;
		}

		m_connectedServerMaps[eServertype].GetNext(pos);

	}

	return sended_count > 0 ? E_ERROR_SEND::E_ERROR_SEND_OK : E_ERROR_SEND::E_ERROR_SEND_FAILD_NO_TARGET;

}

E_ERROR_SEND NetLib::cIocpConnector::TargetSendPacket(E_SERVER_TYPE eServertype, NetLib::cIocpContext* pContext, UINT nCommand, BYTE* pData, UINT nLength)
{
	if (pContext == nullptr)
	{
		return E_ERROR_SEND::E_ERROR_SEND_ERROR;
	}

	ATL::CAtlMap<E_SERVER_TYPE, ATL::CAtlMap<int64, NetLib::cIocpContext*>*>::CPair* pPair = m_myConnectorMap.Lookup(eServertype);
	if (pPair == nullptr)
	{
		return E_ERROR_SEND::E_ERROR_SEND_ERROR;
	}

	int64 key = 0;
	uint32 uiIP = 0;
	inet_pton(AF_INET, pContext->GetConnectorTargetIP(), &uiIP);
	uint32 uiPORT = static_cast<uint32>(pContext->GetConnectorTargetPort());
	MakeServerMapKey(key, uiIP, uiPORT);

	ATL::CAtlMap<int64, NetLib::cIocpContext*>::CPair* pPair1 = pPair->m_value->Lookup(key);
	if (pPair1 == nullptr)
	{
		return E_ERROR_SEND::E_ERROR_SEND_ERROR;
	}

	NetLib::cIocpContext* pIocpContext = pPair1->m_value;
	if (pIocpContext == nullptr)
	{
		E_ERROR_SEND::E_ERROR_SEND_ERROR;
	}

	NetLib::cPacketStack packet(CSNet::E_PROTOCOL::E_TCP);

#if defined(PACKET_ANALYZE_ON)
	packet.Make(nCommand, pData, nLength, pIocpContext->GetSendPacketCnt());
#else
	packet.MakeManualEncrypt(nCommand, pData, nLength, pIocpContext->GetSendPacketCnt());
#endif

	E_ERROR_SEND error = pIocpContext->SendRequest(packet.GetBuffer(), packet.GetLength());

	return error;
}

BOOL NetLib::cIocpConnector::CheckLiveAndSendPacket(E_SERVER_TYPE eServertype, UINT nCommand, BYTE* pData, UINT nLength)
{
	ATL::CAtlMap<E_SERVER_TYPE, ATL::CAtlMap<int64, NetLib::cIocpContext*>*>::CPair* pPair = m_myConnectorMap.Lookup(eServertype);
	if(pPair == nullptr)
	{
		return FALSE;
	}

	if(pPair->m_value->GetCount() == 0)
	{
		return FALSE;
	}

	NetLib::cPacketStack packet(CSNet::E_PROTOCOL::E_TCP);

	POSITION pos = pPair->m_value->GetStartPosition();
	while(pos)
	{
		NetLib::cIocpContext* pContext = pPair->m_value->GetValueAt(pos);
		if(pContext == nullptr)
		{
			return FALSE;
		}

		if(pContext->GetConnectorStatus() != E_IOCP_CONNECTOR_STATUS::E_IOCP_CONNECTOR_STATUS_CONNECTED)
		{
			return FALSE;
		}
#if defined(PACKET_ANALYZE_ON)
		packet.Make(nCommand, pData, nLength, pContext->GetSendPacketCnt());
#else
		packet.MakeManualEncrypt(nCommand, pData, nLength, (*iter)->GetSendPacketCnt());
#endif

		pContext->SendRequest(packet.GetBuffer(), packet.GetLength());

		pPair->m_value->GetNext(pos);
	}

	return TRUE;
}


//
void NetLib::cIocpConnector::MakeServerMapKey(int64& key, TCHAR* IP, UINT PORT)
{
	key = PORT;
	key = key << 32;
	UINT ip_data = _tinet_addr(IP);
	key = key + ip_data;
}

void NetLib::cIocpConnector::MakeServerMapKey(int64& key, uint32 IP, uint32 PORT)
{
	key = IP;
	key = key << 32 | PORT;
}

ConnectorInfo* NetLib::cIocpConnector::RegisterServer(const BYTE servertype, TCHAR* IP, const std::string& public_ip , UINT PORT, UINT REMOTEPORT, NetLib::cInterfaceIocpContext* pContext, int ServerGroupID, const int serverId, int64& allocatedslot)
{
	if((servertype <= E_SERVER_TYPE::BASE_SERVER) || (servertype >= E_SERVER_TYPE::SERVER_TYPE_MAX)) {
		return nullptr;
	}

	MakeServerMapKey(allocatedslot, IP, PORT);

	cCSLock Lock(&m_ConnectionLocks);

	ConnectorInfo* info = nullptr;

	// 없으면 생성하고, 있으면 갱신한다.
	ATL::CAtlMap<__int64, ConnectorInfo*>::CPair* pPair = m_connectedServerMaps[servertype].Lookup(allocatedslot);
	if(pPair == nullptr)
	{
		info = new ConnectorInfo();
		_tcscpy_s(info->szIP, IP);
		info->nPort = PORT;
		info->nRemotePort = REMOTEPORT;
		info->nServerType = static_cast<int>(servertype);
		info->ServerGroupID = ServerGroupID;
		info->pServerContext = pContext;
		info->uiSessionCnt = 0;
		info->uiMaxSessionCnt = 0;
		info->allocatedslot = allocatedslot;
		info->serverTypeString = G_SERVERNAME[servertype];
		info->serverId = serverId;
		info->publicIpAddress = public_ip;

		// std::string으로 변환하여 ipAddress에 할당
#ifdef UNICODE
	// TCHAR가 wchar_t일 경우 (유니코드)
		std::wstring ws(info->szIP);
		info->ipAddress = std::string(ws.begin(), ws.end());
#else
	// TCHAR가 char일 경우 (멀티바이트)
		info->ipAddress = std::string(info->szIP);
#endif

		m_connectedServerMaps[servertype].SetAt(allocatedslot, info);


		pContext->SetAllocateSlot(allocatedslot);

		switch (servertype)
		{
		case E_SERVER_TYPE::LOBBY_SERVER:
			m_LobbyServers.insert(std::pair< int64, ConnectorInfo* >(allocatedslot, info));
			break;
		case E_SERVER_TYPE::SLOT_SERVER:
			m_SlotServers.insert(std::pair< int64, ConnectorInfo* >(allocatedslot, info));
			break;
		}
	}
	else
	{
		info = pPair->m_value;
		_tcscpy_s(info->szIP, IP);
		info->nPort = PORT;
		info->nRemotePort = REMOTEPORT;
		info->nServerType = static_cast<int>(servertype);
		info->ServerGroupID = ServerGroupID;
		info->pServerContext = pContext;
		info->uiSessionCnt = 0;
		info->uiMaxSessionCnt = 0;
		info->allocatedslot = allocatedslot;
		info->serverTypeString = G_SERVERNAME[servertype];
		info->serverId = serverId;

		// std::string으로 변환하여 ipAddress에 할당
#ifdef UNICODE
	// TCHAR가 wchar_t일 경우 (유니코드)
		std::wstring ws(info->szIP);
		info->ipAddress = std::string(ws.begin(), ws.end());
#else
	// TCHAR가 char일 경우 (멀티바이트)
		info->ipAddress = std::string(info->szIP);
#endif

		m_connectedServerMaps[servertype].SetAt(allocatedslot, info);

		pContext->SetAllocateSlot(allocatedslot);

		switch (servertype)
		{
		case E_SERVER_TYPE::LOBBY_SERVER:
		{
			auto iter = m_LobbyServers.find(allocatedslot);
			if (iter != m_LobbyServers.end()) {
				// 특별히 할일이 없음
			}
		}
		break;
		case E_SERVER_TYPE::SLOT_SERVER:
			auto iter = m_SlotServers.find(allocatedslot);
			if (iter != m_SlotServers.end()) {
				// 특별히 할일이 없음
			}
			break;
		}
	}
	
	return info;
}

BOOL NetLib::cIocpConnector::DeRegisterServer(const BYTE servertype, const int64 allocatedslot)
{
	if((servertype <= E_SERVER_TYPE::BASE_SERVER) || (servertype >= E_SERVER_TYPE::SERVER_TYPE_MAX))
	{
		return FALSE;
	}

	cCSLock Lock(&m_ConnectionLocks);

	ATL::CAtlMap<__int64, ConnectorInfo*>::CPair* pPair = m_connectedServerMaps[servertype].Lookup(allocatedslot);
	if(pPair == nullptr)
	{
		return FALSE;
	}

	ConnectorInfo* info = pPair->m_value;
	
	m_connectedServerMaps[servertype].RemoveKey(allocatedslot);

	switch (servertype)
	{
	case E_SERVER_TYPE::LOBBY_SERVER:
	{
		auto iter = m_LobbyServers.find(allocatedslot);
		if (iter != m_LobbyServers.end())
			m_LobbyServers.erase(iter);
	}
	break;
	case E_SERVER_TYPE::SLOT_SERVER:
	{
		auto iter = m_SlotServers.find(allocatedslot);
		if (iter != m_SlotServers.end())
			m_SlotServers.erase(iter);
	}
	break;
	}

	if(info)
	{
		delete info;
	}

	return TRUE;
}

ConnectorInfo* NetLib::cIocpConnector::GetServerInfo(const BYTE servertype, const int serverId)
{
	if ((servertype <= E_SERVER_TYPE::BASE_SERVER) || (servertype >= E_SERVER_TYPE::SERVER_TYPE_MAX))
	{
		return nullptr;
	}

	cCSLock Lock(&m_ConnectionLocks);

	ConnectorInfo* info = nullptr;

	switch (servertype)
	{
	case E_SERVER_TYPE::LOBBY_SERVER:
	{
		for (auto mapPair : m_LobbyServers) {
			info = mapPair.second;
			if (info == nullptr) continue;

			if (info->serverId == serverId)
				return info;
		}
	}
	break;
	case E_SERVER_TYPE::SLOT_SERVER:
	{
		for (auto mapPair : m_SlotServers) {
			info = mapPair.second;
			if (info == nullptr) continue;

			if (info->serverId == serverId)
				return info;
		}
	}
	break;
	}

	return info;
}

ConnectorInfo* NetLib::cIocpConnector::GetServerInfoByAllocatedSlot(const BYTE servertype, int64 allocatedslot)
{
	if((servertype <= E_SERVER_TYPE::BASE_SERVER) || (servertype >= E_SERVER_TYPE::SERVER_TYPE_MAX))
	{
		return nullptr;
	}

	cCSLock Lock(&m_ConnectionLocks);

	ATL::CAtlMap<__int64, ConnectorInfo*>::CPair* pPair = m_connectedServerMaps[servertype].Lookup(allocatedslot);
	if (pPair == nullptr)
	{
		return nullptr;
	}

	return pPair->m_value;
}

E_ERROR_SEND NetLib::cIocpConnector::ConnectionALLSendPacket(const E_SERVER_TYPE servertype, UINT nCommand, BYTE* pData, UINT nLength)
{
	if ((servertype <= E_SERVER_TYPE::BASE_SERVER) || (servertype >= E_SERVER_TYPE::SERVER_TYPE_MAX))
	{
		return E_ERROR_SEND::E_ERROR_SEND_ERROR;
	}
	cCSLock Lock(&m_ConnectionLocks);

	POSITION pos = m_connectedServerMaps[servertype].GetStartPosition();
	while (pos)
	{
		ConnectorInfo* pInfo = m_connectedServerMaps[servertype].GetValueAt(pos);
		if (pInfo == nullptr)
		{
			return E_ERROR_SEND::E_ERROR_SEND_ERROR;
		}

		NetLib::cIocpContext* pContext = reinterpret_cast<NetLib::cIocpContext*>(pInfo->pServerContext);
		if (pContext == nullptr)
		{
			return E_ERROR_SEND::E_ERROR_SEND_ERROR;
		}

		if (pContext->IsActive() == false)
		{
			char szIP[CSDef::EDef::MAX_IP_ADDRESS_LEN] = { 0, };
			::ConvertIP(szIP, sizeof(szIP), pContext->GetIP());
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI,
				"NetLib::cIocpConnector::ConnectionALLSendPacket Context Active false IP[ %s ] PORT[ %d ]",
				szIP,
				pContext->GetPORT());

			return E_ERROR_SEND::E_ERROR_SEND_ERROR;
		}

		m_connectedServerMaps[servertype].GetNext(pos);
	}

	pos = m_connectedServerMaps[servertype].GetStartPosition();
	while (pos)
	{
		ConnectorInfo* pInfo = m_connectedServerMaps[servertype].GetValueAt(pos);
		if (pInfo != nullptr)
		{
			NetLib::cIocpContext* pContext = reinterpret_cast<NetLib::cIocpContext*>(pInfo->pServerContext);
			if (pContext != nullptr)
			{
				if (pContext->IsActive() == true)
				{
					NetLib::cPacketStack packet(CSNet::E_PROTOCOL::E_TCP);

#if defined(PACKET_ANALYZE_ON)
					packet.Make(nCommand, pData, nLength, pContext->GetSendPacketCnt());
#else
					packet.MakeManualEncrypt(nCommand, pData, nLength, pContext->GetSendPacketCnt());
#endif

					E_ERROR_SEND error = pContext->SendRequest(packet.GetBuffer(), packet.GetLength());

					if (E_ERROR_SEND::E_ERROR_SEND_OK != error)
					{
						return error;
					}
				}
			}
		}

		m_connectedServerMaps[servertype].GetNext(pos);
	}

	return E_ERROR_SEND::E_ERROR_SEND_OK;
}

NetLib::cIocpContext* NetLib::cIocpConnector::GetServerContext(E_SERVER_TYPE eType, int64 ServerKey)
{
	ATL::CAtlMap<E_SERVER_TYPE, ATL::CAtlMap<int64, NetLib::cIocpContext*>*>::CPair* pPair = m_myConnectorMap.Lookup(eType);

	if (pPair != nullptr)
	{
		ATL::CAtlMap<int64, NetLib::cIocpContext*>::CPair* pServerPair = pPair->m_value->Lookup(ServerKey);
		if (pServerPair != nullptr)
		{
			return pServerPair->m_value;
		}
	}

	if (E_SERVER_TYPE::NONE_SERVER >= eType ||
		eType <= E_SERVER_TYPE::SERVER_TYPE_MAX)
	{
		return nullptr;
	}

	cCSLock Lock(&m_ConnectionLocks);

	ATL::CAtlMap<int64, ConnectorInfo*>::CPair* pConnectorPair = m_connectedServerMaps[eType].Lookup(ServerKey);
	if (pConnectorPair == nullptr)
	{
		return nullptr;
	}

	if (pConnectorPair->m_value->pServerContext != nullptr)
	{
		return reinterpret_cast<NetLib::cIocpContext*>(pConnectorPair->m_value->pServerContext);
	}

	return nullptr;
}