#include "../../Include/Netlib/IOCP/cIocpContext.h"
#include "../../Include/Netlib/Common/cHeader.h"
#include "../../Include/Netlib/Network/cPacketStack.h"
#include "../../Include/Netlib/Queue/cLogQueue.h"
#include "../../Include/Netlib/Common/cSingleton.h"
#ifdef USING_MULTI_THREAD
#include "../../Include/Netlib/Manager/cCommandQueueManager.h"
#endif

int NetLib::cIocpContext::m_uiEntityNum = 0;

NetLib::cIocpContext::cIocpContext() :
	m_bActive(false),
	m_bAssociated(false),
	m_bConnector(false),
	m_uiSequence(0),
	m_uiEntity(++m_uiEntityNum),
	m_eContextStatus(E_CONNECT_STATUS::E_CONNECT_NONE),
	m_i64AllocateSlot(0)
{
	Init();
}

NetLib::cIocpContext::cIocpContext(bool bConnector, const char* TargetIP, UINT Port) :
	cSocket(),
	m_bActive(false),
	m_bAssociated(false),
	m_bConnector(false),
	m_uiSequence(0),
	m_uiEntity(++m_uiEntityNum),
	m_bCrypt(true),
	m_i64AllocateSlot(0)
{
	Init(bConnector);
	strcpy_s(m_strConnectorIP, CSDef::EDef::MAX_IP_ADDRESS_LEN, TargetIP);
	m_uiConnectorPort = Port;
}

NetLib::cIocpContext::~cIocpContext()
{
	Destroy();
}

void NetLib::cIocpContext::Init(bool bConnector)
{
	CreateStorage(G_NET_BUFFER_SIZE_32K);
	m_bConnector = bConnector;

	m_bEnterContextTable = false;

	m_pos = nullptr;	//	Context Pooler Position 초기화

	// cInterfaceIocpContext의 Session정보 초기화.
	cInterfaceIocpContext::m_pSession = NULL;

	m_eContextStatus = E_CONNECT_STATUS::E_CONNECT_NONE;
	m_eContextType = m_bConnector ? E_CONTEXT_TYPE::E_CONTEXT_SERVER : E_CONTEXT_TYPE::E_CONTEXT_CLIENT;

	SetCrypt(true);

	//m_llUniquekey.UniqueNumber = 0;
	//m_llUniquekey.ContextKey.InstanceNumber = static_cast<unsigned short>(m_uiEntity);

	m_llUniquekey = 0;

	m_i64AllocateSlot = 0;

	m_byEncryptKey = (rand() % 256) + 1;
	m_byDecryptKey = (rand() % 256) + 1;

	m_bDamagePacketCapture = FALSE;

#ifdef USING_MULTI_THREAD
	m_uRoomNum = 0;
	m_uCommandQueueIndex = 0;
#endif

	m_ullPendingDelayIOS = 0;
}

void NetLib::cIocpContext::Destroy()
{
	DestroyStorage();
}

void NetLib::cIocpContext::Reset(DWORD lNumberOfBytesTransferred)
{
	NetLib::cInterfaceIocpContext::m_pSession = NULL;
	m_bClosing = false;
	SetActive(true);
	SetCrypt(true);
	SetSequence(0);
	CleanStorage();

	if(!m_bConnector)
	{
		GetPeerAddress(lNumberOfBytesTransferred, NetLib::cSocket::IsUseIPv6());
	}

	m_bDamagePacketCapture = FALSE;
	for (size_t n = 0; n<m_DamagePackets.size(); ++n)
	{
		BYTE* pBYTE = m_DamagePackets[n];
		if(pBYTE)
			delete pBYTE;
	}
	m_DamagePackets.clear();

#if defined(VIRTUAL_NAGLE_ON)
	cSocket::m_pSendIocpOv = NULL;
#endif
#ifdef USING_MULTI_THREAD
	m_uRoomNum = 0;
	m_uCommandQueueIndex = 0;
#endif

	m_ullPendingDelayIOS = 0;
}

void NetLib::cIocpContext::Disconnect(E_IO_OPERATION eOperation)
{
	if(IsActive())
	{
		SetActive(false);
		cSocket::Disconnect(eOperation);
	}
}

void NetLib::cIocpContext::ForceDisconnect()
{
	SetActive(false);
	NetLib::cSocket::ReCreateSocket();
}

void NetLib::cIocpContext::SetUniqueKey()
{
	m_llUniquekey = ::GetTickCount64();
	/*ULONGLONG TickCount = ::GetTickCount64();
	unsigned short nNewTimeCount = static_cast<unsigned short>(TickCount / TenMinute);
	if (m_llUniquekey.TimeKey.TimeCount < nNewTimeCount)
	{
		m_llUniquekey.TimeKey.ChunkNumber = 0;
	}

	if ((nNewTimeCount % 6) == 0 && m_llUniquekey.TimeKey.ChunkNumber == 0)
	{
		m_llUniquekey.ContextKey.Empty = 0;
	}

	m_llUniquekey.TimeKey.TimeCount = nNewTimeCount;
	++m_llUniquekey.TimeKey.ChunkNumber;
	++m_llUniquekey.ContextKey.Empty;*/
}

void NetLib::cIocpContext::SetConnectorInfo(const std::string& Ip, UINT Port)
{ 
	strcpy_s(m_strConnectorIP, CSDef::EDef::MAX_IP_ADDRESS_LEN, Ip.c_str());
	m_uiConnectorPort = Port; 
	m_bConnector = true; 
	SetContextType(E_CONTEXT_TYPE::E_CONTEXT_SERVER); 
	inet_pton(AF_INET, m_strConnectorIP, &m_nIP);

	m_nConnectorIPHint = m_nIP >> 0 & 255;

}

ULONGLONG NetLib::cIocpContext::GetUniqueKey()
{
	return m_llUniquekey;
	//return m_llUniquekey.UniqueNumber;
}

ULONGLONG NetLib::cIocpContext::GetUniqueKey() const
{
	return m_llUniquekey;
}

E_ERROR_SEND NetLib::cIocpContext::SendRequest(BYTE* pData, UINT uiDataSize)
{
#if defined(PROTOCOL_TRACE_ON) &&  defined(_DEBUG)
	ServerManager::WriteProtocolCommandString(*((UINT*)pData), false, GetPeerPort(), "cSocket");
#endif
	// 패킷 시퀀스 부여
	NetLib::cHeader* pHeader = reinterpret_cast<NetLib::cHeader*>(pData);
	pHeader->SetPacketSequence(GetSequence());

	// 패킷 데미지 로그를 분석하여 데이터 저장해 놓는다. 접속 종료시까지 사이즈가 계속 증가 된다 주의
	/*if(m_bDamagePacketCapture)
	{
		switch (pHeader->GetCommand())
		{
		case CSNet::SC_HIT:
		case CSNet::SC_DOT_HP:
		case CSNet::SC_DOT_DAMAGE:
		case CSNet::SC_NOTI_MAX_HP:
		case CSNet::SC_DIE:
		case CSNet::SC_DEATH:
		{
			BYTE* pInputData = new BYTE[uiDataSize];
			memcpy(pInputData, pData, uiDataSize);
			m_DamagePackets.push_back(pInputData);
		}
		break;
		}
	}*/

	// 전송
	return NetLib::cSocket::SendRequest(pData, uiDataSize);
}

#ifdef USING_MULTI_THREAD
//void NetLib::cIocpContext::SetContextRoom(UINT uRoomNum)
//{
//	m_uRoomNum = uRoomNum;
//
//	int nCreatedCommandQueueCnt = NetLib::cSingleton<NetLib::cCommandQueueManager>::GetInstance()->GetCommandQueueCnt();
//	Assert(nCreatedCommandQueueCnt >= 1, _T("cIocpContext::SetContextRoom()"));
//
//	m_uCommandQueueIndex = m_uRoomNum % nCreatedCommandQueueCnt;
//
//	//if(uRoomNum == 0)
//	//{
//	//	m_uCommandQueueIndex = nCreatedCommandQueueCnt - 1;// array 강제 선택
//	//}
//	//else
//	//{
//	//	int nPerRoomCnt = uMaxRoom / nCreatedCommandQueueCnt;
//	//	m_uCommandQueueIndex = uRoomNum / nPerRoomCnt;// 5개일 경우 0 1 2 3 4
//	//												  //m_uCommandQueueIndex = uRoomNum % nCreatedCommandQueueCnt;// 5개일 경우 0 1 2 3 4
//	//	if(m_uCommandQueueIndex >= (UINT)nCreatedCommandQueueCnt)
//	//		m_uCommandQueueIndex = nCreatedCommandQueueCnt - 1;
//	//}
//
//#ifdef _DEBUG
//	NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("SetContextRoom() Room[%d] CmdQIdx(%d), CmdQCnt(%d)")
//		, uRoomNum, m_uCommandQueueIndex, nCreatedCommandQueueCnt);
//#endif
//}

// Client일경우에 Default CommandIndex를 부여한다.
// Room에 진입하기 전까지는 각기 다른 쓰레드에서 처리된다.
void NetLib::cIocpContext::SetDefaultCommandQueueIndex()
{
	int nCreatedCommandQueueCnt = NetLib::cSingleton<NetLib::cCommandQueueManager>::GetInstance()->GetCommandQueueCnt();
	Assert(nCreatedCommandQueueCnt >= 1, _T("cIocpContext::SetDefaultCommandQueueIndex()"));

	m_uCommandQueueIndex = GetEntity() % nCreatedCommandQueueCnt;
}
#endif

void NetLib::cIocpContext::DestroyAccept()
{
	m_olAccept.Destroy();
	m_olAccept.Init(1024);
}

void NetLib::cIocpContext::PrintDamangePacketCapture()
{
	/*if(m_DamagePackets.size() == 0)
		return;

	cPacketStack packet(CSDef::E_PROTOCOL::E_TCP);

	int nHeaderSize = sizeof(cHeader);

	cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("PrintCollectingPackets Start Entity %u"), GetEntity());
	for (int n = 0; n<m_DamagePackets.size(); ++n)
	{
		BYTE* pData = m_DamagePackets[n];
		if(pData == nullptr)
			continue;

		packet.Clear();

		cHeader* pHeader = reinterpret_cast<cHeader*>(pData);
		switch (pHeader->GetCommand())
		{
		case CSNet::SC_HIT:
		{
			packet.CopyPacket(pData, 16 + sizeof(scHit));
			packet.Decrypt();
			scHit* pHit = reinterpret_cast<scHit*>(packet.GetBody());
			if(pHit->targetEntity == GetEntity())
				cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("SEQ %u SC_HIT damage %f, SENDER ENTITY %u"), pHeader->GetPacketNum(), pHit->damage, pHit->attackerEntity);
		}
		break;
		case CSNet::SC_DOT_HP:
		{
			packet.CopyPacket(pData, 16 + sizeof(scDotHp));
			packet.Decrypt();
			scDotHp* pHit = reinterpret_cast<scDotHp*>(packet.GetBody());
			if(pHit->Entity == GetEntity())
				cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("SEQ %u SC_DOT_HP recover %d"), pHeader->GetPacketNum(), pHit->Hp);
		}
		break;
		case CSNet::SC_DOT_DAMAGE:
		{
			packet.CopyPacket(pData, 16 + sizeof(scDotDamage));
			packet.Decrypt();
			scDotDamage* pHit = reinterpret_cast<scDotDamage*>(packet.GetBody());
			if(pHit->targetEntity == GetEntity())
				cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("SEQ %u SC_DOT_DAMAGE damage %d"), pHeader->GetPacketNum(), pHit->damage);
		}
		break;
		case CSNet::SC_NOTI_MAX_HP:
		{
			packet.CopyPacket(pData, 16 + sizeof(scNotiMaxHP));
			packet.Decrypt();
			scNotiMaxHP* pHit = reinterpret_cast<scNotiMaxHP*>(packet.GetBody());
			if(pHit->TargetEntity == GetEntity())
				cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("SEQ %u SC_NOTI_MAX_HP CurHP %d"), pHeader->GetPacketNum(), pHit->CurHP);
		}
		break;
		case CSNet::SC_DIE:
		{
			packet.CopyPacket(pData, 16 + sizeof(scDie));
			packet.Decrypt();
			scDie* pHit = reinterpret_cast<scDie*>(packet.GetBody());
			if(pHit->Entity == GetEntity())
				cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("SEQ %u SC_DIE"), pHeader->GetPacketNum());
		}
		break;
		case CSNet::SC_DEATH:
		{
			packet.CopyPacket(pData, 16 + sizeof(scDeath));
			packet.Decrypt();
			scDeath* pHit = reinterpret_cast<scDeath*>(packet.GetBody());
			if(pHit->Entity == GetEntity())
				cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("SEQ %u SC_DEATH"), pHeader->GetPacketNum());
		}
		break;
		}
	}

	cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("PrintCollectingPackets End Entity %u"), GetEntity());

	for (int n = 0; n<m_DamagePackets.size(); ++n)
	{
		BYTE* pBYTE = m_DamagePackets[n];
		if(pBYTE)
			delete pBYTE;
	}
	m_DamagePackets.clear();*/
}