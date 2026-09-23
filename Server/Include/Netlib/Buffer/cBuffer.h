#pragma once
#include "../Common/Netlib.h"

BEGIN_NETLIB

class cBuffer
{
private:
	BYTE*	m_pDT;
	UINT	m_uiLen;
	UINT	m_uiMax;

public:
	void	Init();
	void	Destroy();

	bool	Create(UINT uiBuffLen);
	bool	Erase();

	UINT	GetMaxLength() { return m_uiMax; }
	UINT	GetLength() { return m_uiLen; }
	BYTE*	GetBuffer()	const { return m_pDT; }

	void    SetLength(UINT uiLen) { m_uiLen = uiLen; }

	BYTE*	GetBuffer(UINT uiLen)	const { return (m_pDT + uiLen); }

	bool	Copy(const char* pDT);
	bool	Copy(const char* pDT, UINT uiDest);
	bool	Copy(const BYTE* pDT, UINT uiLen);
	bool	Copy(UINT uiDest, const BYTE* pDT, UINT uiLen);

	bool	Append(const char pDT);
	bool	Append(const char* pDT);
	bool	Append(const BYTE* pDT, UINT uiLen);

	bool	AppendFromHead(const BYTE* pDT, UINT uiLen);

public:
	cBuffer();
	cBuffer(UINT uiBuffLen);
	~cBuffer();
};

END_NETLIB