#pragma once
#include "../Common/Netlib.h"

BEGIN_NETLIB

class cIocpContext;
class cInterfaceIocpContext;
class cIocpConnector
{
protected:
	//	현재 서버가 다른 서버로 접속하기 위한 Context 관리
	ATL::CAtlMap<E_SERVER_TYPE, ATL::CAtlMap<int64, NetLib::cIocpContext*>*> m_myConnectorMap;

	//	다른 서버가 접속한 객체관리
	NetLib::cCriticalSection m_ConnectionLocks;
	ATL::CAtlMap<int64, ConnectorInfo*> m_connectedServerMaps[E_SERVER_TYPE::SERVER_TYPE_MAX];
	ATL::CAtlMap<int, GMCS_NOTI_GAMESERVERINFO*> m_atlmapGameServerAdditional;

	std::map<int64 , ConnectorInfo*> m_LobbyServers;
	std::map<int64, ConnectorInfo*> m_SlotServers;

	//Connector
public:
	//	현재 서버가 다른 서버로 접속하기 위해 모아두는 것을 도와주는 함수
	bool InsertConnector(E_SERVER_TYPE eServerType, NetLib::cIocpContext* pIocpContext);
	void KeepConnect();
	BOOL CheckServerConnected(E_SERVER_TYPE servertype, uint32 _uiIP, uint32 _uiPORT);
	BOOL CheckServerConnected(uint32 _uiIP, uint32 _uiPORT);
	BOOL CheckServerConnected(char* _pszIP, uint32 _uiPORT);

	E_ERROR_SEND SendPacket(E_SERVER_TYPE eServertype, UINT nCommand, BYTE* pData = nullptr, UINT nLength = 0);
	E_ERROR_SEND TargetSendPacket(E_SERVER_TYPE eServertype, uint32 _uiIP, uint32 _uiPORT, UINT nCommand, BYTE* pData, UINT nLength);
	E_ERROR_SEND TargetSendPacket(E_SERVER_TYPE eServertype, int64 _i64AllocatedSlot, UINT nCommand, BYTE* pData, UINT nLength);
	E_ERROR_SEND TargetExceptSendPacket(E_SERVER_TYPE eServertype, int64 _i64AllocatedSlot, UINT nCommand, BYTE* pData, UINT nLength);
	E_ERROR_SEND TargetSendPacket(E_SERVER_TYPE eServertype, NetLib::cIocpContext* pContext, UINT nCommand, BYTE* pData, UINT nLength);

	E_ERROR_SEND TargetAllSendPacket(E_SERVER_TYPE eServertype, UINT nCommand, BYTE* pData, UINT nLength);
	E_ERROR_SEND BroadCastToConnectors(E_SERVER_TYPE eServertype, UINT nCommand, BYTE* pData, UINT nLength);

	BOOL CheckLiveAndSendPacket(E_SERVER_TYPE servertype, UINT nCommand, BYTE* pData, UINT nLength);



	//Connection
public:
	//	다른 서버가 현재 서버로 접속했을때 따로 모아두기 위한 함수
	ConnectorInfo* RegisterServer(const BYTE servertype, TCHAR* IP, const std::string& public_ip, UINT PORT, UINT REMOTEPORT, cInterfaceIocpContext* pContext, int ServerGroupID, const int serverId, int64& allocatedslot);
	BOOL DeRegisterServer(const BYTE servertype, const int64 allocatedslot);
	ConnectorInfo* GetServerInfo(const BYTE servertype, const int serverId);
	ConnectorInfo* GetServerInfoByAllocatedSlot(const BYTE servertype, int64 allocatedslot);

	E_ERROR_SEND ConnectionALLSendPacket(const E_SERVER_TYPE servertype, UINT nCommand, BYTE* pData = nullptr, UINT nLength = 0);

public:
	NetLib::cIocpContext* GetServerContext(E_SERVER_TYPE eType, int64 ServerKey);

public:
	void MakeServerMapKey(int64& key, TCHAR* IP, UINT PORT);
	void MakeServerMapKey(int64& key, uint32 IP, uint32 PORT);

private:
	void Destroy();
public:
	cIocpConnector();
	virtual ~cIocpConnector();
};

END_NETLIB