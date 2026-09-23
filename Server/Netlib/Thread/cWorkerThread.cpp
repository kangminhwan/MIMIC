#include "../../Include/Netlib/Thread/cWorkerThread.h"
#include "../../Include/Netlib/Common/cHeader.h"
#include "../../Include/Netlib/Queue/cCommandQueue.h"
#include "../../Include/Netlib/Manager/ServerManager.h"
#include "../../Include/Netlib/Manager/cThreadManager.h"
#include "../../Include/Netlib/Network/cPacketStack.h"
#include "../../Include/Netlib/Buffer/cBuffer.h"
#include "../../Include/Netlib/IOCP/cIocp.h"
#include "../../Include/Netlib/IOCP/cIocpContext.h"
#include "../../Include/Netlib/IOCP/cIocpOv.h"
#include "../../Include/Netlib/Network/cNetWork.h"
#include "../../Include/Netlib/Queue/cLogQueue.h"
#include "../../Include/Netlib/Common/cSingleton.h"
#ifdef USING_MULTI_THREAD
#include "../../Include/Netlib/Manager/cCommandQueueManager.h"
#else
#include "../../Include/Netlib/Queue/cCommandQueue.h"
#endif

using namespace NetLib;

cWorkerThread::cWorkerThread()
{
	Init();
}


cWorkerThread::~cWorkerThread()
{
	Destroy();
	cBaseThread::Destroy();
}

void cWorkerThread::Init()
{
	cBaseThread::SetClassName(_T("cWorkerThread"));
#ifndef USING_MULTI_THREAD
	m_pCommandQueue = cSingleton<cCommandQueue>::GetInstance();
#endif
}

void cWorkerThread::Destroy()
{
	cIOCP* pIocp = cSingleton<cIOCP>::ExistsInstance();
	if(pIocp)
	{
		for (BYTE n = 0; n < m_byThreadCount; ++n)
		{
			PostQueuedCompletionStatus(pIocp->GetComepletionPort(), 0, NULL, nullptr);
		}

		WaitForMultipleObjects(m_byThreadCount, m_hThread, TRUE, INFINITE);
	}
}

void cWorkerThread::InitializeBuffer(const int iBufferCnt)
{
	/*m_pStorage = new cBuffer[nBufferCnt];

	for(int n=0; n<nBufferCnt; ++n)
	{
	m_pStorage[n].Create( G_MAXRECEIVEBUFFERLEN );
	}*/

	//m_pPacket = new cPacket[nBufferCnt];
}


//////////////////////////////////////////////////////////////////////
// operation
//////////////////////////////////////////////////////////////////////

void	cWorkerThread::Process(UINT ThreadArray)
{
	// TLS 셋팅, 쓰레드별로 하나씩 할당해준다.
	if((m_dwTlsIndex[ThreadArray] = TlsAlloc()) == TLS_OUT_OF_INDEXES)
	{
		Assert(m_dwTlsIndex[ThreadArray] != TLS_OUT_OF_INDEXES, _T("cWorkerThread::Process() TlsAlloc Failed"));
		return;
	}

	// TLS 셋팅, 쓰레드별로 하나씩 할당해준다.
	cPacketStack packet(CSNet::E_PROTOCOL::E_TCP);
	if(!TlsSetValue(m_dwTlsIndex[ThreadArray], (LPVOID)&packet))
	{
		cSingleton<ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("WorkerThread TlsSetValue Failed [thread %lu]"), GetCurrentThreadId());
		return;
	}

	bool	bSuccess;
	DWORD	dwTransferredBytes;

	cBuffer csStoreBuffer(G_DEF_MAX_STOREBUFFER_SIZE);
	TCHAR	szOutPut[CSDef::MAX_ERROR_STRING_BUFFER_LEN] = { 0, }; // 출력전용버퍼

	cIocpOv*		pIocpOv = NULL;
	cIocpContext*	pIocpContext = NULL;
	LPWSAOVERLAPPED	lpOverlapped = NULL;

	cIOCP* pIOCP = cSingleton<cIOCP>::ExistsInstance();
	cNetWork* pNetWork = cSingleton<cNetWork>::ExistsInstance();

	if(pIOCP == nullptr || pNetWork == nullptr)
	{
		return;
	}

	stThreadMonitor* pThreadMonitor = pThreadMonitor =
		NetLib::cSingleton<NetLib::cThreadManager>::GetInstance()->GetThreadMonitor(E_SERVER_THREAD_TYPE::E_SERVER_THREAD_TYPE_WORKER, ThreadArray);
	if (pThreadMonitor == nullptr)
	{
		assert(false && "WorkerThread::Process Failed. ThreadMonitor is nullptr");
		return;
	}

	cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_INFO, _T("WorkerThread Running:: [thread %lu]"), GetCurrentThreadId());
	//cSingleton<ServerManager>::GetInstance()->SendMessageToListBox( \
			//BLACK, _T("WorkerThread Running:: [thread %lu]"), GetCurrentThreadId() );

	while (!IsTerminated())
	{
		pIocpContext = NULL;
		lpOverlapped = NULL;

		DWORD dwErrorCode = 0;

		bSuccess = pIOCP->GetIocpStatus(	&dwTransferredBytes,
											reinterpret_cast<PULONG_PTR>(&pIocpContext),
											&lpOverlapped,
											500); // set worker default iocp timeout value 
		 
		// worker monitor set start
		pThreadMonitor->UpdateTick();

		if (!bSuccess)
		{
			dwErrorCode = GetErrorString(szOutPut, _countof(szOutPut));
		}

		//if( !bSuccess ) 
		//{
		//	// socket handle associated with a completion port is closed
		//	// lpOverlapped non-NULL and dwTransferredBytes equal zero
		//	if( lpOverlapped && ( dwTransferredBytes == 0 ))
		//	{
		//		OnClose( pIocpContext );
		//		cSingleton<ServerManager>::GetInstance()->SendMessageToListBox( \
		//		_T("WorkerThread:: [thread %lu] GQCS failed, associated socket closed"), GetCurrentThreadId() );
		//	}
		//	else
		//	{
		//		cSingleton<ServerManager>::GetInstance()->SendMessageToListBox( \
				//		_T("WorkerThread:: [thread %lu] GQCS failed, error %u"), GetCurrentThreadId(), GetLastError() );
		//	}
		//	continue;
		//}

		// 워커쓰레드 모니터링을 위해, iocp에 타임아웃을 추가 합니다.
		// timeout
		if (!bSuccess && pIocpContext == nullptr)
		{
			continue;
		}
		// finish worker thread
		else if (bSuccess && pIocpContext == nullptr)
		{
			return;
		}

		/*if( !lpOverlapped )
		{
		cSingleton<ServerManager>::GetInstance()->SendMessageToListBox( \
		_T("WorkerThread:: [thread %lu] GQCS timeout"), GetCurrentThreadId() );
		continue;
		}*/

		// 이벤트 검사
		//BYTE포인터로 변경해서 BYTE포인터를 빼준 이유는 cOverlapped클래스에 vtable이 생성이되서 그 크기 만큼 빼준겁니다~
		cOverlapped* overlapped = reinterpret_cast<cOverlapped*>(reinterpret_cast<BYTE*>(lpOverlapped) - sizeof(BYTE*));
		pIocpOv = reinterpret_cast<cIocpOv*>(overlapped);

		switch (pIocpOv->GetOperation())
		{
		case E_IO_ACCEPT: // 여기서 pIocpContext는 cSocket객체임
			{
				pIocpContext = CONTAINING_RECORD(pIocpOv, cIocpContext, m_olAccept);

				// AcceptEx 요청완료, GetAcceptingCount 카운트 하나 줄여줌
				pNetWork->AcceptCompleted(pIocpContext);

				// 새로운 Accept 요청
				pNetWork->AcceptRequest(pIocpContext->GetPortID());

				if(!bSuccess)
				{
					// 접속 대기 중 오류 - backlog된 소켓이 종료됐을때
					cSingleton<cLogQueue>::GetInstance()->PushCommand(	LOG_CRI, _T("WorkerThread AcceptEx Failed, Entity=[%u], ErrorCode=[%u], Message=[%s]"),
																		pIocpContext->GetEntity(),
																		dwErrorCode,
																		szOutPut);

					pNetWork->FinishErrorContext(pIocpContext);
					continue;
				}

				if(!pIocpContext->IsAssociated())
				{
					// 미등록된 Context를 IOCP에 등록
					if(!pIOCP->AssocInstance(	reinterpret_cast<HANDLE>(pIocpContext->::cSocket::GetSockHandle()),
												reinterpret_cast<ULONG_PTR>(pIocpContext)))
					{
						// 접속 대기 중 오류 - 이미 등록되어 있거나 기타 오류시
						dwErrorCode = GetErrorString(szOutPut, _countof(szOutPut));
						cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("WorkerThread AssocInstance Failed, Entity=[%u], ErrorCode=[%u], Message=[%s]"),
							pIocpContext->GetEntity(),
							dwErrorCode,
							szOutPut);

						pNetWork->FinishErrorContext(pIocpContext);
						continue;
					}

					pIocpContext->Associate(true);
				}

				// 접속성공
				pIocpContext->Reset(dwTransferredBytes);

				// Receive 요청
				if(!pIocpContext->ReceiveRequest())
				{
					OnClose(pIocpContext);
				}
				else
				{
	#if defined(_DEBUG)
					pIocpContext->SetStartTime();
	#endif
					//pIocpContext->DestroyAccept();
					OnConnect(pIocpContext, pIocpContext->GetAcceptBuffer(), dwTransferredBytes);
				}
			}
			break;
		case E_IO_RECEIVE:
			{
				// GQCS failed!!
				if(!bSuccess)
				{
					// WSARecv 실패하였다. 윈인??
					///*DWORD dwLastError = PrintLastError();
					//GetErrorString(szOutPut);
					//cSingleton<cLogQueue>::GetInstance()->PushCommand(_T("GQCS detect WSARecv Failed Entity[%lu] socket[%d], code[%u] = %s"), 
					//	pIocpContext->GetEntity(), pIocpContext->m_hSocket, dwLastError, szOutPut );*/

					OnClose(pIocpContext);
					continue;
				}

				// socket closed, client may closesocket
				if(!dwTransferredBytes)
				{
					OnClose(pIocpContext);
					continue;
				}

				// 패킷 처리
				if(OnReceive(pIocpContext, dwTransferredBytes, csStoreBuffer))
				{
					// Receive 요청
					if(!pIocpContext->ReceiveRequest())
					{
						// WSARecv가 소켓에러로 실패했을 경우..
						cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("WorkerThread Entity[%u] Socket[%d] ReceiveRequest Failed"),
							pIocpContext->GetEntity(), pIocpContext->m_hSocket);

						OnClose(pIocpContext);
					}
				}
				else
				{
					// 버퍼 카피 실패
					cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("WorkerThread Entity[%u] Socket[%d] OnReceive Failed"),
						pIocpContext->GetEntity(), pIocpContext->m_hSocket);

					OnClose(pIocpContext);
					//pNetWork->Finish( pIocpContext );
				}
			}
			break;
		case E_IO_SEND:
			{
				if(bSuccess)
				{
					if(dwTransferredBytes != pIocpOv->m_WsaBuf.len)
					{
						cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("WorkerThread GQCS detect WSASend failed to send whole packet Entity[%lu], WsaBufLen[%d], TransferredLen[%d]"),
							pIocpContext->GetEntity(), pIocpOv->m_WsaBuf.len, dwTransferredBytes);
					}

					//#ifdef _DEBUG
					//					cHeader* pHeader = reinterpret_cast<cHeader*>(pIocpOv->m_WsaBuf.buf);
					//
					//					cLogQueue* pLogQueue = cSingleton<cLogQueue>::ExistsInstance();
					//					if(pLogQueue)
					//					{
					//						if(pHeader->GetCommand() != CSNet::CS_PING || pHeader->GetCommand() != CSNet::SC_PONG)
					//							pLogQueue->PushCommand(LOG_CRI, _T("WorkerThread E_IO_SEND Complete, Command:%u, SendOvCnt:%d"), pHeader->GetCommand(), pIocpContext->GetSendOvlCnt());
					//					}
					//#endif
					pIocpContext->SendCompleted(pIocpOv);
				}
				else
				{
					pIocpContext->SendCompleted(pIocpOv);
				}
			}
			break;
		case E_IO_DISCONNECT: // E_IO_RECEIVE 에서 전송이 0일경우
			{
				if(bSuccess && pIocpContext)
				{
					// Context를 반환하고 새로운 접속을 요청
					if(pIocpContext->IsConnector() == false)
					{
						//	Pooler의 Context Table에 들어가져있는 상태
						if (pIocpContext->IsEnterContextTable() == true)
						{
							pNetWork->FinishAndAccept(pIocpContext);
						}
						else
						{
							//	Pooler의 Context Table 에 들어가져있지 않으면 이 루틴을
							pNetWork->FinishErrorContext(pIocpContext);
						}
					}
					else
					{
						pIocpContext->SetConnectorStatus(E_IOCP_CONNECTOR_STATUS::E_IOCP_CONNECTOR_STATUS_DISCONNECTED);
					}
				}
				else
				{
					BOOL bUseIPv6 = pIocpContext->IsUseIPv6();
					pIocpContext->ReCreateSocket(bUseIPv6);

					if(pIocpContext->IsConnector() == false)
					{
						// Accept 재처리
						pNetWork->FinishAndAccept(pIocpContext);
					}
					else
					{
						pIocpContext->SetConnectorStatus(E_IOCP_CONNECTOR_STATUS_DISCONNECTED);
					}

					DWORD dwLastError = PrintLastError();
					cSingleton<ServerManager>::GetInstance()->SendLogMessage(RED, _T("WorkerThread disconnect error %lu"), dwLastError);
				}
			}
			break;
			case E_IO_FORCE_DISCONNECT:
			{
				if(bSuccess && pIocpContext)
				{
					// Context를 반환하고 새로운 접속을 요청
					//pNetWork->Finish( pIocpContext );
				}
				else
				{
					DWORD dwLastError = PrintLastError();
					cSingleton<ServerManager>::GetInstance()->SendLogMessage(RED, _T("WorkerThread forcedisconnect error %lu"), dwLastError);
				}

				//	Pooler의 Context Table에 들어가져있는 상태
				if (pIocpContext->IsEnterContextTable() == true)
				{
					pNetWork->FinishAndAccept(pIocpContext);
				}
				else
				{
					//	Pooler의 Context Table 에 들어가져있지 않으면 이 루틴을
					pNetWork->FinishErrorContext(pIocpContext);
				}
			}
			break;
		case E_IO_CONNECT:
			{
				if(bSuccess && pIocpContext)
				{
					pIocpContext->Reset(0);

					if(!pIocpContext->ReceiveRequest())
					{
						OnClose(pIocpContext);
					}
					else
					{
	#if defined(_DEBUG)
						pIocpContext->SetStartTime();
	#endif
						pIocpContext->SetConnectorStatus(E_IOCP_CONNECTOR_STATUS::E_IOCP_CONNECTOR_STATUS_CONNECTED);

						OnConnectToServer(pIocpContext);

						cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_GRADE::LOG_CRI,
							"WorkerThread ConnectEx Success Entity=[%u], IP=[%s], Port=[%u]",
							pIocpContext->GetEntity(),
							pIocpContext->GetConnectorTargetIP(),
							pIocpContext->GetConnectorTargetPort());
					}
				}
				else
				{
					pIocpContext->SetConnectorStatus(E_IOCP_CONNECTOR_STATUS::E_IOCP_CONNECTOR_STATUS_DISCONNECTED);

					dwErrorCode = GetErrorString(szOutPut, _countof(szOutPut));

					cSingleton<cLogQueue>::GetInstance()->PushCommand(	LOG_GRADE::LOG_CRI,
																		"WorkerThread ConnectEx Failed, Entity=[%u], ErrorCode=[%u], Message=[%s], IP=[%s], Port=[%u]",
																		pIocpContext->GetEntity(),
																		dwErrorCode,
																		ATL::CW2A(szOutPut).m_psz,
																		pIocpContext->GetConnectorTargetIP(),
																		pIocpContext->GetConnectorTargetPort());
				}
			}
			break;
		default:
			{
				cSingleton<ServerManager>::GetInstance()->SendMessageToListBox(GREEN, _T("WorkerThread strange operation %d"), pIocpOv->GetOperation());
			}
			break;
		}

		// worker monitor set end
		pThreadMonitor->EndTick();
	}
}

bool	cWorkerThread::OnConnect(cIocpContext* pIocpContext, BYTE* pBuffer, UINT uiLength)
{
	if(pIocpContext->IsActive())
	{
		PushCommand(EVENT_CONNECT, pIocpContext, pBuffer, uiLength);
		return true;
	}
	return false;
}

bool	cWorkerThread::OnConnectToServer(cIocpContext* pIocpContext)
{
	if(pIocpContext->IsActive())
	{
		PushCommand(EVENT_CONNECT_TO_SERVER, pIocpContext);
		return true;
	}
	return false;
}

bool	cWorkerThread::OnClose(cIocpContext* pIocpContext)
{
	//if (pIocpContext->IsActive())
	if (pIocpContext->IsActive() && !pIocpContext->IsClosing())
	{
		pIocpContext->MarkClosing();
		PushCommand(EVENT_CLOSE, pIocpContext);
		pIocpContext->ResetClosing();
		return true;
	}
		
	pIocpContext->ResetClosing();
	return false;
}

bool	cWorkerThread::OnForceClose(cIocpContext* pIocpContext)
{
	if(pIocpContext->IsActive())
	{
		PushCommand(EVENT_FORCE_CLOSE, pIocpContext);
		return true;
	}
	return false;
}

bool cWorkerThread::OnReceive(cIocpContext* pIocpContext, DWORD dwLength, cBuffer& refBuffer)
{
	if(pIocpContext->GetStorageLength() == 0)
	{
		if(pIocpContext->StoreBuffer(pIocpContext->GetWsaBuffer(), dwLength) == FALSE)
		{
			return false;
		}
	}
	else
	{
		if(pIocpContext->AppendBuffer(pIocpContext->GetWsaBuffer(), dwLength) == FALSE)
		{
			return false;
		}
	}

	return GetPacket(pIocpContext, refBuffer);
}

bool cWorkerThread::GetPacket(cIocpContext* pIocpContext, cBuffer& tempBuffer)
{
	if(pIocpContext->GetStorageLength() < sizeof(cHeader))
	{
		return true;
	}

	cHeader* pHeader = reinterpret_cast<cHeader*>(pIocpContext->GetStorageBuffer());
	UINT uiPacketLength = sizeof(cHeader) + pHeader->GetPayload();
	UINT uiRemainSize = pIocpContext->GetStorageLength();

	if(uiPacketLength > uiRemainSize)
	{
		return true;
	}

	int ioffset = 0;
	while (uiRemainSize >= uiPacketLength)
	{
		if(!pHeader->CheckPacket())
		{
			cLogQueue* pLogQueue = cSingleton<cLogQueue>::ExistsInstance();
			if(pLogQueue != nullptr)
			{
				pLogQueue->PushCommand(LOG_GRADE::LOG_CRI, _T("Netlib [thread %lu] wrong identity[%lu] or too large payload[%lu] [entity %lu]"),
					GetCurrentThreadId(), pHeader->GetIdentity(), pHeader->GetPayload(), pIocpContext->GetEntity());
			}
			return false;
		}

		if(!PushCommand(	EventID::EVENT_RECEIVE,
							pIocpContext,
							reinterpret_cast<BYTE*>(pHeader),
							uiPacketLength))
		{
			cLogQueue* pLogQueue = cSingleton<cLogQueue>::ExistsInstance();
			if(pLogQueue != nullptr)
			{
				pLogQueue->PushCommand(LOG_GRADE::LOG_CRI, _T("cWorkerThread::GetPacket PushCommand failed : %lu"), pIocpContext->GetEntity());
			}
			return false;
		}

		ioffset += uiPacketLength;
		uiRemainSize -= uiPacketLength;
		if(uiRemainSize < sizeof(cHeader))
		{
			break;
		}

		pHeader = reinterpret_cast<cHeader*>((reinterpret_cast<BYTE*>(pHeader) + uiPacketLength));
		uiPacketLength = sizeof(cHeader) + pHeader->GetPayload();
	}

	if(pIocpContext->GetStorageLength() == ioffset)
	{
		pIocpContext->CleanStorage();
		return TRUE;
	}

	if(uiRemainSize > (pIocpContext->GetStorageMaxLength() / 2))
	{
		tempBuffer.Copy((pIocpContext->GetStorageBuffer() + ioffset), uiRemainSize);
		pIocpContext->CleanStorage();

		if(pIocpContext->StoreBuffer(tempBuffer.GetBuffer(), tempBuffer.GetLength()) == FALSE)
			return FALSE;
	}
	else
	{
		pIocpContext->StoreBuffer((pIocpContext->GetStorageBuffer() + ioffset), uiRemainSize);
	}

	return true;
}

bool	cWorkerThread::CheckPacket(cIocpContext* pIocpContext, BYTE* pBuffer, UINT uiLength)
{
	// 패킷의 헤더길이 검사
	if(uiLength < sizeof(cHeader))
	{
		// 헤더보다 작은 패킷이 들어왔을 경우 저장하고 끝난다
		//cSingleton<ServerManager>::GetInstance()->SendMessageToListBox( \
				//	_T("StarLib [thread %lu] short than header : %lu"), GetCurrentThreadId(), nLength );
		pIocpContext->StoreBuffer(pBuffer, uiLength);
		return true;
	}


	cHeader* pHeader = reinterpret_cast<cHeader*>(pBuffer);

	// 패킷의 유효성 검사
	if(!pHeader->IsPerfect())
	{
		cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("Netlib [thread %lu] wrong identity [value %lu] [entity %lu]"), GetCurrentThreadId(), pHeader->GetIdentity(), pIocpContext->GetEntity());

		/*cSingleton<ServerManager>::GetInstance()->SendMessageToListBox( \
		RED, _T("Netlib [thread %lu] wrong identity [value %lu] [entity %lu]"), \
		GetCurrentThreadId(), pHeader->GetIdentity(), pIocpContext->GetEntity() );*/
		//OnClose( pIocpContext );
		return false;
	}

	// 헤더로 페이로드 길이 확인
	if(pHeader->GetPayload() > G_DEFIOBUFFERLEN)
	{
		// 페이로드의 패킷 길이가 버퍼의 길이보다 크다.
		// 잘못된 패킷이 왔음, 변조일 가능성도 있다. 잘라버리자.
		cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("Netlib [thread %lu] too large payload : %lu"), GetCurrentThreadId(), pHeader->GetPayload());
		/*cSingleton<ServerManager>::GetInstance()->SendMessageToListBox( \
		RED, _T("Netlib [thread %lu] too large payload : %lu"), GetCurrentThreadId(), pHeader->GetPayload() );*/
		//OnClose( pIocpContext );
		return false;
	}

	// 패킷 헤더에 따른 길이 검사
	UINT uiPacketLength = sizeof(cHeader) + pHeader->GetPayload();
	if(uiLength < uiPacketLength) // 패킷길이가 잘려서 들어왔다
	{
		pIocpContext->StoreBuffer(pBuffer, uiLength);
	}
	else if(uiLength == uiPacketLength) // 패킷길이가 정확하게 들어왔다
	{
		// decryption 후 command queue에 push
		if(!PushCommand(EVENT_RECEIVE,
			pIocpContext,
			pBuffer,
			uiPacketLength))
		{
			cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("PushCommand failed : %lu"), pIocpContext->GetEntity());
			/*cSingleton<ServerManager>::GetInstance()->SendMessageToListBox( \
			RED, _T("PushCommand failed : %lu"), pIocpContext->GetEntity() );*/
			//OnClose( pIocpContext );
			return false;
		}
	}
	else if(uiLength > uiPacketLength)	// 패킷길이가 붙어서 들어왔다
	{
		//cSingleton<ServerManager>::GetInstance()->SendMessageToListBox( \
				//	_T("StarLib [thread %lu] [packet %lu][united %lu]"), GetCurrentThreadId(), nLength, unPacketLength );

// decryption 후 command queue에 push
		if(!PushCommand(EVENT_RECEIVE,
			pIocpContext,
			pBuffer,
			uiPacketLength))
		{
			cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("PushCommand failed : %lu"), pIocpContext->GetEntity());
			/*cSingleton<ServerManager>::GetInstance()->SendMessageToListBox( \
			RED, _T("PushCommand failed : %lu"), pIocpContext->GetEntity() );*/

			//OnClose( pIocpContext );
			return false;
		}

		// 다시 한번 패킷 검사
		return CheckPacket(pIocpContext, pBuffer + uiPacketLength, uiLength - uiPacketLength);
	}
	return true;
}

bool cWorkerThread::PushCommand(int iEvent, cIocpContext* pIocpContext, BYTE* pBuffer, UINT uiLength)
{
#ifndef USING_MULTI_THREAD
	if(!m_pCommandQueue)
		return true;
#endif

	bool bResult = false;

	switch (iEvent)
	{
	case EVENT_CONNECT:
	{
		// AcceptEx함수가 패킷을 받게되면 MSDN에서 성능 이슈가 생긴다고 했는대 성능이슈가 생기면 다시 부활?
		// 성능 이슈가 생겨서 부활했습니다.
		// 이슈는 클라이언트도 네이티브면 가능 했을꺼같은 생각입니다.
		// 문제는 클라이언트 엔진이 유니티가 되면서 Connect하면서 패킷을 같이 못보내면서
		// 서버소켓에서는 예약한 다음 패킷 사이즈만큼 와야지 Accept를 완료하는대 타임아웃 시간까지 대기 타면서 느리게 Accept하는 현상이 발견되었습니다.
		// 그래서 아래 코드들을 주석 처리 했습니다.
		BYTE* pBody = nullptr;
		cHeader* pHeader = reinterpret_cast<cHeader*>(pBuffer);

		if(uiLength > 0)
		{
			if(pIocpContext->IsCrypt())
				pBody = Decrypt(pBuffer);
			else
				pBody = Decrypt(pBuffer, FALSE);
		}

#ifndef USING_MULTI_THREAD
		bResult = m_pCommandQueue->PushCommand(pIocpContext, uiLength > 0 ? CSNet::SYS_NET_BUFFER_CONNECT : CSNet::SYS_NET_CONNECT, pBody, pHeader->GetPayload()); // Connect 패킷을 받은것으로 생각
		//bResult = m_pCommandQueue->PushCommand(pIocpContext, CSNet::SYS_NET_CONNECT); // Connect 패킷을 받은것으로 생각
#else
		pIocpContext->SetDefaultCommandQueueIndex();
		bResult = cSingleton<cCommandQueueManager>::GetInstance()->PushCommand(pIocpContext, uiLength > 0 ? CSNet::SYS_NET_BUFFER_CONNECT : CSNet::SYS_NET_CONNECT, pBody, pHeader->GetPayload()); // Connect 패킷을 받은것으로 생각
		//bResult = cSingleton<cCommandQueueManager>::GetInstance()->PushCommand(pIocpContext, CSNet::SYS_NET_CONNECT); // Connect 패킷을 받은것으로 생각
#endif
	}
	break;
	case EVENT_CONNECT_TO_SERVER:
	{
		
#ifndef USING_MULTI_THREAD
		bResult = m_pCommandQueue->PushCommand(pIocpContext, CSNet::SYS_NET_CONNECT_TO_SERVER);
#else
		pIocpContext->SetDefaultCommandQueueIndex();
		bResult = cSingleton<cCommandQueueManager>::GetInstance()->PushCommand(pIocpContext, CSNet::SYS_NET_CONNECT_TO_SERVER);
#endif
	}
	break;
	case EVENT_CLOSE:
	{
#ifndef USING_MULTI_THREAD
		bResult = m_pCommandQueue->PushCommand(pIocpContext, CSNet::SYS_NET_DISCONNECT); // Disconnect 패킷을 받은것으로 생각
#else
		bResult = cSingleton<cCommandQueueManager>::GetInstance()->PushCommand(pIocpContext, CSNet::SYS_NET_DISCONNECT); // Disconnect 패킷을 받은것으로 생각
#endif
	}
	break;
	case EVENT_FORCE_CLOSE:
	{
#ifndef USING_MULTI_THREAD
		bResult = m_pCommandQueue->PushCommand(pIocpContext, CSNet::SYS_NET_FORCE_DISCONNECT); // Disconnect 패킷을 받은것으로 생각
#else
		bResult = cSingleton<cCommandQueueManager>::GetInstance()->PushCommand(pIocpContext, CSNet::SYS_NET_FORCE_DISCONNECT); // Disconnect 패킷을 받은것으로 생각
#endif
	}
	break;

	case EVENT_RECEIVE:
	{
		BYTE* pBody = NULL;
		cHeader* pHeader = reinterpret_cast<cHeader*>(pBuffer);

		if(uiLength > 0)
		{
			if(pIocpContext->IsCrypt())
				pBody = Decrypt(pBuffer);
			else
				pBody = Decrypt(pBuffer, FALSE);
		}

#ifndef USING_MULTI_THREAD
		bResult = m_pCommandQueue->PushCommand(	pIocpContext,
												pHeader->GetCommand(),
												pBody,
												pHeader->GetPayload());
#else

		bResult = cSingleton<cCommandQueueManager>::GetInstance()->PushCommand(	pIocpContext,
																				pHeader->GetCommand(),
																				pBody,
																				pHeader->GetPayload());
#endif
	}
	break;
	}

	return bResult;
}

// workerthread 갯수만큼 Iocp큐의 completionkey값으로 NULL을 날려준다..
void cWorkerThread::PushWorkerThreadExit()
{
	cIOCP* pIOCP = cSingleton<cIOCP>::GetInstance();
	if(!pIOCP) return;

	for (int n = 0; n<m_byThreadCount; ++n)
	{
		PostQueuedCompletionStatus(pIOCP->GetComepletionPort(), 0, NULL, NULL);
	}
}

bool cWorkerThread::WaitThreadTermination()
{
	// WorkerThread 종료 신호를 기다림 (2초 동안 기다림.)
	DWORD dwWaitRet = WaitForMultipleObjects(m_byThreadCount, (CONST HANDLE*)m_hThread, TRUE, 1000);
	if(dwWaitRet != WAIT_OBJECT_0)
	{
		return false;
	}

	return true;
}

void cWorkerThread::PushDisconnect(cIocpContext* pIocpContext)
{
	cIOCP* pIOCP = cSingleton<cIOCP>::GetInstance();
	if(!pIOCP) return;

	PostQueuedCompletionStatus(	pIOCP->GetComepletionPort(), sizeof(pIocpContext),
								reinterpret_cast<ULONG_PTR>(pIocpContext), &(pIocpContext->m_olDisconnect.m_Overlapped));
}

BYTE* cWorkerThread::Decrypt(BYTE* pPacket, BOOL bDecryptOn)
{
	// 헤더 이후부터 복호화 한다.
	cHeader* pHeader = reinterpret_cast<cHeader*>(pPacket);
	BYTE* pBody = pPacket + sizeof(cHeader);

#ifdef _CRYPT
	if(bDecryptOn)
	{
		for (UINT n = 0; n<pHeader->GetPayload(); ++n)
		{
			BYTE* pByte = pBody + n;
			*pByte = *pByte ^ _packetMask;
		}
	}
#endif

	return pBody;
}
