#include "../../Include/Netlib/Queue/cBaseQueueElement.h"

NetLib::cBaseQueueElement::cBaseQueueElement(const int BufferSize)
{
	Init(BufferSize);
}


NetLib::cBaseQueueElement::~cBaseQueueElement()
{
	Destroy();
}

void NetLib::cBaseQueueElement::Init(const int BufferSize)
{
	//m_pData		= new BYTE[G_DEFIOBUFFERLEN];
	if(m_pData)
	{
		delete m_pData;
		m_pData = nullptr;
	}

	m_pData = new BYTE[BufferSize];
	m_nLength = 0;
	m_nBufferSize = BufferSize;
	memset(m_pData, 0x00, m_nBufferSize == 0 ? G_NET_BUFFER_SIZE_BASIC : m_nBufferSize);
}

void NetLib::cBaseQueueElement::Destroy()
{
	if(m_pData)
	{
		delete[]m_pData;
		m_pData = NULL;

		m_nLength = 0;
		m_nBufferSize = 0;
	}
}

void NetLib::cBaseQueueElement::Reset()
{
	if(m_pData)
	{
		m_nLength = 0;
		memset(m_pData, 0x00, m_nBufferSize == 0 ? G_NET_BUFFER_SIZE_BASIC : m_nBufferSize);
	}
}

//////////////////////////////////////////////////////////////////////
// operation
//////////////////////////////////////////////////////////////////////

BOOL NetLib::cBaseQueueElement::CopyData(const BYTE* lpBuffer, const UINT nLength)
{
	if(!m_pData)
	{
		m_pData = new BYTE[m_nBufferSize == 0 ? G_NET_BUFFER_SIZE_BASIC : m_nBufferSize];
		memset(m_pData, 0, m_nBufferSize == 0 ? G_NET_BUFFER_SIZE_BASIC : m_nBufferSize);
		m_nLength = 0;
		m_nBufferSize = 1024;
	}

	if(nLength > m_nBufferSize)
		return FALSE;

	if(!lpBuffer || !nLength)
	{
		m_nLength = 0;
		memset(m_pData, 0, m_nBufferSize);
		return TRUE;
	}

	memcpy(m_pData, lpBuffer, nLength);
	m_nLength = nLength;
	return TRUE;
}

BOOL NetLib::cBaseQueueElement::CopyString(const char* lpBuffer, const UINT nLength)
{
	if(!lpBuffer || !nLength)
		return FALSE;

	if(!m_pData)
	{
		m_pData = new BYTE[m_nBufferSize == 0 ? G_NET_BUFFER_SIZE_BASIC : m_nBufferSize];
		memset(m_pData, 0, m_nBufferSize == 0 ? G_NET_BUFFER_SIZE_BASIC : m_nBufferSize);
		m_nLength = 0;
		m_nBufferSize = 1024;
	}

	if(nLength > m_nBufferSize)
		return FALSE;

	if(!lpBuffer || !nLength)
	{
		m_nLength = 0;
		memset(m_pData, 0, m_nBufferSize);
		return TRUE;
	}

	memcpy(m_pData, lpBuffer, nLength);
	m_pData[nLength] = NULL;
	m_nLength = nLength;
	return TRUE;
}

BOOL NetLib::cBaseQueueElement::CopyString(const TCHAR* lpBuffer, const UINT nLength)
{
	if(!lpBuffer || !nLength)
		return FALSE;

	if(!m_pData)
	{
		m_pData = new BYTE[m_nBufferSize == 0 ? G_NET_BUFFER_SIZE_BASIC : m_nBufferSize];
		memset(m_pData, 0, m_nBufferSize == 0 ? G_NET_BUFFER_SIZE_BASIC : m_nBufferSize);
		m_nLength = 0;
		m_nBufferSize = 1024;
	}

	if(nLength > m_nBufferSize)
		return FALSE;

	if(!lpBuffer || !nLength)
	{
		m_nLength = 0;
		memset(m_pData, 0, m_nBufferSize);
		return TRUE;
	}

	memcpy(m_pData, lpBuffer, sizeof(TCHAR)*nLength);
	m_pData[sizeof(TCHAR)*nLength] = NULL;
	m_nLength = sizeof(TCHAR)*nLength;
	return TRUE;
}
