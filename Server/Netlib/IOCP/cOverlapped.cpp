#include "../../Include/Netlib/IOCP/cOverlapped.h"

NetLib::cOverlapped::cOverlapped()
{
	Clean();
}


NetLib::cOverlapped::~cOverlapped()
{
}

void NetLib::cOverlapped::Clean()
{ 
	memset(&m_Overlapped, 0, sizeof(WSAOVERLAPPED)); 
}

void NetLib::cOverlapped::SetOperation(E_IO_OPERATION eOperation)
{ 
	m_Operation = eOperation;
}

WORD NetLib::cOverlapped::GetOperation()
{
	return m_Operation;
}