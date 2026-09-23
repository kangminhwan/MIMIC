#pragma once
#include "cBaseQueueElement.h"

BEGIN_NETLIB

class cWebQueueElement : public cBaseQueueElement
{
private:
	UINT m_nID;
	UINT m_nCommand;
	class cIocpContext* m_pContext;

private:
	void Init();
	void Destroy();

public:
	BOOL CopyData(	const class cIocpContext* pContext,
					const UINT nCommand,
					const BYTE* lpBuffer = nullptr,
					const UINT nLength = 0);

	BOOL CopyData(	const UINT nID,
					const UINT nCommand,
					const BYTE* lpBuffer = nullptr,
					const UINT nLength = 0);
public:
	UINT GetID() const;
	UINT GetCommand() const;
	class cIocpContext* GetContext() const;

	void MakeInitialize();

public:
	cWebQueueElement(const int BufferSize = G_NET_BUFFER_SIZE_BASIC);
	~cWebQueueElement();
};

END_NETLIB