#pragma once
#include "TableServerHeader.h"
#include "../Include/Netlib/Common/cInterfacePacketParser.h"
#include "../Include/Netlib/Network/cDisPatcher.h"
#include "../Include/Netlib/Network/cPacketStack.h"
#include "../Include/Netlib/IOCP/cIocpContext.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

#include <sstream>

#include "IStub.h"

namespace NetLib
{
	class cSession;
};

//typedef void(*fpHandlerArray[General::SysPacket_End])(NetLib::cInterfaceIocpContext*, UINT, BYTE*, UINT, class cInstancePacketParser*, UINT);
//typedef void(*fpHandlerServerArray[General::SysPacket_End])(UINT, UINT, BYTE*, UINT, UINT, class cInstancePacketParser*);

class cInstancePacketParser : public NetLib::cInterfacePacketParser, public NetLib::cDisPatcher
{
private:
	//fpHandlerArray m_fpHandlerArray;
	//fpHandlerServerArray m_fpHandlerServerArray;
	////fpHandlerWebArray m_fpHandlerWebArray;
	//fpHandlerArray m_fpHandlerWebArray;

	NetLib::cVector<IStub*> m_vecStub;
	/*
	* ���� �������� ���� �Ⱥ��ϰ��ϱ� ���� functional�� �迭�� ������ �մϴ�~
	*/
	std::function<void(NetLib::cInterfaceIocpContext*, UINT, BYTE*, UINT, UINT)> m_fpHandlerStub[General::PacketID::Packet_End];
	std::function<void(UINT, UINT, BYTE*, UINT, UINT)> m_fpHandlerServerStub[General::PacketID::Packet_End];

private:
	void Init();
	void ServerInit();
	void WebInit();

public:
	virtual void WebProcess(NetLib::cInterfaceIocpContext* pContext, UINT nCommand, BYTE* pData, UINT nLength, UINT nThreadIndex) override; // Web Thread
	virtual void WebProcess(UINT nID, UINT nCommand, BYTE* pData, UINT nLength, UINT nThreadIndex) override; // Web Thread
	virtual void Process(NetLib::cUDPDispatcher* pUDPDispatcher, UINT nCommand, UINT Entity, UINT uiPacketSeq, NetLib::cCommandQueueElement* pCommandQueueElement, BYTE* pData, UINT nLength, UINT nThreadIndex) override {} //UDP (Command Thread)
	virtual void Process(UINT nID, UINT nCommand, BYTE* pData, UINT nLength) override {} //Command Thread
	virtual void Process(NetLib::cInterfaceIocpContext* pContext, UINT nCommand, BYTE* pData, UINT nLength, UINT nThreadIndex) override; // Parser (Command Thread)
	virtual void Process(UINT nID, UINT nCommand, BYTE* pData, UINT nLength, UINT nThreadIndex, NetLib::mRedis* pRedis) override; //Server (Command Thread)

	virtual void NotifyContextLogout(UINT Entity) override;

	void CreateClientSession();
	void InitializeGameVersionChecker();

public:
	cInstancePacketParser();
	virtual ~cInstancePacketParser();

public:
	//	���� �޽����� ������ bDisconnectFlag �� ���� True�� DisConnect �մϴ�.
	void SendErrorWithDisConnect(UINT nCommand,
		const int nErrorCode,
		std::string& ErrorString,
		UINT nThreadIndex,
		NetLib::cInterfaceIocpContext* pContext,
		google::protobuf::Message& _message,
		BOOL bDisconnectFlag);

	//���������� Ŭ���̾�Ʈ���� ������ �� �Լ��� ����մϴ�?
	bool SendBuffer(NetLib::cInterfaceIocpContext* pContext,
		UINT nThreadIndex,
		UINT nCommand,
		google::protobuf::Message& _message,
		General::ResultCode errorCode = General::ResultCode::Result_Success,
		const std::string& errorString = "",
		e_thread_type thread = e_thread_type::e_command_thread);

	/*
		������ ��Ŷ�� ������ ���˴ϴ�.
		�� �Լ� �ȿ��� ���������� ServerHeader Ŭ������ �����ϰ� �����ϴ�.
	*/
	Server::ServiceStatusCode WebPostRequest(const std::string& url,
		google::protobuf::Message& _postMessage,
		UINT nCommand,
		int iServerID,
		int64 AID,
		std::string& data,
		const int default_time_out_second = 10);

	bool CheckPrivateIP(NetLib::cInterfaceIocpContext* pContext, bool ExcuteDisConnect, char* CallerInfo);
	void SendUnDefinedPacket( NetLib::cInterfaceIocpContext* pContext , UINT nCommand , UINT nThreadIndex );
};

// ==============================================================
// ==============================================================
