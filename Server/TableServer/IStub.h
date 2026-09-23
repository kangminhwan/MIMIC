#pragma once
#define _WINSOCKAPI_	//	NetLib 에서 Winsock
#include <Windows.h>
#include <functional>

namespace NetLib
{
	class cInterfaceIocpContext;
	class cUDPDispatcher;
	class cCommandQueueElement;
	class mRedis;
	class cInterfacePacketParser;
};

class cInstancePacketParser;
class IStubOwner
{
private:
	//	나중에 NetLib로 이 클래스가 올라간다면 NetLib::cInterfacePacketParser 로 변경됩니다.
	cInstancePacketParser* m_pOwner;

protected:
	inline void SetOwner(cInstancePacketParser* pOwner) { m_pOwner = pOwner; }
	inline cInstancePacketParser* GetOwner() { return m_pOwner; }

public:
	IStubOwner() : m_pOwner(nullptr) {}
	virtual ~IStubOwner() = 0 {}
};

class IStub : public IStubOwner
{
protected:
	friend class cInstancePacketParser;

protected:
	virtual void bindmethod(std::function<void(NetLib::cInterfaceIocpContext* /*pContext*/, UINT /*nCommand*/, BYTE* /*pData*/, UINT /*nLength*/, UINT /*nThreadIndex*/)>* fp) = 0;
	virtual void bindmethod(std::function<void(UINT /*nID*/, UINT /*nCommand*/, BYTE* /*pData*/, UINT /*nLength*/, UINT /*nThreadIndex*/)>* fp) = 0;
	//virtual void bindmethod(std::function<void(UINT /*nID*/, UINT /*nCommand*/, BYTE* /*pData*/, UINT /*nLength*/, UINT /*nThreadIndex*/)>* fp) = 0;

	virtual void bindmethod(std::function<void(NetLib::cUDPDispatcher* /*pUDPDispatcher*/, UINT /*nCommand*/, UINT /*Entity*/, UINT /*uiPacketSeq*/, NetLib::cCommandQueueElement* /*pCommandQueueElement*/, BYTE* /*pData*/, UINT /*nLength*/, UINT /*nThreadIndex*/)>* fp) = 0;

public:
	IStub() {}
	virtual ~IStub() = 0 {}
};