#pragma once
#include "../Common/Netlib.h"

BEGIN_NETLIB

class cBaseQueueElement
{
protected:
	UINT	m_nLength;
	BYTE*	m_pData;
	UINT    m_nBufferSize;

public:
	void	Init(const int BufferSize);
	void	Destroy();

	void	Reset();
public:
	BOOL	CopyData(const BYTE* lpBuffer, const UINT nLength);
	BOOL	CopyString(const char* lpBuffer, const UINT nLength);
	BOOL	CopyString(const TCHAR* lpBuffer, const UINT nLength);

	UINT	GetLength() { return m_nLength; }
	BYTE*	GetData() { return m_pData; }
	UINT	GetBufferSize() { return m_nBufferSize; }

public:
	cBaseQueueElement(const int BufferSize = G_NET_BUFFER_SIZE_BASIC);
	virtual ~cBaseQueueElement();
};

END_NETLIB