#pragma once
#include "cBaseThread.h"

BEGIN_NETLIB

class cIocpContext;
class cWorkerThread : public cBaseThread
{
protected:
#ifndef USING_MULTI_THREAD
	class cCommandQueue* m_pCommandQueue;
#endif

protected:
	virtual void Process(UINT ThreadArray) override;

public:
	void	Init();
	void	Destroy();

	void	InitializeBuffer(const int iBufferCnt);
	void	PushWorkerThreadExit();
	bool	WaitThreadTermination();

public:
	bool	OnConnect(cIocpContext* pIocpContext, BYTE* pBuffer, UINT uiLength);
	bool    OnConnectToServer(cIocpContext* pIocpContext);
	bool	OnClose(cIocpContext* pIocpContext);
	bool	OnForceClose(cIocpContext* pIocpContext);
	bool	OnReceive(cIocpContext* pIocpContext, DWORD dwLength, class cBuffer& refBuffer);

	bool	CheckPacket(cIocpContext* pIocpContext, BYTE* pBuffer, UINT uiLength);

	bool	GetPacket(cIocpContext* pIocpContext, cBuffer& tempBuffer);

	bool	PushCommand(int iEvent, cIocpContext* pIocpContext, BYTE* pBuffer = nullptr, UINT uiLength = 0);

	// workerthread에서 context를 해제해 주기 위해 집어 넣었음..
	void	PushDisconnect(cIocpContext* pIocpContext);

	// 복호화 워커 쓰레드에서 처리한다.
	BYTE*	Decrypt(BYTE* pPacket, BOOL bDecryptOn = TRUE);

public:
	cWorkerThread();
	virtual ~cWorkerThread();
};

END_NETLIB