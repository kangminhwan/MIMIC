#pragma once

#include <WS2tcpip.h>
#include <MSWSock.h>
#include <WinInet.h>
#include <IPHlpApi.h>
#include <atlcoll.h>
#include <atlstr.h>
#include <map>
#include <list>
#include <vector>
#include <queue>
#include <deque>
#include <string>
#include <process.h>
#include <tchar.h>
#include <strsafe.h>
#include <assert.h>
#include <fstream>
#include <functional>

#include <stdlib.h>
#include <crtdbg.h>

///Parallel Patterns Library(PPL)

///#include <ppltasks.h>

#pragma comment(lib, "WS2_32")
#pragma comment(lib, "Mswsock")
#pragma comment(lib, "Wininet")
#pragma comment(lib, "IPHLPAPI")