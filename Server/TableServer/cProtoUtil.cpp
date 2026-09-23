#include "cProtoUtil.h"

#include "../Include/Netlib/Common/Flag.h"

#include "../../ProtocolBuffer/cpp/General.pb.h"
#include "../../ProtocolBuffer/cpp/PmNet.pb.h"

std::vector<GOOGLE_PROTOBUF_BUFFER*> protoutil::cProtoUtil::m_proto_buffers;
std::vector<GOOGLE_PROTOBUF_BUFFER*> protoutil::cProtoUtil::m_proto_buffers_web;
std::vector<GOOGLE_PROTOBUF_BUFFER*> protoutil::cProtoUtil::m_proto_buffers_udp;

ATL::CAtlMap<Server::ServiceStatusCode, const char*> protoutil::cProtoUtil::m_web_error_string_container;

protoutil::cProtoUtil::cProtoUtil()
{
	//생성이되면 Init을 합니당~
	protoutil::E_PROTO_STRING_ERROR_CODE eErrorCode = Init();

	//Init의 리턴값이 OK가 아닌 값이라면 if문이 실행이됩니다.
	if (protoutil::E_PROTO_STRING_ERROR_CODE::E_PROTO_STRING_ERROR_CODE_OK != eErrorCode)
	{
		//심각함으로 assert해줍니다.
		//assert가 났으니 무조건 해결해줍시다.
		//런타임중에 일어나는 오류는 아니니 충분히 해결할 수 있습니다.
		assert(false && "ProtoUtil::Init() Failed.");
	}
}

protoutil::cProtoUtil::~cProtoUtil()
{
}

/*
	Protocol Buffers 의 DescriptorPool을 검색해서 Common::MsgID와 비슷한 이름으로 되어있는 Class들의 ProtoType을 찾아서 가지고 있게 해주는 함수입니다.
	생성될때 자동적으로 한번만 호출되기 때문에 신경을 안써도 됩니다.
*/
protoutil::E_PROTO_STRING_ERROR_CODE protoutil::cProtoUtil::Init()
{
	//DescriptorPool 을 가져옵니다.
	const google::protobuf::DescriptorPool* pool = google::protobuf::DescriptorPool::generated_pool();
	if (!pool)
		return protoutil::E_PROTO_STRING_ERROR_CODE::E_PROTO_STRING_ERROR_CODE_DESCRIPTORPOOL; //DexcriptorPool객체가 nullpoint 입니다.

	//Enum의 Descriptot을 찾아옵니다.
	const google::protobuf::EnumDescriptor* enumDescriptor = pool->FindEnumTypeByName("Common.MsgID");
	if (!enumDescriptor)
		return protoutil::E_PROTO_STRING_ERROR_CODE::E_PROTO_STRING_ERROR_CODE_DESCRIPTOR; //EnumDescriptor 객체가 nullpoint 입니다.

	//Enum의 Count를 가져옵니다.
	int iValueCount = enumDescriptor->value_count();

	//Count 만큼 돕니다~
	for (int i = 0; i < iValueCount; ++i)
	{
		const google::protobuf::EnumValueDescriptor* enumValueDescriptor = enumDescriptor->value(i);
		const std::string strEnumName = enumValueDescriptor->name();
		int ii = 0;
	}

	//정상적으로 실행이 되었으므로 OK를 리턴합니다.
	return protoutil::E_PROTO_STRING_ERROR_CODE::E_PROTO_STRING_ERROR_CODE_OK;
}

void protoutil::cProtoUtil::initialize_atlmap()
{
	m_web_error_string_container.SetAt(Server::ServiceStatusCode::ServiceStatus_RequestTimedOut, "Web Time Out");
	m_web_error_string_container.SetAt(Server::ServiceStatusCode::ServiceStatus_Success, "Web OK");
	m_web_error_string_container.SetAt(Server::ServiceStatusCode::ServiceStatus_UnexpectedCondition, "Web Unknown");
	m_web_error_string_container.SetAt(Server::ServiceStatusCode::ServiceStatus_RequiredFieldMissing, "Web Unknown");

	m_web_error_string_container.SetAt(Server::ServiceStatusCode::ServiceStatus_RuntimeFault, "Web Exception");

	m_web_error_string_container.SetAt(Server::ServiceStatusCode::ServiceStatus_EndpointRejected, "Web Invalid Url");
	m_web_error_string_container.SetAt(Server::ServiceStatusCode::ServiceStatus_MessageEncodeFailed, "Web Message Serialize Failed");
	m_web_error_string_container.SetAt(Server::ServiceStatusCode::ServiceStatus_PostBodyEncodeFailed, "Web Post Serialize Failed");
	m_web_error_string_container.SetAt(Server::ServiceStatusCode::ServiceStatus_WebPayloadParseFailed, "Web Parsing Failed");


	m_web_error_string_container.SetAt(Server::ServiceStatusCode::ServiceStatus_CacheAccountMissing, "Web Not Exist Account Data");
	m_web_error_string_container.SetAt(Server::ServiceStatusCode::ServiceStatus_CacheUserDataMissing, "Web Not Exist Hero Data");
	m_web_error_string_container.SetAt(Server::ServiceStatusCode::ServiceStatus_SessionTokenMismatch, "Web Incorrect Session ID");

	//m_web_error_string_container.SetAt(legacy invalid battle type, "Web Invalid Battle Type");

	//m_web_error_string_container.SetAt(legacy tcppvp result count zero, "Web TCPPVPResult Count Zero");

}

BOOL protoutil::cProtoUtil::MemoryAllocProtobufSerializeBuffers(const int commandthread_cnt, const int webthread_cnt)
{
	GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTOBUF_BUFFER = nullptr;

	for (int n = 0; n < commandthread_cnt; ++n)
	{
		pGOOGLE_PROTOBUF_BUFFER = new GOOGLE_PROTOBUF_BUFFER();
		if (pGOOGLE_PROTOBUF_BUFFER == nullptr)
			return FALSE;

		m_proto_buffers.push_back(pGOOGLE_PROTOBUF_BUFFER);
	}

	for (int n = 0; n < webthread_cnt; ++n)
	{
		pGOOGLE_PROTOBUF_BUFFER = new GOOGLE_PROTOBUF_BUFFER();
		if (pGOOGLE_PROTOBUF_BUFFER == nullptr)
			return FALSE;

		m_proto_buffers_web.push_back(pGOOGLE_PROTOBUF_BUFFER);
	}

	return TRUE;
}

BOOL protoutil::cProtoUtil::MemoryAllocProtobufUdpSerializeBuffers(const int udpthread_cnt)
{
	GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTOBUF_BUFFER = nullptr;

	for (int n = 0; n < udpthread_cnt; ++n)
	{
		pGOOGLE_PROTOBUF_BUFFER = new GOOGLE_PROTOBUF_BUFFER();
		if (pGOOGLE_PROTOBUF_BUFFER == nullptr)
			return FALSE;

		m_proto_buffers_udp.push_back(pGOOGLE_PROTOBUF_BUFFER);
	}

	return TRUE;
}

GOOGLE_PROTOBUF_BUFFER* protoutil::cProtoUtil::GetProtobufBuffer(const unsigned int commandthread_array)
{
	if (m_proto_buffers.size() == 0)
		return nullptr;

	if (commandthread_array >= m_proto_buffers.size())
		return nullptr;

	return m_proto_buffers[commandthread_array];
}

GOOGLE_PROTOBUF_BUFFER* protoutil::cProtoUtil::GetWebProtobufBuffer(const unsigned int webthread_array)
{
	if (m_proto_buffers_web.size() == 0)
		return nullptr;

	if (webthread_array >= m_proto_buffers_web.size())
		return nullptr;

	return m_proto_buffers_web[webthread_array];
}

GOOGLE_PROTOBUF_BUFFER* protoutil::cProtoUtil::GetUdpProtobufBuffer(const unsigned int udpthread_array)
{
	if (m_proto_buffers_udp.size() == 0)
		return nullptr;

	if (udpthread_array >= m_proto_buffers_udp.size())
		return nullptr;

	return m_proto_buffers_udp[udpthread_array];
}

void protoutil::cProtoUtil::AllBufferDestroy()
{
	std::vector<GOOGLE_PROTOBUF_BUFFER*>::iterator iter = m_proto_buffers.begin();
	std::vector<GOOGLE_PROTOBUF_BUFFER*>::iterator iterEnd = m_proto_buffers.end();
	for (; iter != iterEnd; ++iter)
	{
		if ((*iter) != nullptr)
		{
			delete (*iter);
			(*iter) = nullptr;
		}
	}

	m_proto_buffers.clear();

	iter = m_proto_buffers_web.begin();
	iterEnd = m_proto_buffers_web.end();
	for (; iter != iterEnd; ++iter)
	{
		if ((*iter) != nullptr)
		{
			delete (*iter);
			(*iter) = nullptr;
		}
	}

	m_proto_buffers_web.clear();

	iter = m_proto_buffers_udp.begin();
	iterEnd = m_proto_buffers_udp.end();
	for (; iter != iterEnd; ++iter)
	{
		if ((*iter) != nullptr)
		{
			delete (*iter);
			(*iter) = nullptr;
		}
	}

	m_proto_buffers_udp.clear();
}

/*
	함수를 이용해서 Message클래스를 상속받고있는 객체의 셋팅되어있는 값을 문자열로 뽑아냅니다.
	output => output스트링입니다. 이곳에 Message클래스의 객체의 값이 문자열로 채워집니다.
	name => Message 클래스의 이름입니다. namespace 까지 써주셔야합니다.
	ex) "namespace.name" 입니다.
	data => 해당 Message객체의 데이터입니다.
	return value ErrorCode 입니다. 성공적으로 끝났을 경우는 OK 를 리턴합니다.
*/
protoutil::E_PROTO_STRING_ERROR_CODE protoutil::cProtoUtil::ToString(std::string& output, const std::string& name, std::string data, bool bConvertChar)
{
	//DescriptorPool 을 가져옵니다.
	const google::protobuf::DescriptorPool* pool = google::protobuf::DescriptorPool::generated_pool();
	if (!pool)
		return protoutil::E_PROTO_STRING_ERROR_CODE::E_PROTO_STRING_ERROR_CODE_DESCRIPTORPOOL; //DexcriptorPool객체가 nullpoint 입니다.

	//pool에서 사용자가 넘겨준 messageTypeName 을 검색 해서 Descriptor를 가져옵니다.
	const google::protobuf::Descriptor* descriptor = pool->FindMessageTypeByName(name);
	if (!descriptor)
		return protoutil::E_PROTO_STRING_ERROR_CODE::E_PROTO_STRING_ERROR_CODE_DESCRIPTOR; // Descriptor 객체가 nullpoint 입니다.

	//해당 Descriptor을 이용해서 message의 ProtoType을 가져옵니다.
	//ProtoType은 말그대로 ProtoType이여서 해당 ProtoType객체를 사용하지 못합니다.
	const google::protobuf::Message* prototypeMessage = google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
	if (!prototypeMessage)
		return protoutil::E_PROTO_STRING_ERROR_CODE::E_PROTO_STRING_ERROR_CODE_PROTOTYPE; // ProtoType 객체가 nullpoint 입니다.

	//ProtoType을 이용해서 새로운 객체를 생성합니다.
	google::protobuf::Message* message = prototypeMessage->New();
	if (!message)
		return protoutil::E_PROTO_STRING_ERROR_CODE::E_PROTO_STRING_ERROR_CODE_MESSAGE; //Message 객체가 nullpoint 입니다.

	//새로 만들어진 객체에 유저가 넘겨준 data를 파싱합니다.
	message->ParseFromString(data.c_str());

	//string변수에 2바이트 문자들은 UTF8로 변환이 되어있습니다.
	//그래서 Utf8DebugString을 이용해서 message객체안의 정보를 문자로 뽑아냅니다.
	output = message->Utf8DebugString();

	int iLength = 0;

	//밖에서 UTF8문자열을 변환을 시켜주고 콜을했다면 굳이 이곳에서 변환을 할 이유는 없습니다.
	if (bConvertChar)
	{
		//2바이트 문자들이 UTF8로 변환이 되어있기때문에 사용자가 보기쉬운 문자로 다시 변경을 시킵니다.
		iLength = protoutil::cProtoUtil::UTF8ToMultiByte(output);
	}

	//ProtoType을 이용해서 만든 객체를 지워줍니다.
	delete message;

	//iLength 의 값이 0보다 큰지를 비교합니다.
	//이유는 protoutil::cProtoUtil::UTF8ToMultiByte함수에서 실패하면 0을 리턴하기 때문입니다.
	return iLength > 0 ? E_PROTO_STRING_ERROR_CODE::E_PROTO_STRING_ERROR_CODE_OK : E_PROTO_STRING_ERROR_CODE::E_PROTO_STRING_ERROR_CODE_UTF8_CONVERT;
}

/*
	함수를 이용해서 Message클래스를 상속받고있는 객체의 셋팅되어있는 값을 문자열로 뽑아냅니다.
	output => output스트링입니다. 이곳에 Message클래스의 객체의 값이 문자열로 채워집니다.
	message => Message 클래스 객체입니다.
	return value ErrorCode 입니다. 성공적으로 끝났을 경우는 OK 를 리턴합니다.
*/
protoutil::E_PROTO_STRING_ERROR_CODE protoutil::cProtoUtil::ToString(std::string& output, const google::protobuf::Message& message, bool bConvertChar)
{
	//message 객체 안에 DebugString 이라는 함수가있습니다.
	//DebugString 이라는 함수는 UTF8이 아닌 문자열에 이스케이프를 붙입니다.
	//UTF8문자열로 변경을 해줘도 원래 문자가 UTF8이 아닌곳에는 이스케이프를 붙입니다.
	//그래서 Utf8DebugString 이라는 함수를 사용했습니다.
	//이 함수는 message객체 안에있는 문자열이 Utf8이라는 문자열이라고 생각해서 이스케이프를 붙이는 작업을 건너뛰고있습니다.
	output = message.Utf8DebugString();

	int iLength = 0;

	//밖에서 UTF8문자열을 변환을 시켜주고 콜을했다면 굳이 이곳에서 변환을 할 이유는 없습니다.
	if (bConvertChar)
	{
		//원래 2바이트 문자열이 UTF8문자열로 변경되어있는대.
		//밑에 함수에서 UTF8문자열로 변경된 2바이트 문자열을 다시 원래의 문자열로 변경해주는 작업을 하고있습니다.
		iLength = protoutil::cProtoUtil::UTF8ToMultiByte(output);
	}

	//iLength 의 값이 0보다 큰지를 비교합니다.
	//이유는 protoutil::cProtoUtil::UTF8ToMultiByte함수에서 실패하면 0을 리턴하기 때문입니다.
	return iLength > 0 ? protoutil::E_PROTO_STRING_ERROR_CODE::E_PROTO_STRING_ERROR_CODE_OK : protoutil::E_PROTO_STRING_ERROR_CODE::E_PROTO_STRING_ERROR_CODE_UTF8_CONVERT;
}

/*
 * codedBuffer에 message를 Serialize 해서 넣어줍니다.
 * 내부에서 사이즈를 체크해서 안전하게 Serialize를 해줍니다.
*/
protoutil::Table_PVP_PARSE protoutil::cProtoUtil::CodedMADEPVPResponseParse(BYTE* codedBuffer, size_t codedBufferSize, UINT& codedSize, const google::protobuf::Message& message, GOOGLE_PROTO_USAGE eUsage, UINT nThreadIndex)
{
	if (codedBuffer == nullptr || codedBufferSize == 0)
	{
		return protoutil::Table_PVP_PARSE::Table_PVP_PARSE_ARGUMENT_NULL;
	}

	GOOGLE_PROTOBUF_BUFFER* pGOOGLE_PROTOBUF_BUFFER = nullptr;

	switch (eUsage)
	{
	case TCP:
	{
		pGOOGLE_PROTOBUF_BUFFER = protoutil::cProtoUtil::GetProtobufBuffer(nThreadIndex);
	}
	break;
	case UDP:
	{
		pGOOGLE_PROTOBUF_BUFFER = protoutil::cProtoUtil::GetUdpProtobufBuffer(nThreadIndex);
	}
	break;
	case WEB:
	{
		pGOOGLE_PROTOBUF_BUFFER = protoutil::cProtoUtil::GetWebProtobufBuffer(nThreadIndex);
	}
	break;
	default:
		break;
	}

	size_t nSize = static_cast<size_t>(message.ByteSizeLong());

	if (nSize > sizeof(pGOOGLE_PROTOBUF_BUFFER->DataBuffer))
	{
		return protoutil::Table_PVP_PARSE::Table_PVP_PARSE_MESSAGE_BUFFER_SIZE_SMALL;
	}

	if (message.SerializeToArray(pGOOGLE_PROTOBUF_BUFFER->DataBuffer, static_cast<int>(sizeof(pGOOGLE_PROTOBUF_BUFFER->DataBuffer))) == false)
	{
		return protoutil::Table_PVP_PARSE::Table_PVP_PARSE_MESSAGE_SERIALIZE_FAILED;
	}

	PmNet::PktBase protoBase;

	protoBase.set_payload(pGOOGLE_PROTOBUF_BUFFER->DataBuffer, nSize);
	protoBase.set_payload_size(static_cast<google::protobuf::uint32>(nSize));
	protoBase.set_err_kind( General::ResultCode::Result_Success );

	UINT uiSize = static_cast<UINT>(protoBase.ByteSizeLong());

	if (uiSize > codedBufferSize)
	{
		return protoutil::Table_PVP_PARSE::Table_PVP_PARSE_CODED_BUFFER_SIZE_SMALL;
	}

	if (protoBase.SerializeToArray(codedBuffer, static_cast<int>(codedBufferSize)) == false)
	{
		return protoutil::Table_PVP_PARSE::Table_PVP_PARSE_CODED_BUFFER_SERIALIZE_FAILED;
	}

	codedSize = uiSize;

	return protoutil::Table_PVP_PARSE::Table_PVP_PARSE_OK;
}

protoutil::Table_PVP_PARSE protoutil::cProtoUtil::CodedMADEPVPResponseParse(BYTE* codedBuffer, size_t codedBufferSize, UINT& codedSize, const BYTE* pData, UINT nLength, UINT nThreadIndex)
{
	if (codedBuffer == nullptr || codedBufferSize == 0)
	{
		return protoutil::Table_PVP_PARSE::Table_PVP_PARSE_ARGUMENT_NULL;
	}

	PmNet::PktBase protoBase;

	protoBase.set_payload(pData, nLength);
	protoBase.set_payload_size(static_cast<google::protobuf::uint32>(nLength));
	protoBase.set_err_kind( General::ResultCode::Result_Success );

	UINT uiSize = static_cast<UINT>(protoBase.ByteSizeLong());

	if (uiSize > codedBufferSize)
	{
		return protoutil::Table_PVP_PARSE::Table_PVP_PARSE_CODED_BUFFER_SIZE_SMALL;
	}

	if (protoBase.SerializeToArray(codedBuffer, static_cast<int>(codedBufferSize)) == false)
	{
		return protoutil::Table_PVP_PARSE::Table_PVP_PARSE_CODED_BUFFER_SERIALIZE_FAILED;
	}

	codedSize = uiSize;

	return protoutil::Table_PVP_PARSE::Table_PVP_PARSE_OK;
}

/*
Server::ServiceStatusCode 를 넘겨주면 해당 값에 맞는 에러스트링을 만들어줍니다.
*/
void protoutil::cProtoUtil::CodedWebErrorCodeString(std::string& codedstring, Server::ServiceStatusCode errorcode)
{
	ATL::CAtlMap<Server::ServiceStatusCode, const char*>::CPair* pPair = m_web_error_string_container.Lookup(errorcode);
	if (pPair == nullptr)
	{
		codedstring = "!!Empty!!";
		return;
	}

	codedstring.insert(0, pPair->m_value);
}

/*
ErrorCodeString 작성 하실때는 항상 이 함수로 작성하세요
내부에서 ErrorCodeString 작성 할것인지 안할 것인지 검사를 합니다.
*/
std::string protoutil::cProtoUtil::ErrorCodeString(const char* strErrorString)
{
	//아직 if문이 없습니다. 
	//나중에 추가가 될 것입니다.

	std::string strError = strErrorString;

	return strError;
}

/*
	General::ResultCode 를 넘겨주면 해당 값에 맞는 Default 에러스트링을 만들어줍니다.
*/
void protoutil::cProtoUtil::CodedTCPErrorCodeString(std::string& codedstring, General::ResultCode errorcode)
{
	switch (errorcode)
	{
	case General::ResultCode::Result_Success:
		break;
	case General::ResultCode::Result_UnexpectedCondition:
	{
		codedstring = "Unknown Error";
	}
		break;
	case General::ResultCode::Result_TransportSerializeFailed:
	{
		codedstring = "Serialize Error";
	}
	break;
	default:
		break;
	}
}

/*
std::string protoutil::cProtoUtil::EnumToString( int enumValue , const google::protobuf::EnumDescriptor* descriptor )
{
	const google::protobuf::EnumValueDescriptor* valueDescriptor = descriptor->FindValueByNumber( enumValue );
	if ( valueDescriptor ) {
		return valueDescriptor->name();
	}
	else {
		return "Unknown"; // 알 수 없는 값 처리
	}
}
*/


/*
 * E_ERROR_SEND 에러를 General::ResultCode 타입으로 변환시켜 줍니다.
 * 첫번째 파라미터로 넘어온 codedstring 에 어떤 에러인지를 포함 시켜줍니다.
*/
/*
General::ResultCode protoutil::cProtoUtil::ConvertSendErrorToErrorCode(std::string* codedstring, E_ERROR_SEND errorCode)
{
	General::ResultCode eErrorCode = General::ResultCode::Result_Success;

	switch (errorCode)
	{
	case E_ERROR_SEND::E_ERROR_SEND_OK:
	{
		eErrorCode = General::ResultCode::Result_Success;
		if (codedstring != nullptr)
		{
			*codedstring = "";
		}
	}
	break;
	case E_ERROR_SEND::E_ERROR_SEND_ERROR:
	{
		eErrorCode = General::ResultCode::Result_TransportSendFailed;
		if (codedstring != nullptr)
		{
			*codedstring = protoutil::cProtoUtil::ErrorCodeString("Matching Server Send Error");
		}
	}
	break;
	case E_ERROR_SEND::E_ERROR_SEND_SEND_POOL_EMPTY:
		eErrorCode = General::ResultCode::Result_TransportSendPoolEmpty;
		if (codedstring != nullptr)
		{
			*codedstring = protoutil::cProtoUtil::ErrorCodeString("");
		}
		break;
	case E_ERROR_SEND::E_ERROR_SEND_DATA_SIZE_OVER:
		eErrorCode = General::ResultCode::Result_TransportSendPayloadTooLarge;
		if (codedstring != nullptr)
		{
			*codedstring = protoutil::cProtoUtil::ErrorCodeString("");
		}
		break;
	case E_ERROR_SEND::E_ERROR_SEND_WOULDBLOCK:
		eErrorCode = General::ResultCode::Result_TransportSendWouldBlock;
		if (codedstring != nullptr)
		{
			*codedstring = protoutil::cProtoUtil::ErrorCodeString("Matching Server Socket WouldBlock");
		}
		break;
	case E_ERROR_SEND::E_ERROR_SEND_SOCKET_ERROR:
		eErrorCode = General::ResultCode::Result_TransportSendSocketFault;
		if (codedstring != nullptr)
		{
			*codedstring = protoutil::cProtoUtil::ErrorCodeString("Matching Server Socket Error");
		}
		break;
	case E_ERROR_SEND::E_ERROR_SEND_YET:
		eErrorCode = General::ResultCode::Result_TransportSendPending;
		if (codedstring != nullptr)
		{
			*codedstring = protoutil::cProtoUtil::ErrorCodeString("Matching Server Socket Yet");
		}
		break;
	default:
	{
		eErrorCode = General::ResultCode::Result_TransportServerFault;
		if (codedstring != nullptr)
		{
			*codedstring = protoutil::cProtoUtil::ErrorCodeString("Invalid ErrorCode ID");
		}

		assert(false && "cInstancePacketParser::TCPPVPMatching_FP Failed. cIocpConnector::SendPacket error Code Not Defined");
	}
	break;
	}

	return eErrorCode;
}
*/
