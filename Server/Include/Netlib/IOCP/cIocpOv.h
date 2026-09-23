#pragma once
#include "cOverlapped.h"

BEGIN_NETLIB

class cIocpOv : public cOverlapped
{
public:
	WSABUF		m_WsaBuf;

	UINT		m_uiMaxBufferLength;
	BYTE*		m_pBuffer;
	UINT		m_uiDataSize;

public:
	void	Init(const UINT uiBufferLength, E_IO_OPERATION eOperation = E_IO_NONE);
	void	Destroy();
	void	Clean();

public:
	bool	Alloc(const UINT uiBufferLength);

#if defined(VIRTUAL_NAGLE_ON)
	DWORD	m_dwStartTime;
	DWORD	CopyBuffer(const BYTE* pBuffer, const UINT uiBufferLength);
	DWORD	AppendBuffer(const BYTE* pBuffer, const UINT uiBufferLength);
	bool	CheckSend();
#else
	bool	CopyBuffer(const BYTE* pBuffer, const UINT uiBufferLength);
	bool	AppendBuffer(const BYTE* pBuffer, const UINT uiBufferLength);
#endif

public:
	cIocpOv(const int BufferSize = 1024, E_IO_OPERATION eOperation = E_IO_NONE);
	virtual ~cIocpOv();
};

END_NETLIB