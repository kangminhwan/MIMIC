#include "../../Include/Netlib/Network/cDisPatcher.h"
#include "../../Include/Netlib/Network/cPacketStack.h"
#include "../../Include/Netlib/Manager/cSessionManager.h"
#include "../../Include/Netlib/Queue/cLogQueue.h"
#include "../../Include/Netlib/IOCP/cIocpContext.h"
#include "../../Include/Netlib/Queue/cCommandQueue.h"
#include "../../Include/Netlib/Session/cSession.h"
#include "../../Include/Netlib/Common/cSingleton.h"
#ifdef USING_MULTI_THREAD
#include "../../Include/Netlib/Manager/cCommandQueueManager.h"
#endif

NetLib::cDisPatcher::cDisPatcher() :
	m_pSessionManager(nullptr)
{
	Init();
}


NetLib::cDisPatcher::~cDisPatcher()
{
	Destroy();
}

void NetLib::cDisPatcher::Init()
{
	m_pSessionManager = NetLib::cSingleton<NetLib::cSessionManager>::ExistsInstance();
	m_pPacketStack = new NetLib::cPacketStack( CSNet::E_PROTOCOL::E_TCP );
}

void NetLib::cDisPatcher::Destroy()
{
	m_pSessionManager = nullptr;
	delete m_pPacketStack;
}

void NetLib::cDisPatcher::SendPacket( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , BYTE* pData , UINT nLength )
{
	if ( !pContext ) {
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_CRI , _T( "cDisPatcher::SendPacket NULL Context requests command %d" ) , nCommand );
		return;
	}

	m_pPacketStack->Clear();

#if defined(PACKET_ANALYZE_ON)
	if ( pContext->IsCrypt() )
		m_pPacketStack->Make( nCommand , pData , nLength , pContext->GetSendPacketCnt() );
	else
		m_pPacketStack->MakeManualEncrypt( nCommand , pData , nLength , pContext->GetSendPacketCnt() );
#else
	if ( pContext->IsCrypt() )
		m_pPacketStack->Make( nCommand , pData , nLength , 0 );
	else
		m_pPacketStack->MakeManualEncrypt( nCommand , pData , nLength , 0 );
#endif

	// SendRequest가 에러가 발생되면
	// 접속을 종료시킴
	WORD wResult = pContext->SendRequest( m_pPacketStack->GetBuffer() , m_pPacketStack->GetLength() );

	if ( wResult != E_ERROR_SEND_OK )
	{
		switch ( wResult )
		{
		case E_ERROR_SEND_SEND_POOL_EMPTY:
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_INFO, _T( "SendPacket error POOL_EMPTY Entity[%d]" ) , pContext->GetEntity() );
			break;
		case E_ERROR_SEND_DATA_SIZE_OVER:
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_INFO, _T( "SendPacket error DATA_SIZE_OVER Entity[%d]" ) , pContext->GetEntity() );
			break;
			/*case E_ERROR_SEND_WOULDBLOCK:
			cSingleton<cLogQueue>::GetInstance()->PushCommand( LOG_INFO, _T("SendPacket error EWOULDBLOCK Entity[%d"), pContext->GetEntity() );
			break;*/
		case E_ERROR_SEND_SOCKET_ERROR:
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand( LOG_INFO , _T( "SendPacket error SOCKET_ERROR Entity[%d]" ) , pContext->GetEntity() );
			break;
		default:
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T( "SendPacket error ????? Entity[%d]" ) , pContext->GetEntity() );
			break;
		}

		// 문제 생긴 클라이언트 종료시켜준다.
		// Session 처리를 위해 DISCONNECT 이벤트 발생
#ifdef USING_MULTI_THREAD
		NetLib::cSingleton<NetLib::cCommandQueueManager>::GetInstance()->PushCommand( reinterpret_cast< NetLib::cIocpContext* >( pContext ) , CSNet::SYS_NET_DISCONNECT );
#else
		NetLib::cSingleton<NetLib::cCommandQueue>::GetInstance()->PushCommand( reinterpret_cast< NetLib::cIocpContext* >( pContext ) , CSNet::SYS_NET_DISCONNECT );
#endif

		// 소켓재활용
		//pContext->Disconnect(E_IO_FORCE_DISCONNECT);
		//m_pSessionManager->Remove(pContext->GetEntity());
	}
	return;
}

//void NetLib::cDisPatcher::SendPacket(NetLib::cInterfaceIocpContext* pContext, UINT nCommand, BYTE* pData, UINT nLength)
//{
//	if(!pContext)
//	{
//		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("cDisPatcher::SendPacket NULL Context requests command %d"), nCommand);
//		return;
//	}
//	// Packet merge GetSendPacketCnt() 
//	//#if defined(PACKET_ANALYZE_ON)
//	//		m_cPacket->MakeManualEncrypt( nCommand, pData, nLength, pContext->GetSendPacketCnt());
//	//#else
//	//		m_cPacket->MakeManualEncrypt( nCommand, pData, nLength, 0);
//	//#endif
//	//		BYTE before[10000], after[10000];
//	//		memset(before, 0x00, sizeof(before));
//	//		memset(after, 0x00, sizeof(after));
//	//		cHeader* cHeaderBefore = reinterpret_cast<cHeader*>(m_cPacket->GetBuffer());
//	//		memcpy(before, m_cPacket->GetBuffer(), m_cPacket->GetLength());
//	//		m_cPacket->Encrypt();
//	//		cHeader* cHeaderafter = reinterpret_cast<cHeader*>(m_cPacket->GetBuffer());
//	//		memcpy(after, m_cPacket->GetBuffer(), m_cPacket->GetLength());
//
//	NetLib::cPacketStack packet(CSNet::E_PROTOCOL::E_TCP);
//
//#if defined(PACKET_ANALYZE_ON)
//	if(pContext->IsCrypt())
//		packet.Make(nCommand, pData, nLength, pContext->GetSendPacketCnt());
//	else
//		packet.MakeManualEncrypt(nCommand, pData, nLength, pContext->GetSendPacketCnt());
//#else
//	if(pContext->IsCrypt())
//		packet.Make(nCommand, pData, nLength, 0);
//	else
//		packet.MakeManualEncrypt(nCommand, pData, nLength, 0);
//#endif
//
//	// SendRequest가 에러가 발생되면
//	// 접속을 종료시킴
//	WORD wResult = pContext->SendRequest(packet.GetBuffer(), packet.GetLength());
//
//	// 지렁이 테스트
//	/*WORD wResult = pContext->SendRequest( m_cPacket->GetBuffer(), 4 );
//	wResult = pContext->SendRequest( m_cPacket->GetBuffer() + 4, 8 );
//	wResult = pContext->SendRequest( m_cPacket->GetBuffer() + 12, m_cPacket->GetLength() - 12 );*/
//
//	if(wResult != E_ERROR_SEND_OK)
//	{
//		switch (wResult)
//		{
//		case E_ERROR_SEND_SEND_POOL_EMPTY:
//			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_INFO, _T("SendPacket error POOL_EMPTY Entity[%d]"), pContext->GetEntity());
//			break;
//		case E_ERROR_SEND_DATA_SIZE_OVER:
//			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_INFO, _T("SendPacket error DATA_SIZE_OVER Entity[%d]"), pContext->GetEntity());
//			break;
//			/*case E_ERROR_SEND_WOULDBLOCK:
//			cSingleton<cLogQueue>::GetInstance()->PushCommand( LOG_INFO, _T("SendPacket error EWOULDBLOCK Entity[%d"), pContext->GetEntity() );
//			break;*/
//		case E_ERROR_SEND_SOCKET_ERROR:
//			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_INFO, _T("SendPacket error SOCKET_ERROR Entity[%d]"), pContext->GetEntity());
//			break;
//		default:
//			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_INFO, _T("SendPacket error ????? Entity[%d]"), pContext->GetEntity());
//			break;
//		}
//
//		// 문제 생긴 클라이언트 종료시켜준다.
//		// Session 처리를 위해 DISCONNECT 이벤트 발생
//#ifdef USING_MULTI_THREAD
//		NetLib::cSingleton<NetLib::cCommandQueueManager>::GetInstance()->PushCommand(reinterpret_cast<NetLib::cIocpContext*>(pContext), CSNet::SYS_NET_DISCONNECT);
//#else
//		NetLib::cSingleton<NetLib::cCommandQueue>::GetInstance()->PushCommand(reinterpret_cast<NetLib::cIocpContext*>(pContext), CSNet::SYS_NET_DISCONNECT);
//#endif
//
//		// 소켓재활용
//		//pContext->Disconnect(E_IO_FORCE_DISCONNECT);
//		//m_pSessionManager->Remove(pContext->GetEntity());
//	}
//	return;
//}

bool NetLib::cDisPatcher::SendRequest(NetLib::cInterfaceIocpContext* pContext, const BYTE* pPacket, const UINT unLength)
{
	// SendRequest가 에러가 발생되면
	// 접속을 종료시킴
	WORD wResult = pContext->SendRequest(const_cast<BYTE*>(pPacket), unLength);

	if(wResult != E_ERROR_SEND_OK)
	{
		switch (wResult)
		{
		case E_ERROR_SEND_SEND_POOL_EMPTY:
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("SendRequest error POOL_EMPTY Entity[%d]"), pContext->GetEntity());
			break;
		case E_ERROR_SEND_DATA_SIZE_OVER:
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("SendRequest error DATA_SIZE_OVER Entity[%d]"), pContext->GetEntity());
			break;
			/*case E_ERROR_SEND_WOULDBLOCK:
			cSingleton<cLogQueue>::GetInstance()->PushCommand( LOG_INFO, _T("SendRequest error EWOULDBLOCK Entity[%d]"), pContext->GetEntity() );
			break;*/
		case E_ERROR_SEND_SOCKET_ERROR:
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("SendRequest error SOCKET_ERROR Entity[%d]"), pContext->GetEntity());
			break;
		default:
			NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("SendRequest error ????? Entity[%d]"), pContext->GetEntity());
			break;
		}

		//// 이곳에서 
		//// 소켓 에러는 종료시켜 주면서 소켓 close를 해줄것
		//if( wResult == E_ERROR_SEND_SOCKET_ERROR )
		//{
		//	m_SocketErrorSession.push(pContext);
		//}
		//// 소켓 에러 이외의 에러는 소켓을 재활용 한다.
		//else
		//{
		//	m_ErrorSession.push(pContext);
		//}

		// 소켓재활용을 위해 넣어둬..유..
		m_ErrorSession.push(pContext);

		return false;
	}
	return true;
}

// pContext 파라미터가 NULL이면 전체 브로드 캐스트
// 그외에는 pContext만 빼고  브로드 캐스트
//void cDisPatcher::BroadCast( cInterfaceIocpContext* pContext, UINT nCommand, BYTE* pData, UINT nLength )
//{
//	cPacketStack packet(CSDef::E_PROTOCOL::E_TCP);
//
//	// Packet merge
//	packet.Make( nCommand, pData, nLength, 0);
//
//	if( !m_pSessionManager ) 
//	{
//		cSingleton<cLogQueue>::GetInstance()->PushCommand( LOG_CRI, _T("cInstancePacketParser::BroadCast, SessionManager is NULL") );
//		return;
//	}
//
//	if(!m_pSessionManager->GetSessionCount())
//		return;	
//
//	// 문제가 생긴놈들을 저장하자. SendPacket처리뒤에 삭제해준다.
//	cSession* pSession = NULL;
//	for(int n=0; n<m_pSessionManager->m_SessionTable.size(); ++n)
//	{
//		pSession = m_pSessionManager->m_SessionTable[n];
//		if(pSession == nullptr)
//			continue;
//
//		// 자기자신을 제외한 브로드캐스팅, Entity값 비교
//		if( pSession->GetContext() != pContext )
//		{
//			SendRequest( pSession->GetContext(), packet.GetBuffer(), packet.GetLength() );
//		}
//	}
//
//	BroadCastErrorSessionClear();
//}

void NetLib::cDisPatcher::BroadCastToSessionsType(const Sessions  type, UINT nCommand, BYTE* pData, UINT nLength)
{
	NetLib::cPacketStack packet(CSNet::E_PROTOCOL::E_TCP);

	packet.Make(nCommand, pData, nLength, 0);

	if(!m_pSessionManager)
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("cInstancePacketParser::BroadCast, SessionManager is NULL"));
		return;
	}

	if(m_pSessionManager->GetSessionCount() < 1)
		return;

	// 문제가 생긴놈들을 저장하자. SendPacket처리뒤에 삭제해준다.	
	NetLib::cInterfaceIocpContext* pContext = NULL;
	NetLib::cSession* pSession = NULL;
	//for(int n=0; n<m_pSessionManager->m_SessionTable.size(); ++n)
	//{
	//	pSession = m_pSessionManager->m_SessionTable[n];
	//	if(pSession == nullptr)
	//		continue;

	//	//if(pSession->GetSessionType() != Sessions::SESSION_CLIENT)
	//	if(pSession->GetSessionType() == type && pSession->GetContext() )
	//	{
	//		pContext = pSession->GetContext();

	//		if(pContext && pContext->IsActive())
	//			SendRequest(pContext, packet.GetBuffer(), packet.GetLength());
	//	}
	//}

	BroadCastErrorSessionClear();
}

void NetLib::cDisPatcher::BroadCastToServerType(const E_SERVER_TYPE  type, UINT nCommand, BYTE* pData, UINT nLength)
{
	NetLib::cPacketStack packet(CSNet::E_PROTOCOL::E_TCP);

	packet.MakeManualEncrypt(nCommand, pData, nLength, 0);

	if(!m_pSessionManager)
	{
		NetLib::cSingleton<NetLib::cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("cDisPatcher::BroadCastToServerType, SessionManager is NULL"));
		return;
	}

	if(m_pSessionManager->GetSessionCount() < 1)
		return;

	ATL::CAtlMap<int64, NetLib::cSession*>* pSessionTable = m_pSessionManager->GetSessionTable();
	if(pSessionTable == nullptr)
		return;

	// 문제가 생긴놈들을 저장하자. SendPacket처리뒤에 삭제해준다.	
	NetLib::cInterfaceIocpContext* pContext = NULL;
	NetLib::cSession* pSession = NULL;
	POSITION sessionPos = pSessionTable->GetStartPosition();
	while (sessionPos)
	{
		pSession = pSessionTable->GetValueAt(sessionPos);
		if(pSession && pSession->GetContext())
		{
			if(pSession->GetServerType() == type)
			{
				pContext = pSession->GetContext();

				if(pContext && pContext->IsActive())
					SendRequest(pContext, packet.GetBuffer(), packet.GetLength());
			}
		}

		pSessionTable->GetNext(sessionPos);
	}

	/*for(int n=0; n<m_pSessionManager->m_SessionTable.size(); ++n)
	{
	pSession = m_pSessionManager->m_SessionTable[n];
	if(pSession == nullptr)
	continue;

	if(pSession->GetServerType() == type && pSession->GetContext() )
	{
	pContext = pSession->GetContext();

	if(pContext && pContext->IsActive())
	SendRequest(pContext, packet.GetBuffer(), packet.GetLength());
	}
	}*/

	BroadCastErrorSessionClear();
}

void NetLib::cDisPatcher::BroadCastErrorSessionClear()
{
	NetLib::cInterfaceIocpContext* pContext = NULL;

	for (size_t n = 0; n<m_ErrorSession.size(); ++n)
	{
		pContext = m_ErrorSession.front();
		m_ErrorSession.pop();
		m_pSessionManager->Remove(pContext->GetEntity());

		// IOCP로 메시지 날림, IOCP에서 컨텍스트 반환
		pContext->Disconnect(E_IO_FORCE_DISCONNECT);
	}
}

NetLib::cInterfaceIocpContext* NetLib::cDisPatcher::GetContextPtr(UINT nEntity)
{
	// 세션메니저에서 Context찾기
	NetLib::cSession* pSession = m_pSessionManager->Get(nEntity);
	if(!pSession) return NULL;

	return pSession->GetContext();
}

#if defined(VIRTUAL_NAGLE_ON)
void NetLib::cDisPatcher::RunVitualNagle()
{
	NetLib::cSession* pSession = m_pSessionManager->GetFirst();
	while (pSession)
	{
		// 자기자신을 제외한 브로드캐스팅, Entity값 비교
		NetLib::cIocpContext* pContext = static_cast<NetLib::cIocpContext*>(pSession->GetContext());
		pContext->Send();

		pSession = m_pSessionManager->GetNext();
	}

	BroadCastErrorSessionClear();
}
#endif