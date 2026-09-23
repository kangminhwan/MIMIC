#include "../../Include/Netlib/UdpModule/cUDPIOCompletionData.h"

NetLib::cUDPIOCompletionData::cUDPIOCompletionData() :
	m_pUDPSession(nullptr)
{
}


NetLib::cUDPIOCompletionData::~cUDPIOCompletionData()
{
}

void NetLib::cUDPIOCompletionData::Clear()
{
	m_pUDPIocpOv.Clean();
	m_pUDPSession = nullptr;
}