#pragma once
#define _WINSOCKAPI_	//	NetLib 에서 Winsock9
#include <Windows.h>
#include <vector>
#include <assert.h>
#include <atlcoll.h>
#include <string>
#include "../Include/Common/CommonStructure.h"

#include <iostream>
#include <string>
#include <google/protobuf/descriptor.h>
#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/Server.pb.h"

#include <iostream>
#include <nlohmann/json.hpp>


namespace google
{
	namespace protobuf
	{
		class Message;
	};
};

namespace protoutil
{
	enum Table_PVP_PARSE
	{
		Table_PVP_PARSE_NONE ,
		Table_PVP_PARSE_OK ,
		Table_PVP_PARSE_ARGUMENT_NULL ,
		Table_PVP_PARSE_MESSAGE_BUFFER_SIZE_SMALL ,
		Table_PVP_PARSE_MESSAGE_SERIALIZE_FAILED ,
		Table_PVP_PARSE_CODED_BUFFER_SIZE_SMALL ,
		Table_PVP_PARSE_CODED_BUFFER_SERIALIZE_FAILED ,
	};

	enum GOOGLE_PROTO_USAGE
	{
		TCP,
		UDP,
		WEB,
	};

	/*
		google Protocol Buffers 를 사용하면서 조금이라도 편해보고자 만든 클래스입니다.
		Singleton Class입니다.
	*/

	class cProtoUtil
	{
	public:
		cProtoUtil();
		~cProtoUtil();

	private:
		//ATL::CAtlMap<Common::MSG_ID, google::protobuf::Message*> m_atlmapMessageProto;
		static std::vector<GOOGLE_PROTOBUF_BUFFER*> m_proto_buffers;
		static std::vector<GOOGLE_PROTOBUF_BUFFER*> m_proto_buffers_web;
		static std::vector<GOOGLE_PROTOBUF_BUFFER*> m_proto_buffers_udp;

		static ATL::CAtlMap<Server::ServiceStatusCode, const char*> m_web_error_string_container;

	private:
		//Protocol Buffers 의 DescriptorPool을 검색해서 Common::MsgID와 비슷한 이름으로 되어있는 Class들의 ProtoType을 찾아서 가지고 있게 해주는 함수입니다.
		//생성될때 자동적으로 한번만 호출되기 때문에 신경을 안써도 됩니다.
		protoutil::E_PROTO_STRING_ERROR_CODE Init();

	public:

		static void initialize_atlmap();


		//UtilFunction Functions
		static BOOL MemoryAllocProtobufSerializeBuffers(const int commandthread_cnt, const int webthread_cnt);
		static BOOL MemoryAllocProtobufUdpSerializeBuffers(const int udpthread_cnt);
		static GOOGLE_PROTOBUF_BUFFER* GetProtobufBuffer(const unsigned int commandthread_array);
		static GOOGLE_PROTOBUF_BUFFER* GetWebProtobufBuffer(const unsigned int webthread_array);
		static GOOGLE_PROTOBUF_BUFFER* GetUdpProtobufBuffer(const unsigned int udpthread_array);

		static void AllBufferDestroy();
		/*
		함수를 이용해서 Message클래스를 상속받고있는 객체의 셋팅되어있는 값을 문자열로 뽑아냅니다.
		output => output스트링입니다. 이곳에 Message클래스의 객체의 값이 문자열로 채워집니다.
		name => Message 클래스의 이름입니다. namespace 까지 써주셔야합니다.
		ex) "namespace.name" 입니다.
		data => 해당 Message객체의 데이터입니다.
		return value ErrorCode 입니다. 성공적으로 끝났을 경우는 OK 를 리턴합니다.
		*/
		static protoutil::E_PROTO_STRING_ERROR_CODE ToString(std::string& output, const std::string& name, std::string data, bool bConvertChar = true);

		/*
		함수를 이용해서 Message클래스를 상속받고있는 객체의 셋팅되어있는 값을 문자열로 뽑아냅니다.
		output => output스트링입니다. 이곳에 Message클래스의 객체의 값이 문자열로 채워집니다.
		message => Message 클래스 객체입니다.
		return value ErrorCode 입니다. 성공적으로 끝났을 경우는 OK 를 리턴합니다.
		*/
		static protoutil::E_PROTO_STRING_ERROR_CODE ToString(std::string& output, const google::protobuf::Message& message, bool bConvertChar = true);

		/*
			Encode the error text for the supplied Server::ServiceStatusCode.
		*/
		static void CodedWebErrorCodeString(std::string& codedstring, Server::ServiceStatusCode errorcode);

		/*
			ErrorCodeString 작성 하실때는 항상 이 함수로 작성하세요
			내부에서 ErrorCodeString 작성 할것인지 안할 것인지 검사를 합니다.
		*/
		static std::string ErrorCodeString(const char* strErrorString);

		/*
			General::ResultCode 를 넘겨주면 해당 값에 맞는 Default 에러스트링을 만들어줍니다.
		*/
		static void CodedTCPErrorCodeString(std::string& codedstring, General::ResultCode errorcode);

		/*
		 * codedBuffer에 message를 Serialize 해서 넣어줍니다.
		 * 내부에서 사이즈를 체크해서 안전하게 Serialize를 해줍니다.
		*/
		static Table_PVP_PARSE CodedMADEPVPResponseParse(BYTE* codedBuffer, size_t codedBufferSize, UINT& codedSize, const google::protobuf::Message& message, GOOGLE_PROTO_USAGE eUsage, UINT nThreadIndex);

		static Table_PVP_PARSE CodedMADEPVPResponseParse(BYTE* codedBuffer, size_t codedBufferSize, UINT& codedSize, const BYTE* pData, UINT nLength, UINT nThreadIndex);

		/*
		 * E_ERROR_SEND 에러를 General::ResultCode 타입으로 변환시켜 줍니다.
		 * 첫번째 파라미터로 넘어온 codedstring 에 어떤 에러인지를 포함 시켜줍니다.
		*/
		//static General::ResultCode ConvertSendErrorToErrorCode(std::string* codedstring, E_ERROR_SEND errorCode);

	private:

		//내부 버퍼 크기가 2048바이트 입니다.
		//OutBufferSize는 OutPutBuffer 버퍼의 크기를 넣어주세요
		//stUTF8Len은 strUTF8안에 들어있는 글자수를 넣어주세요
		//UTF8을 멀티바이트로 변경합니다.
		//OutBufferSize 를 넘어갈 경우 0을리턴합니다.
		//성공할경우는 사이즈를 리턴합니다.
		static int UTF8ToMultiByte(OUT char* OutPutBuffer, IN size_t OutBufferSize, IN char* strUTF8, IN size_t stUTF8Len)
		{
			if (OutPutBuffer == nullptr || strUTF8 == nullptr || stUTF8Len > (size_t)protoutil::E_PROTO_UTIL_BUFFER::MAX_BUFFER_2K_LEN)
			{
				return 0;
			}

			wchar_t strUnicode[protoutil::E_PROTO_UTIL_BUFFER::MAX_BUFFER_2K_LEN] = { 0, };
			int nLen = MultiByteToWideChar(CP_UTF8, 0, strUTF8, (int)stUTF8Len, NULL, NULL);
			MultiByteToWideChar(CP_UTF8, 0, strUTF8, (int)stUTF8Len, strUnicode, nLen);
			nLen = WideCharToMultiByte(CP_ACP, 0, strUnicode, -1, NULL, 0, NULL, NULL);
			if (OutBufferSize >= (size_t)nLen)
			{
				WideCharToMultiByte(CP_ACP, 0, strUnicode, -1, OutPutBuffer, nLen, NULL, NULL);
				return nLen;
			}
			return 0;
		}

		//UTF8로 되어있는 std::string 객체를 넘겨주면 MultiByte로 변경해서 돌려줍니다.
		static int UTF8ToMultiByte(IN OUT std::string& str)
		{
			char strConvert[protoutil::E_PROTO_UTIL_BUFFER::MAX_BUFFER_2K_LEN] = { 0, };
			int iLength = protoutil::cProtoUtil::UTF8ToMultiByte(strConvert, sizeof(strConvert), const_cast<char*>(str.c_str()), str.length());
			str.clear();
			str = strConvert;
			return iLength;
		}

	public:
		//Template Functions

		/*
		프로토콜 클래스 중에서 string변수의 값의 셋팅을 도와주는 함수입니다.
		일반 문자를 UTF8로 변환을 시켜서 셋팅해주는 함수입니다.
		내부적으로 버퍼의 크기가 protoutil::E_PROTO_UTIL_BUFFER::MAX_BUFFER_32K_LEN 입니다. 이 이상의 크기가 들어올겨우 assert가 발생합니다.
		내부 버퍼의 크기 이상이면 false를 리턴합니다.
		*/
		template<typename T>
		static bool SetProtoBuffer_String(IN OUT T* CodedInstance,
			IN const std::string& strData,
			IN void(T::* fptr)(const char*, size_t size))
		{
			if (strData.length() > protoutil::E_PROTO_UTIL_BUFFER::MAX_BUFFER_32K_LEN)
			{
				assert(FALSE && "[ProtoUtil.h][MakeProtocolString] is Faild. strData.length() > protoutil::E_PROTO_UTIL_BUFFER::MAX_BUFFER_32K_LEN");
				return false;
			}

			char strTempBuffer[protoutil::E_PROTO_UTIL_BUFFER::MAX_BUFFER_32K_LEN] = { 0, };

			int nSize = ::MultiByteToUTF8(strTempBuffer,
				sizeof(strTempBuffer),
				const_cast<char*>(strData.c_str()),
				strData.length());

			if (nSize == 0)
			{
				assert(FALSE && "[ProtoUtil.h][MakeProtocolString] is Faild. UTF8Convert Faild.");
				return false;
			}

			(CodedInstance->*fptr)(strTempBuffer, static_cast<size_t>(nSize));
			return true;
		}

		/*
		프로토콜 클래스 중에서 string변수의 값의 셋팅을 도와주는 함수입니다.
		일반 문자를 UTF8로 변환을 시켜서 셋팅해주는 함수입니다.
		내부적으로 버퍼의 크기가 protoutil::E_PROTO_UTIL_BUFFER::MAX_BUFFER_32K_LEN 입니다. 이 이상의 크기가 들어올겨우 assert가 발생합니다.
		내부 버퍼의 크기 이상이면 false를 리턴합니다.
		*/
		template<typename T>
		static bool SetProtoBuffer_String(IN OUT T* CodedInstance,
			IN const char* pStrData,
			IN size_t stLength,
			IN void(T::* fptr)(const char*, size_t size))
		{
			if (stLength > protoutil::E_PROTO_UTIL_BUFFER::MAX_BUFFER_32K_LEN)
			{
				assert(FALSE && "[ProtoUtil.h][MakeProtocolString] is Faild. strData.length() > protoutil::E_PROTO_UTIL_BUFFER::MAX_BUFFER_32K_LEN");
				return false;
			}

			char strTempBuffer[protoutil::E_PROTO_UTIL_BUFFER::MAX_BUFFER_32K_LEN] = { 0, };
			int nSize = ::MultiByteToUTF8(strTempBuffer,
				sizeof(strTempBuffer),
				const_cast<char*>(pStrData),
				stLength);

			if (nSize == 0)
			{
				assert(FALSE && "[ProtoUtil.h][MakeProtocolString] is Faild. UTF8Convert Faild.");
				return false;
			}

			(CodedInstance->*fptr)(strTempBuffer, static_cast<size_t>(nSize));
			return true;
		}

		//static std::string EnumToString( int enumValue , const google::protobuf::EnumDescriptor* descriptor );

		// ProtoEnumType 를 const 값이 오지 않도록
		template<typename ProtoEnumType>
		static std::string GetEnumString( ProtoEnumType& paramValue )
		{
			const google::protobuf::EnumDescriptor* enumDescriptor = nullptr;

			if constexpr ( std::is_same_v<ProtoEnumType , General::ResultCode> || std::is_same_v<ProtoEnumType , const General::ResultCode> ) {
				enumDescriptor = General::ResultCode_descriptor();
			}
			else if constexpr ( std::is_same_v<ProtoEnumType , General::PlayCategory> || std::is_same_v<ProtoEnumType , const General::PlayCategory> ) {
				enumDescriptor = General::PlayCategory_descriptor();
			}
			else if constexpr ( std::is_same_v<ProtoEnumType , Server::PlayPhase> || std::is_same_v<ProtoEnumType , const Server::PlayPhase> ) {
				enumDescriptor = Server::PlayPhase_descriptor();
			}
			else if constexpr ( std::is_same_v<ProtoEnumType , General::TableAction> || std::is_same_v<ProtoEnumType , const General::TableAction> ) {
				enumDescriptor = General::TableAction_descriptor();
			}
			else if constexpr ( std::is_same_v<ProtoEnumType , General::CardSuit> || std::is_same_v<ProtoEnumType , const General::CardSuit> ) {
				enumDescriptor = General::CardSuit_descriptor();
			}
			else if constexpr ( std::is_same_v<ProtoEnumType , General::CardRank> || std::is_same_v<ProtoEnumType , const General::CardRank> ) {
				enumDescriptor = General::CardRank_descriptor();
			}
			else if constexpr ( std::is_same_v<ProtoEnumType , General::HandRank> || std::is_same_v<ProtoEnumType , const General::HandRank> ) {
				enumDescriptor = General::HandRank_descriptor();
			}
			else if constexpr ( std::is_same_v<ProtoEnumType , General::PacketID> || std::is_same_v<ProtoEnumType , const General::PacketID> ) {
				enumDescriptor = General::PacketID_descriptor();
			}
			else if constexpr ( std::is_same_v<ProtoEnumType , General::AssetKind> || std::is_same_v<ProtoEnumType , const General::AssetKind> ) {
				enumDescriptor = General::AssetKind_descriptor();
			}
			else if constexpr ( std::is_same_v<ProtoEnumType , General::GrantItemKind> || std::is_same_v<ProtoEnumType , const General::GrantItemKind> ) {
				enumDescriptor = General::GrantItemKind_descriptor();
			}
			else if constexpr ( std::is_same_v<ProtoEnumType , General::InboxReason> || std::is_same_v<ProtoEnumType , const General::InboxReason> ) {
				enumDescriptor = General::InboxReason_descriptor();
			}
			else if constexpr ( std::is_same_v<ProtoEnumType , General::TaskCategory> || std::is_same_v<ProtoEnumType , const General::TaskCategory> ) {
				enumDescriptor = General::TaskCategory_descriptor();
			}
			else if constexpr ( std::is_same_v<ProtoEnumType , General::TaskTrigger> || std::is_same_v<ProtoEnumType , const General::TaskTrigger> ) {
				enumDescriptor = General::TaskTrigger_descriptor();
			}
			else if constexpr ( std::is_same_v<ProtoEnumType , General::BenefitTier> || std::is_same_v<ProtoEnumType , const General::BenefitTier> ) {
				enumDescriptor = General::BenefitTier_descriptor();
			}
			else if constexpr ( std::is_same_v<ProtoEnumType , General::SymbolCode> || std::is_same_v<ProtoEnumType , const General::SymbolCode> ) {
				enumDescriptor = General::SymbolCode_descriptor();
			}
			else if constexpr ( std::is_same_v<ProtoEnumType , General::SpinMode> || std::is_same_v<ProtoEnumType , const General::SpinMode> ) {
				enumDescriptor = General::SpinMode_descriptor();
			}
			else {
				printf( "NotWorkedYet" );
				//throw "NotWorkedYet";
				return "";
			}

			const google::protobuf::EnumValueDescriptor* enumValueDescriptor = enumDescriptor->FindValueByNumber( static_cast< int >( paramValue ) );
			return enumValueDescriptor->name();
		}

		static void ProtobufToJson( const Server::ProductData& protoData , std::string& jsonString )
		{
			nlohmann::json j;

			if ( protoData.chip() )
				j[ "chip" ] = protoData.chip();

			if ( protoData.coin() )
				j[ "coin" ] = protoData.coin();

			if ( protoData.gem() )
				j[ "gem" ] = protoData.gem();

			if ( protoData.member_ship_class() )
				j[ "member_ship_class" ] = protoData.member_ship_class();

			if ( protoData.kickout_ticket() )
				j[ "kickout_ticket" ] = protoData.kickout_ticket();

			if ( protoData.avatar_id() )
				j[ "avatar_id" ] = protoData.avatar_id();

			if ( protoData.count() )
				j[ "count" ] = protoData.count();

			jsonString = j.dump();
		}
	};

	static bool ParseJson( const std::string& jsonString , nlohmann::json& jsonData )
	{
		try
		{
			jsonData = nlohmann::json::parse( jsonString ); // 문자열을 JSON 객체로 변환
			return true;
		}
		catch ( const nlohmann::json::parse_error& e )
		{
			std::cerr << "JSON Parsing Error: " << e.what() << std::endl;
			return false;
		}
	}

};