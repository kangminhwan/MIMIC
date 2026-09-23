#include "../../Include/Netlib/IOCP/cIocpOv.h"
#include "../../Include/Netlib/Common/cHeader.h"

NetLib::cIocpOv::cIocpOv(const int BufferSize, E_IO_OPERATION eOperation)
{
	Init(BufferSize, eOperation);
}


NetLib::cIocpOv::~cIocpOv()
{
	Destroy();
}

void NetLib::cIocpOv::Init(const UINT uiBufferLength, E_IO_OPERATION eOperation)
{
	m_pBuffer = NULL;

	memset(&m_WsaBuf, 0, sizeof(m_WsaBuf));
	memset(&m_Overlapped, 0, sizeof(WSAOVERLAPPED));

	if(Alloc(uiBufferLength))
	{
		m_WsaBuf.buf = reinterpret_cast<char*>(m_pBuffer);
		m_WsaBuf.len = m_uiMaxBufferLength;

		memset(m_pBuffer, 0, m_uiMaxBufferLength);
	}
	else
	{
		return;
	}

	m_uiDataSize = 0;

	SetOperation(eOperation);
}

void NetLib::cIocpOv::Destroy()
{
	if(m_pBuffer)
	{
		delete[]m_pBuffer;
		m_pBuffer = nullptr;
	}

	memset(&m_WsaBuf, 0, sizeof(m_WsaBuf));
	memset(&m_Overlapped, 0, sizeof(WSAOVERLAPPED));
}

void NetLib::cIocpOv::Clean()
{
	if(m_pBuffer)
	{
		memset(m_pBuffer, 0, m_uiMaxBufferLength);
		memset(&m_Overlapped, 0, sizeof(WSAOVERLAPPED));
	}

	m_uiDataSize = 0;
}

//////////////////////////////////////////////////////////////////////
// operation
//////////////////////////////////////////////////////////////////////

bool NetLib::cIocpOv::Alloc(const UINT uiBufferLength)
{
	if(m_pBuffer)
	{
		delete[]m_pBuffer;
		m_pBuffer = NULL;
	}

	m_pBuffer = new BYTE[uiBufferLength];
	if(m_pBuffer)
	{
		m_uiMaxBufferLength = uiBufferLength;
		return true;
	}
	return false;
}

#if defined(VIRTUAL_NAGLE_ON)
DWORD NetLib::cIocpOv::CopyBuffer(const BYTE* pBuffer, const UINT uiBufferLength)
{
	if(uiBufferLength >= m_uiMaxBufferLength)
	{
		m_WsaBuf.len = G_DEFIOBUFFERLEN;
		m_uiDataSize = G_DEFIOBUFFERLEN;
		memcpy(m_WsaBuf.buf, pBuffer, G_DEFIOBUFFERLEN);
		return G_DEFIOBUFFERLEN;
	}
	else
	{
		m_WsaBuf.len = uiBufferLength;
		m_uiDataSize = uiBufferLength;
		memcpy(m_WsaBuf.buf, pBuffer, uiBufferLength);
		return uiBufferLength;
	}
	return 0;
}

// 데이터를 Append시킨다.
DWORD NetLib::cIocpOv::AppendBuffer(const BYTE* pBuffer, const UINT uiBufferLength)
{
	// 보낼수 있는만큼만 리턴해줌..
	if(m_uiDataSize + uiBufferLength > m_uiMaxBufferLength)
	{
		m_WsaBuf.len = m_uiMaxBufferLength;
		memcpy(m_WsaBuf.buf + m_uiDataSize, pBuffer, m_uiMaxBufferLength - m_uiDataSize);
		return m_uiMaxBufferLength - m_uiDataSize;
	}
	else
	{
		m_WsaBuf.len = m_uiDataSize + uiBufferLength;
		m_uiDataSize += uiBufferLength;
		memcpy(m_WsaBuf.buf + m_uiDataSize, pBuffer, uiBufferLength);
		return uiBufferLength;
	}

	return 0;
}

bool NetLib::cIocpOv::CheckSend()
{
	if(m_uiDataSize == 0)
		return false;

	// 설정되어 있는 virtual nagle시간보다 크다면 무조건 센드한다.
	if(::GetTickCount() - m_dwStartTime >= VIRTUAL_NAGLE_MS)
		return true;

	float fRate = m_uiDataSize / (float)m_uiMaxBufferLength;

	if(fRate > 0.8)
		return true;

	return false;
}
#else 
// 데이터를 Copy함, 패킷을 전송할경우 처음에 꼭 CopyBuffer로써 데이터를 카피해여야 한다.
bool NetLib::cIocpOv::CopyBuffer(const BYTE* pBuffer, const UINT uiBufferLength)
{
	if(uiBufferLength > m_uiMaxBufferLength) return false;

	m_WsaBuf.len = uiBufferLength;
	m_uiDataSize = uiBufferLength;
	memcpy(m_WsaBuf.buf, pBuffer, uiBufferLength);

	return true;
}

// 데이터를 Append시킨다.
bool NetLib::cIocpOv::AppendBuffer(const BYTE* pBuffer, const UINT uiBufferLength)
{
	if(m_uiDataSize + uiBufferLength > m_uiMaxBufferLength) return false;

	m_WsaBuf.len = m_uiDataSize + uiBufferLength;
	memcpy(m_WsaBuf.buf + m_uiDataSize, pBuffer, uiBufferLength);
	return true;
}
#endif