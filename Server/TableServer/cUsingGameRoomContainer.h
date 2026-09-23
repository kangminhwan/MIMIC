#pragma once
#include "TableServerHeader.h"

class cGameRoom;
class cUsingGameRoomContainer
{
public:
	NetLib::cSRWLock_CriticalSection m_SRWLock;
	std::map<int , cGameRoom*> _atlmapGameContainer;
};