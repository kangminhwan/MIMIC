#include "../../Include/Netlib/Buffer/cBuffer.h"

NetLib::cBuffer::cBuffer()
{
	Init();
}

NetLib::cBuffer::cBuffer(UINT uiBuffLen)
{
	Init();
	Create(uiBuffLen);
}

NetLib::cBuffer::~cBuffer()
{
	Destroy();
}

void NetLib::cBuffer::Init()
{
	m_pDT = NULL;
	m_uiLen = 0;
	m_uiMax = 0;
}

void NetLib::cBuffer::Destroy()
{
	if(m_pDT)
	{
		delete[]m_pDT;
		m_pDT = NULL;
	}

	m_uiLen = 0;
	m_uiMax = 0;
}

//////////////////////////////////////////////////////////////////////
// Create/Erase
//////////////////////////////////////////////////////////////////////

bool NetLib::cBuffer::Create(UINT uiBuffLen)
{
	if(m_pDT) return false;

	m_pDT = new BYTE[uiBuffLen];

	if(m_pDT)
	{
		m_uiLen = 0;
		m_uiMax = uiBuffLen;
		memset(m_pDT, 0, m_uiMax);
		return true;
	}

	return false;
}

bool NetLib::cBuffer::Erase()
{
	if(m_pDT)
	{
		memset(m_pDT, 0, m_uiLen);
		m_uiLen = 0;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////
// Copy/Append
//////////////////////////////////////////////////////////////////////

bool NetLib::cBuffer::Copy(const char* pDT)
{
	if(m_pDT)
	{
		UINT unLen = (UINT)strlen(pDT) + 1;
		if(unLen <= m_uiMax)
		{
			Erase();

			m_uiLen = unLen;
			memcpy(m_pDT, pDT, m_uiLen);
			return true;
		}
	}
	return false;
}

bool NetLib::cBuffer::Copy(const char* pDT, UINT uiDest)
{
	if(m_pDT)
	{
		UINT unLen = (UINT)strlen(pDT) + 1;
		if((uiDest + unLen) <= m_uiMax)
		{
			memcpy(m_pDT + uiDest, pDT, unLen);
			m_uiLen = uiDest + unLen;
			return true;
		}
	}
	return false;
}

bool NetLib::cBuffer::Copy(const BYTE* pDT, UINT uiLen)
{
	if(m_pDT)
	{
		if(uiLen > m_uiMax) return false;

		m_uiLen = uiLen;
		memcpy(m_pDT, pDT, m_uiLen);
		return true;
	}
	return false;
}

bool NetLib::cBuffer::Copy(UINT uiDest, const BYTE* pDT, UINT uiLen)
{
	if(m_pDT)
	{
		if((uiDest + uiLen) <= m_uiMax)
		{
			memcpy(m_pDT + uiDest, pDT, uiLen);
			m_uiLen = uiDest + uiLen;
			return true;
		}
	}
	return false;
}


bool NetLib::cBuffer::Append(const char pDT)
{
	if(m_pDT)
	{
		if((sizeof(char) + m_uiLen) <= m_uiMax)
		{
			m_pDT[m_uiLen] = pDT;
			m_uiLen++;
			return true;
		}
	}
	return false;
}

bool NetLib::cBuffer::Append(const char* pDT)
{
	if(m_pDT)
	{
		UINT unLen = (UINT)strlen(pDT) + 1;
		if((unLen + m_uiLen) <= m_uiMax)
		{
			memcpy(m_pDT + m_uiLen, pDT, unLen);
			m_uiLen += unLen;
			return true;
		}
	}
	return false;
}


bool NetLib::cBuffer::Append(const BYTE* pDT, UINT uiLen)
{
	if(m_pDT)
	{
		if((uiLen + m_uiLen) <= m_uiMax)
		{
			memcpy(m_pDT + m_uiLen, pDT, uiLen);
			m_uiLen += uiLen;
			return true;
		}
	}
	return false;
}

bool NetLib::cBuffer::AppendFromHead(const BYTE* pDT, UINT uiLen)
{
	if(m_pDT)
	{
		memcpy(m_pDT, pDT, uiLen);
	}
	return false;
}