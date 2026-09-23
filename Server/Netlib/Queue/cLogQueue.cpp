#include "../../Include/Netlib/Queue/cLogQueue.h"
#include "../../Include/Netlib/Queue/cLogQueueElement.h"
#include "../../Include/Netlib/Manager/ServerManager.h"
#include "../../Include/Netlib/Common/cSingleton.h"

NetLib::cLogQueue::cLogQueue() :
	m_bPoolCreated(false)
{
	bListBoxPrintOpertaion = true;
	m_pMemPooler = nullptr;
}


NetLib::cLogQueue::~cLogQueue()
{
	SetExitThread();

	if(m_pMemPooler)
	{
		size_t nPoolCnt = m_pMemPooler->GetRemainPoolCnt();
		int nCurCnt = m_pMemPooler->GetCurrentPoolCnt();

		size_t nPool_32K_Cnt = m_pMemPooler->GetRemainPool_32K_Cnt();
		int nCur_32K_Cnt = m_pMemPooler->GetCurrentPool_32K_Cnt();

		bool bLoop = true;

		while (bLoop)
		{
			NetLib::cLogQueueElement* pElement = reinterpret_cast<NetLib::cLogQueueElement*>(PopQueue(100));
			if (pElement != nullptr)
			{
				Free(pElement);
			}

			nPoolCnt = m_pMemPooler->GetRemainPoolCnt();
			nCurCnt = m_pMemPooler->GetCurrentPoolCnt();

			nPool_32K_Cnt = m_pMemPooler->GetRemainPool_32K_Cnt();
			nCur_32K_Cnt = m_pMemPooler->GetCurrentPool_32K_Cnt();

			if ((int)nPoolCnt == nCurCnt && (int)nPool_32K_Cnt == nCur_32K_Cnt)
			{
				bLoop = false;
			}
		}

		delete m_pMemPooler;
		m_pMemPooler = nullptr;
	}
}

void NetLib::cLogQueue::CreateLogQueueElementPool(int nMaxCnt)
{
	if (nMaxCnt == 0)
		throw ("CreateCommandQueueElementPool nMaxCnt size 0");

	m_pMemPooler = new NetLib::cMemPooler<NetLib::cLogQueueElement>(0, nMaxCnt, 0, TRUE);

	m_bPoolCreated = true;
}

//////////////////////////////////////////////////////////////////////
// operation
//////////////////////////////////////////////////////////////////////
void NetLib::cLogQueue::PushCommandString(const TCHAR* szLog, BOOL bCheckNewFile, const LOG_GRADE grade)
{
	//if(!GetListBoxPrint()) return; // 로그 메시지 출력&저장 옵션..
	if(!bListBoxPrintOpertaion)
		return;

	ServerManager* pServerManager = cSingleton<ServerManager>::GetInstance();
	if (pServerManager != nullptr)
	{
		TServerConfiguration* pConfig = pServerManager->GetConfiguration();
		if (pConfig != nullptr)
		{
			if (grade < pConfig->LogGrade)
				return;
		}
	}

	if(!szLog) return;
	if(_tcslen(szLog) >= BUFFSIZE)		return;

	NetLib::cLogQueueElement* pElement = m_pMemPooler->Pop();

	if(pElement)
	{
		pElement->bCheckNewFile = bCheckNewFile;

		DWORD dwLength = (DWORD)_tcslen(szLog);
		BOOL isWriteTime = TRUE;
		if(pElement->CopyLogString(szLog, dwLength, grade, isWriteTime))
		{
			if (PushQueue(reinterpret_cast<ULONG_PTR>(pElement), sizeof(NetLib::cLogQueueElement)) == false)
			{
				Free(pElement);
			}
		}
	}
	else
	{
		cSingleton<ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("NetLib :: not enough log queue"));
	}
}

void NetLib::cLogQueue::PushCommand(const LOG_GRADE grade, const TCHAR* szFormat, ...)
{
	ServerManager* pServerManager = cSingleton<ServerManager>::GetInstance();
	if (pServerManager != nullptr)
	{
		TServerConfiguration* pConfig = pServerManager->GetConfiguration();
		if (pConfig != nullptr)
		{
			if (grade < pConfig->LogGrade)
				return;
		}
	}

	//if(!GetListBoxPrint()) return; // 로그 메시지 출력&저장 옵션..
	if(!bListBoxPrintOpertaion)
		return;

	if(!szFormat)
		return;

	// 4천 바이트 이상 출력 안함
	const int STRING_BUFFER_SIZE = BUFFSIZE - 96;

	va_list marker;
	va_start(marker, szFormat);
	int stringlen = _vsctprintf(szFormat, marker);
	if(stringlen >= STRING_BUFFER_SIZE)// 스트링 사이즈 측정
	{
		va_end(marker);
		return;
	}

	TCHAR Buffer[STRING_BUFFER_SIZE];
	vswprintf_s(Buffer, szFormat, marker);
	va_end(marker);

	NetLib::cLogQueueElement* pElement = m_pMemPooler->Pop();
	if(pElement)
	{
		BOOL isWriteTime = TRUE;
		if(pElement->CopyLogString(Buffer, stringlen, grade, isWriteTime))
		{
			if (PushQueue(reinterpret_cast<ULONG_PTR>(pElement), sizeof(NetLib::cLogQueueElement)) == false)
			{
				Free(pElement);
			}
		}
	}
	else
	{
		NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("NetLib :: not enough log queue"));
	}
}

void NetLib::cLogQueue::EraseExcetionPushCommand(const LOG_GRADE grade, const char* szFormat, ...)
{
	if(!bListBoxPrintOpertaion)
		return;

	char szBuffer[5000] = { 0, };
	va_list marker;

	va_start(marker, szFormat);
	int stringlen = _vscprintf(szFormat, marker);
	// < -- 측정이 안되는 스트링이 들어 왔을 경우에 스택을 다 먹어 버린다. 
	// vsprintf_s(szBuffer, _countof(szBuffer), szFormat, marker);를 쓰면 런타임 환경에서 프로세스를 아예 죽여버리지는 않는다. security exception이 발생하는데 아직 막는 법을 찾지 못함
	vsprintf_s(szBuffer, szFormat, marker);
	va_end(marker);

	CA2W convertwite(szBuffer);

	TCHAR Buffer[4000];
	errno_t error = _tcsncpy_s(Buffer, _countof(Buffer), convertwite.m_szBuffer, _countof(Buffer) - 1);

	NetLib::cLogQueueElement* pElement = m_pMemPooler->Pop();
	if(pElement)
	{
		BOOL isWriteTime = TRUE;
		if(pElement->CopyLogString(Buffer, (DWORD)_tcslen(Buffer), grade, isWriteTime))
		{
			if (PushQueue(reinterpret_cast<ULONG_PTR>(pElement), sizeof(NetLib::cLogQueueElement)) == false)
			{
				Free(pElement);
			}
		}
	}
	else
	{
		NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("NetLib :: not enough log queue"));
	}
}

void NetLib::cLogQueue::PushCommand(const LOG_GRADE grade, const char* szFormat, ...)
{
	if(!bListBoxPrintOpertaion)
		return;

	// 4천 바이트 이상 출력 안함
	const int STRING_BUFFER_SIZE = BUFFSIZE - 96;

	va_list marker;
	va_start(marker, szFormat);
	int stringlen = _vscprintf(szFormat, marker);
	if(stringlen >= STRING_BUFFER_SIZE)// 스트링 사이즈 측정
	{
		va_end(marker);
		return;
	}

	char szBuffer[STRING_BUFFER_SIZE] = { 0, };
	vsprintf_s(szBuffer, _countof(szBuffer), szFormat, marker);
	va_end(marker);

	CA2W convertwite(szBuffer);

	TCHAR Buffer[STRING_BUFFER_SIZE];
	_tcsncpy_s(Buffer, _countof(Buffer), convertwite, _countof(Buffer) - 1);

	NetLib::cLogQueueElement* pElement = m_pMemPooler->Pop();
	if(pElement)
	{
		BOOL isWriteTime = TRUE;
		if(pElement->CopyLogString(Buffer, stringlen, grade, isWriteTime))
		{
			if (PushQueue(reinterpret_cast<ULONG_PTR>(pElement), sizeof(NetLib::cLogQueueElement)) == false)
			{
				Free(pElement);
			}
		}
	}
	else
	{
		NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("NetLib :: not enough log queue"));
	}
}

void NetLib::cLogQueue::PushCommandLongString(const LOG_GRADE grade, const char* szFormat, ...)
{
	if(!bListBoxPrintOpertaion)
		return;

	const int STRING_BUFFER_SIZE = 100000;

	va_list marker;
	va_start(marker, szFormat);
	int stringlen = _vscprintf(szFormat, marker);
	if(stringlen >= STRING_BUFFER_SIZE)// 스트링 사이즈 측정
	{
		va_end(marker);
		return;
	}

	char szBuffer[STRING_BUFFER_SIZE] = { 0, };
	vsprintf_s(szBuffer, szFormat, marker);
	va_end(marker);

	CA2W wstringbase(szBuffer);
	std::wstring wstringdata(wstringbase);

	TCHAR Buffer[BUFFSIZE];
	const int maxBufBlockSize = (BUFFSIZE - 100);
	size_t nCopySize = 0;
	int nLogPushCnt = 0;//첫번째 로그에는 시간이 들어갈 수 있도록 함.
	if(wstringdata.length() >= maxBufBlockSize)
	{
		size_t currPos = 0;
		while (currPos < wstringdata.length())
		{
			if(wstringdata.length() - currPos > maxBufBlockSize)
			{
				nCopySize = maxBufBlockSize;
			}
			else
			{
				nCopySize = wstringdata.length() - currPos;
			}

			if(nCopySize < 1)
				break;

			_tcsncpy_s(Buffer, wstringdata.c_str() + currPos, nCopySize);
			//_tcsncpy_s(Buffer, _countof(Buffer), wstringdata.c_str()+currPos, _countof(Buffer) - 1);
			//Buffer[nCopySize] = 0;
			currPos += nCopySize;

			NetLib::cLogQueueElement* pElement = m_pMemPooler->Pop();
			if(pElement)
			{
				if(pElement->CopyLogString(Buffer, (DWORD)_tcslen(Buffer), grade, nLogPushCnt == 0 ? TRUE : FALSE))//nLogPushCnt == 0;//첫번째 로그에는 시간이 들어갈 수 있도록 함.
				{
					if (PushQueue(reinterpret_cast<ULONG_PTR>(pElement), sizeof(NetLib::cLogQueueElement)) == false)
					{
						Free(pElement);
					}
				}
			}
			else
			{
				NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("NetLib :: not enough log queue"));
				break;
			}

			++nLogPushCnt;
		}//end while
	}
	else
	{
		nCopySize = wstringdata.length();
		_tcsncpy_s(Buffer, _countof(Buffer), wstringdata.c_str(), _countof(Buffer) - 1);

		NetLib::cLogQueueElement* pElement = m_pMemPooler->Pop();
		if(pElement)
		{
			BOOL isWriteTime = TRUE;
			if(pElement->CopyLogString(Buffer, (DWORD)_tcslen(Buffer), grade, isWriteTime))
			{
				if (PushQueue(reinterpret_cast<ULONG_PTR>(pElement), sizeof(NetLib::cLogQueueElement)) == false)
				{
					Free(pElement);
				}
			}
		}
		else
		{
			NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("NetLib :: not enough log queue"));
		}
	}
}

void NetLib::cLogQueue::PushUserCommand(const __int64 AID, const LOG_GRADE grade, const TCHAR* szFormat, ...)
{
	//if(!GetListBoxPrint()) return; // 로그 메시지 출력&저장 옵션..
	if(!bListBoxPrintOpertaion)
		return;

	if(!szFormat)
		return;

	// 4천 바이트 이상 출력 안함
	const int STRING_BUFFER_SIZE = BUFFSIZE - 96;

	va_list marker;
	va_start(marker, szFormat);
	int stringlen = _vsctprintf(szFormat, marker);
	if(stringlen >= STRING_BUFFER_SIZE)// 스트링 사이즈 측정
	{
		va_end(marker);
		return;
	}

	TCHAR Buffer[STRING_BUFFER_SIZE];
	vswprintf_s(Buffer, szFormat, marker);
	va_end(marker);

	NetLib::cLogQueueElement* pElement = m_pMemPooler->Pop();
	if(pElement)
	{
		if(pElement->CopyUserLogString(AID, Buffer, stringlen, grade))
		{
			if (PushQueue(reinterpret_cast<ULONG_PTR>(pElement), sizeof(NetLib::cLogQueueElement)) == false)
			{
				Free(pElement);
			}
		}
	}
	else
	{
		NetLib::cSingleton<NetLib::ServerManager>::GetInstance()->SendMessageToListBox(RED, _T("NetLib :: not enough log queue"));
	}

	va_end(marker);
}

size_t NetLib::cLogQueue::GetRemainQueueCnt()
{
	if(!m_bPoolCreated)
		return 0;

	return m_pMemPooler->GetRemainPoolCnt();
}

int NetLib::cLogQueue::GetMaxPoolCnt()
{
	if(!m_bPoolCreated)
		return 0;

	return m_pMemPooler->GetMaxPoolCnt();
}

int NetLib::cLogQueue::GetCurrentPoolCnt()
{
	if(!m_bPoolCreated)
		return 0;

	return m_pMemPooler->GetCurrentPoolCnt();
}

void NetLib::cLogQueue::Free(NetLib::cLogQueueElement* pElem)
{
	if ( pElem != nullptr )
		m_pMemPooler->Push(pElem);

	// 다시 Free 에 들어왔을때 처리 되지 않게 하기 위함
	pElem = nullptr;
}