#pragma once

#ifdef UNICODE
#define tstring	std::wstring
#define AddrInfo ADDRINFOW
#else 
#define tstring	std::string
#define AddrInfo ADDRINFO
#endif

#include <format>

static float RandomRange(float Min, float Max)
{
	return ((Max - Min)*((float)rand() / RAND_MAX)) + Min;
}

// GetLastError출력
inline DWORD PrintLastError(const char* szFile = NULL, long lLine = 0)
{
	DWORD dwError = WSAGetLastError();

#ifdef _DEBUG
	TCHAR tzFile[MAX_PATH] = { 0 };
	if(szFile)
	{
		MultiByteToWideChar(CP_ACP
			, 0
			, szFile
			, (int)(strlen(szFile) + 1)
			, tzFile
			, sizeof(tzFile) / sizeof(TCHAR));
	}

	LPTSTR lpMsgBuf = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwError,
		0, // Default language 
		(LPTSTR)&lpMsgBuf,
		0,
		NULL);

	TCHAR szPrintLastError[512] = { 0 };

	if(!szFile) swprintf_s(szPrintLastError, _T("WSAGetLastError[%lu] %s"), dwError, lpMsgBuf);
	else
		_tprintf_s(szPrintLastError, _T("WSAGetLastError[%lu] %s, src=%s, line=%d"), dwError, lpMsgBuf, tzFile, lLine);

	::OutputDebugString(szPrintLastError);

	// Free the buffer.
	LocalFree(lpMsgBuf);
#endif
	return dwError;
}

inline DWORD GetErrorString(TCHAR* szString, const int counfOfStringBuf)
{
	if(!szString || counfOfStringBuf < CSDef::MAX_ERROR_STRING_BUFFER_LEN)
		return 0;

	DWORD dwError = WSAGetLastError();

	LANGID langid = 0x409;
	LPTSTR lpMsgBuf = NULL;
	FormatMessage(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
					FORMAT_MESSAGE_FROM_SYSTEM |
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					dwError,
					langid, // force english
					(LPTSTR)&lpMsgBuf,
					0,
					NULL);

	_stprintf_s(szString, counfOfStringBuf, _T("%s"), lpMsgBuf);

	// Free the buffer.
	LocalFree(lpMsgBuf);

	return dwError;
}

inline void GetErrorString(TCHAR* szString, const int counfOfStringBuf, DWORD dwError)
{
	if (!szString || counfOfStringBuf < CSDef::MAX_ERROR_STRING_BUFFER_LEN)
		return;

	LANGID langid = 0x409; //MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT) <== get from system
	LPTSTR lpMsgBuf = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwError,
		langid, // force english 
		(LPTSTR)&lpMsgBuf,
		0,
		NULL);

	_stprintf_s(szString, counfOfStringBuf, _T("%s"), lpMsgBuf);

	// Free the buffer.
	LocalFree(lpMsgBuf);

	return;
}

inline void Trace(const TCHAR* szFormat, ...)
{
#ifdef _DEBUG
	TCHAR Buffer[1024];
	va_list marker;

	va_start(marker, szFormat);
	_vstprintf_s(Buffer, szFormat, marker);

	::OutputDebugString(Buffer);

	va_end(marker);
#endif
}

inline void TraceA( std::string outputString )
{
#ifdef _DEBUG

	std::ofstream ofs("output.txt", std::ios::out | std::ios::app);

	// 현재 시간 얻기
	std::time_t currentTime;
	std::time( &currentTime );

	std::tm timeInfo;
	if ( localtime_s( &timeInfo , &currentTime ) == 0 ) {
		char timeBuffer[ 64 ];
		std::strftime( timeBuffer , sizeof( timeBuffer ) , "%Y-%m-%d %H:%M:%S" , &timeInfo );

		// 시간 정보와 메시지를 결합
		std::string outString = timeBuffer + std::string( " " ) + outputString;

		::OutputDebugStringA( outString.c_str() );
		ofs << outString.c_str();
		::OutputDebugStringA( "\n" );
		ofs << "\n";
	}
	ofs.close();
#endif
}

inline void ReleaseTraceA(std::string outputString)
{
	// 현재 시간 얻기
	std::time_t currentTime;
	std::time(&currentTime);

	std::tm timeInfo;
	if (localtime_s(&timeInfo, &currentTime) == 0) {
		char timeBuffer[64];
		std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &timeInfo);

		// 시간 정보와 메시지를 결합
		std::string outString = timeBuffer + std::string(" ") + outputString;

		::printf(outString.c_str());
		::printf("\n");
	}
}

inline void TraceW( std::wstring outputString )
{
#ifdef _DEBUG
	::OutputDebugStringW( outputString.c_str() );
	::OutputDebugStringW( L"\n" );
#endif
}

// 범용 메세지 박스 . 기본 메세지 박스와 소스의 코드 위치도 알려 준다.
static void MSG_BOX(const TCHAR* msg)
{
	const size_t countOfTempBuf = (_tcslen(_T(__FILE__)) + _tcslen(msg)) + 80;
	TCHAR* pTemp = (TCHAR *)malloc(countOfTempBuf * sizeof(TCHAR));
	if(pTemp == nullptr)
		return;

	_stprintf_s(pTemp, countOfTempBuf, _T("%s \n\nFile: %s\nLine: %d  "), msg, _T(__FILE__), __LINE__);
	::MessageBox(NULL, pTemp, _T(__FILE__), MB_OK);
	free(pTemp);
}

static BOOL GetLocalAddress(TCHAR* pLocalIp)
{
	if(!pLocalIp)
		return FALSE;

	char   hostname[MAX_PATH] = { 0, };
	char   szLocal_IP[21] = { 0 };

	AddrInfo hints;
	AddrInfo* result = nullptr;

	char ip[MAX_PATH] = { 0, };

	if (gethostname(hostname, sizeof(hostname)) == 0)
	{
		memset(&hints, 0x00, sizeof(struct addrinfo));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		TCHAR hostname_real[MAX_PATH] = {0, };

#ifdef UNICODE
		_tcscpy_s(hostname_real, CA2W(hostname).m_psz);
#else
		strcpy_s(hostname_real, hostname);
#endif

		if(GetAddrInfo(hostname_real, nullptr, &hints, &result) == 0)
		{
			inet_ntop(result->ai_family, &(((struct sockaddr_in *)result->ai_addr)->sin_addr), ip, sizeof(ip));

			int iIpAddressLength = (int)strlen(ip);
			strcpy_s(szLocal_IP, ip);

			MultiByteToWideChar(	CP_ACP
									, 0
									, szLocal_IP
									, iIpAddressLength + 1		// 복사할 스트링의 길이
									, pLocalIp
									, iIpAddressLength + 1);

			return TRUE;
		}
	}

	return FALSE;
}

//// 자신의 랜카드 중에, 내부 아이피인놈만을 리턴해 준다.
//static BOOL GetPrivateAddress(char* pLocalIp)
//{
//	if(!pLocalIp)
//		return FALSE;
//
//	char   hostname[MAX_PATH];
//	char   szLocal_IP[16] = { 0 };
//
//	AddrInfo hints;
//	AddrInfo* result = nullptr;
//
//	char ip[MAX_PATH] = { 0, };
//	BOOL bvirtualIP = FALSE;
//
//	if(gethostname(hostname, sizeof(hostname)) == 0)
//	{
//		memset(&hints, 0x00, sizeof(struct addrinfo));
//		hints.ai_family = AF_INET;
//		hints.ai_socktype = SOCK_STREAM;
//		hints.ai_protocol = IPPROTO_TCP;
//
//		TCHAR hostname_real[MAX_PATH] = { 0, };
//
//#ifdef UNICODE
//		_tcscpy_s(hostname_real, CA2W(hostname).m_psz);
//#else
//		strcpy_s(hostname_real, hostname);
//#endif
//
//		if (GetAddrInfo(hostname_real, nullptr, &hints, &result) == 0)
//		{
//			inet_ntop(result->ai_family, &(((struct sockaddr_in *)result->ai_addr)->sin_addr), ip, sizeof(ip));
//
//			int iIpAddressLength = (int)strlen(ip);
//			strcpy_s(szLocal_IP, ip);
//
//			strcpy_s(pLocalIp, 16, szLocal_IP);
//		}
//		else
//			return FALSE;
//	}
//
//	return !bvirtualIP;
//}
//
//static BOOL GetPublicAddress(char* pLocalIp)
//{
//	if(!pLocalIp)
//		return FALSE;
//
//	char   hostname[MAX_PATH];
//	char   szLocal_IP[16] = { 0 };
//
//	AddrInfo hints;
//	AddrInfo* result = nullptr;
//
//	char ip[MAX_PATH] = { 0, };
//	BOOL bvirtualIP = FALSE;
//
//	if(gethostname(hostname, sizeof(hostname)) == 0)
//	{
//		memset(&hints, 0x00, sizeof(struct addrinfo));
//		hints.ai_family = AF_INET;
//		hints.ai_socktype = SOCK_STREAM;
//		hints.ai_protocol = IPPROTO_TCP;
//
//		TCHAR hostname_real[MAX_PATH] = { 0, };
//
//#ifdef UNICODE
//		_tcscpy_s(hostname_real, CA2W(hostname).m_psz);
//#else
//		strcpy_s(hostname_real, hostname);
//#endif
//
//		if(GetAddrInfo(hostname_real, nullptr, &hints, &result) == 0)
//		{
//			AddrInfo* temp = result;
//			while (temp != nullptr)
//			{
//				if(temp->ai_family == AF_INET)
//				{
//					inet_ntop(temp->ai_family, &(((struct sockaddr_in *)temp->ai_addr)->sin_addr), ip, sizeof(ip));
//					char* token = nullptr;//앞에 문자
//					char* context = nullptr;//분리하도 뒤의 문자
//					token = strtok_s(ip, ".", &context);
//
//					if(strcmp(token, "10") == 0 || strcmp(token, "127") == 0 || strcmp(token, "192") == 0)
//					{
//						temp = temp->ai_next;
//						continue;
//					}
//
//					strcpy_s(pLocalIp, 16, ip);
//					return TRUE;
//
//				}
//				temp = temp->ai_next;
//			}
//			
//				return TRUE;//한개라도 찾으면 리턴
//			}
//		}
//
//	return FALSE;
//}

static bool GetAddress(char* pOutIP, int nHint, BOOL bPrivate)
{
	if (pOutIP == nullptr)
		return false;

	bool firstFindIP = false;

	char   hostname[MAX_PATH];
	char   szLocal_IP[16] = { 0 };

	AddrInfo hints;
	AddrInfo* result = nullptr;

	char ip[MAX_PATH] = { 0, };

	if (gethostname(hostname, sizeof(hostname)) == 0)
	{
		memset(&hints, 0x00, sizeof(struct addrinfo));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		TCHAR hostname_real[MAX_PATH] = { 0, };

#ifdef UNICODE
		_tcscpy_s(hostname_real, CA2W(hostname).m_psz);
#else
		strcpy_s(hostname_real, hostname);
#endif

		if (GetAddrInfo(hostname_real, nullptr, &hints, &result) == 0)
		{
			AddrInfo* temp = result;
			while (temp != nullptr)
			{
				if (temp->ai_family == AF_INET)
				{
					inet_ntop(temp->ai_family, &(((struct sockaddr_in *)temp->ai_addr)->sin_addr), ip, sizeof(ip));
					char* token = nullptr;//앞에 문자
					char* context = nullptr;//분리하도 뒤의 문자
					strcpy_s(szLocal_IP, 16, ip);
					token = strtok_s(ip, ".", &context);

					if (bPrivate == TRUE)
					{
						if (strcmp(token, "10") == 0 && nHint == 10)
						{
							strcpy_s(pOutIP, 16, szLocal_IP);
							return true;
						}
						else if (strcmp(token, "172") == 0 && nHint == 172)
						{
							strcpy_s(pOutIP, 16, szLocal_IP);
							return true;
						}
						else if (strcmp(token, "127") == 0 && nHint == 127)
						{
							strcpy_s(pOutIP, 16, szLocal_IP);
							return true;
						}
						else if (strcmp(token, "192") == 0 && nHint == 192)
						{
							strcpy_s(pOutIP, 16, szLocal_IP);
							return true;
						}

						//	Private IP를 찾았는지 확인하고 private IP 인지도 확인합니다.
						if (firstFindIP == false && (strcmp(token, "10") == 0 || strcmp(token, "127") == 0 || strcmp(token, "172") || strcmp(token, "192") == 0))
						{
							strcpy_s(pOutIP, 16, szLocal_IP);
							firstFindIP = true;
						}

					}
					else
					{

						if (strcmp(token, "10") == 0 || strcmp(token, "127") == 0 || strcmp(token, "192") == 0)
						{
							temp = temp->ai_next;
							continue;
						}

						//	public IP 는 찾자마자 리턴합니다.
						strcpy_s(pOutIP, 16, szLocal_IP);
						return true;
					}
				}
				temp = temp->ai_next;
			}
				
			return true;//한개라도 찾으면 리턴
		}
	}
	
	return false;
}

static bool CheckPrivateIP(const uint32& uiIP)
{
	if (uiIP == 0)
	{
		return false;
	}

	const unsigned char chIP1 = uiIP >> 0 & 255;
	const unsigned char chIP2 = uiIP >> 8 & 255;

	if (chIP1 == 10)
	{
		return true;
	}
	else if (chIP1 == 127)
	{
		return true;
	}
	else if (chIP1 == 192)
	{
		if (chIP2 == 168)
		{
			return true;
		}
	}
	else if (chIP1 == 172)
	{
		if (chIP2 == 31)
		{
			return true;
		}
	}

	return false;
}

static bool ConvertIP(wchar_t* tzConnectIP, int nConnectIPSize, const uint32 uiIP)
{
	if (nConnectIPSize <= 0 ||
		tzConnectIP == nullptr)
	{
		return false;
	}

	SOCKADDR_IN sockAddr;
	sockAddr.sin_addr.s_addr = uiIP;
	char szConnectIP[CSDef::EDef::MAX_IP_ADDRESS_LEN] = { 0, };

	inet_ntop(AF_INET, &sockAddr.sin_addr, szConnectIP, sizeof(szConnectIP));

	MultiByteToWideChar(CP_ACP,
		0,
		szConnectIP,
		CSDef::EDef::MAX_IP_ADDRESS_LEN,
		tzConnectIP,
		nConnectIPSize);

	tzConnectIP[(nConnectIPSize / sizeof(wchar_t)) - 1] = 0;

	return true;
}

static bool ConvertIP(char* pszConnectIP, int nConnectIPSize, const uint32 uiIP)
{
	if (nConnectIPSize <= 0 ||
		pszConnectIP == nullptr)
	{
		return false;
	}

	SOCKADDR_IN sockAddr;
	sockAddr.sin_addr.s_addr = uiIP;

	inet_ntop(AF_INET, &sockAddr.sin_addr, pszConnectIP, nConnectIPSize);

	pszConnectIP[nConnectIPSize - 1] = 0;

	return true;
}

static std::string ConvertIP(const uint32_t uiIP)
{
	char ipBuffer[INET_ADDRSTRLEN]; // Buffer to hold the IP address as a string

	// Create a sockaddr_in structure and set the IP address
	struct sockaddr_in sockAddr;
	sockAddr.sin_addr.s_addr = uiIP;

	// Convert the IP address from binary to text form
	if (inet_ntop(AF_INET, &sockAddr.sin_addr, ipBuffer, sizeof(ipBuffer)) == nullptr)
	{
		return ""; // Return an empty string on error
	}

	return std::string(ipBuffer); // Return the IP address as a std::string
}

static tstring GetExeFileName()
{
	TCHAR m_local_path[MAX_PATH];
	::GetModuleFileName(NULL, m_local_path, MAX_PATH);
	tstring localpath(m_local_path);
	tstring sFind(_T(".exe"));
	tstring::size_type findpos = localpath.find(sFind);
	localpath = localpath.substr(0, findpos);

	findpos = localpath.rfind(_T("\\"));
	tstring exefilename = localpath.substr(findpos + 1, localpath.length());

	return exefilename;
}

static void UtilWriteConsole(TCHAR* szFormat, ...)
{
	HANDLE hCon;
	DWORD dwWrite;
	TCHAR Buffer[1024];
	va_list marker;

	va_start(marker, szFormat);
	_vstprintf_s(Buffer, szFormat, marker);
	va_end(marker);

	hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	WriteConsole(hCon, Buffer, (DWORD)_tcslen(Buffer), &dwWrite, NULL);
}

static BOOL CheckInputShutdownConsoleCommand(DWORD sleepTimeTick = 2000)
{
	char buffer[81] = { 0, };
	int i = 0, ch = 0;

	for (i = 0; (i < 80) && ((ch = getchar()) != EOF) && (ch != '\n'); i++)
	{
		buffer[i] = (char)ch;
	}

	buffer[i] = '\0';
	if(buffer[0] == 0 || strlen(buffer) == 0)
		return FALSE;

	if(strcmp(buffer, "end") == 0)
	{
		printf("You tried to finish server, server will be finish within %u secs\n", sleepTimeTick);
		Sleep(sleepTimeTick);
		return TRUE;
	}
	else
	{
		printf("Wrong Command\n");
	}

	return FALSE;
}

static INT _tinet_addr(const TCHAR *cp)
{
#ifdef UNICODE
	char IP[16] = { 0, };
	int Ret = 0;
	Ret = WideCharToMultiByte(CP_ACP, 0, cp, (int)_tcslen(cp), IP, 15, NULL, NULL);
	IP[Ret] = 0;
	IN_ADDR sin_addr;
	inet_pton(AF_INET, IP, &sin_addr.s_addr);
	return sin_addr.s_addr;
#endif

#ifndef UNICODE
	IN_ADDR sin_addr;
	inet_pton(AF_INET, cp, &sin_addr.s_addr);
	return sin_addr.s_addr;
#endif
}

static void ReplaceString(std::string & strCallId, const char * pszBefore, const char * pszAfter)
{
	size_t iPos = strCallId.find(pszBefore);
	size_t iBeforeLen = strlen(pszBefore);

	while (iPos < std::string::npos)
	{
		strCallId.replace(iPos, iBeforeLen, pszAfter);
		iPos = strCallId.find(pszBefore, iPos);
	}
}

static void ReplaceString( tstring& strCallId , const TCHAR* pszBefore , const TCHAR* pszAfter )
{
	size_t iPos = strCallId.find( pszBefore );
	size_t iBeforeLen = _tcslen( pszBefore );

	while ( iPos < std::string::npos )
	{
		strCallId.replace( iPos , iBeforeLen , pszAfter );
		iPos = strCallId.find( pszBefore , iPos );
	}
}

static std::string string_format(const char* szFormat, ...)
{
	std::string str;

	char Buffer[BUFFSIZE];
	memset(Buffer, 0x00, sizeof(Buffer));
	va_list marker;

	va_start(marker, szFormat);
	vsprintf_s(Buffer, szFormat, marker);

	str = Buffer;
	return str;
}

static bool GetStringWFromlpTo(UINT uiIP, TCHAR* addrBuf, UINT nCounOfBuf)
{
	if(CSDef::EDef::MAX_IP_ADDRESS_LEN > nCounOfBuf)
		return false;

	TCHAR IP[CSDef::EDef::MAX_IP_ADDRESS_LEN + 1] = { 0, };

	sockaddr_in addrin;
	memcpy(&addrin.sin_addr, &uiIP, sizeof(UINT));

	_stprintf_s(IP,
				CSDef::EDef::MAX_IP_ADDRESS_LEN,
				_T("%u.%u.%u.%u"),
				addrin.sin_addr.S_un.S_un_b.s_b1,
				addrin.sin_addr.S_un.S_un_b.s_b2,
				addrin.sin_addr.S_un.S_un_b.s_b3,
				addrin.sin_addr.S_un.S_un_b.s_b4);

	IP[CSDef::EDef::MAX_IP_ADDRESS_LEN - 1] = 0;

	//memcpy(address, IP, IPSIZE);
	_tcscpy_s(addrBuf, nCounOfBuf, IP);

	return true;
}

//내부 버퍼 크기가 2048바이트 입니다.
//OutBufferSize는 OutPutBuffer 버퍼의 크기를 넣어주세요
//strMultiByte은 strMultiByteLen안에 들어있는 글자수를 넣어주세요
//멀티바이트를 UTF8로 변경합니다.
//OutBufferSize 를 넘어갈 경우 0을리턴합니다.
//성공할경우는 사이즈를 리턴합니다.
static int MultiByteToUTF8(OUT char* OutPutBuffer,IN size_t OutBufferSize,IN char* strMultiByte,IN size_t strMultiByteLen)
{
	if(OutPutBuffer == nullptr || strMultiByte == nullptr || strMultiByteLen > (size_t)CSDef::EDef::MAX_UTF8_CONVERT_LENGTH)
	{
		return 0;
	}

	wchar_t strUnicode[CSDef::EDef::MAX_UTF8_CONVERT_LENGTH] = { 0, };
	int nLen = MultiByteToWideChar(CP_ACP, 0, strMultiByte, (int)strMultiByteLen, NULL, NULL);
	MultiByteToWideChar(CP_ACP, 0, strMultiByte, (int)strMultiByteLen, strUnicode, nLen);
	nLen = WideCharToMultiByte(CP_UTF8, 0, strUnicode, -1, NULL, 0, NULL, NULL);
	if(OutBufferSize >= (size_t)nLen)
	{
		int nLength = WideCharToMultiByte(CP_UTF8, 0, strUnicode, lstrlenW(strUnicode), OutPutBuffer, nLen, NULL, NULL);
		return nLength;
	}
	return 0;
}

//내부 버퍼 크기가 2048바이트 입니다.
//OutBufferSize는 OutPutBuffer 버퍼의 크기를 넣어주세요
//stUTF8Len은 strUTF8안에 들어있는 글자수를 넣어주세요
//UTF8을 멀티바이트로 변경합니다.
//OutBufferSize 를 넘어갈 경우 0을리턴합니다.
//성공할경우는 사이즈를 리턴합니다.
static int UTF8ToMultiByte(OUT char* OutPutBuffer,IN size_t OutBufferSize,IN char* strUTF8,IN size_t stUTF8Len)
{
	if(OutPutBuffer == nullptr || strUTF8 == nullptr || stUTF8Len > (size_t)CSDef::EDef::MAX_UTF8_CONVERT_LENGTH)
	{
		return 0;
	}

	wchar_t strUnicode[CSDef::EDef::MAX_UTF8_CONVERT_LENGTH] = { 0, };
	int nLen = MultiByteToWideChar(CP_UTF8, 0, strUTF8, (int)stUTF8Len, NULL, NULL);
	MultiByteToWideChar(CP_UTF8, 0, strUTF8, (int)stUTF8Len, strUnicode, nLen);
	int wLen = static_cast<int>(lstrlenW(strUnicode));
	nLen = WideCharToMultiByte(CP_ACP, 0, strUnicode, wLen, NULL, 0, NULL, NULL);
	if(OutBufferSize >= (size_t)nLen)
	{
		WideCharToMultiByte(CP_ACP, 0, strUnicode, wLen, OutPutBuffer, nLen, NULL, NULL);
		return nLen;
	}
	return 0;
}

//UTF8로 되어있는 std::string 객체를 넘겨주면 MultiByte로 변경해서 돌려줍니다.
static int UTF8ToMultiByte(IN OUT std::string& str)
{
	char strConvert[CSDef::EDef::MAX_UTF8_CONVERT_LENGTH] = { 0, };
	int iLength = UTF8ToMultiByte(strConvert, sizeof(strConvert), const_cast<char*>(str.c_str()), str.length());
	str.clear();
	str = strConvert;
	return iLength;
}

//MultiByte로 되어있는 std::string 객체를 넘겨주면 UTF8로 변경해서 돌려줍니다.
static int MultiByteToUTF8(IN OUT std::string& str)
{
	char strConvert[CSDef::EDef::MAX_UTF8_CONVERT_LENGTH] = { 0, };
	int iLength =  MultiByteToUTF8(strConvert, sizeof(strConvert), const_cast<char*>(str.c_str()), str.length());
	str.clear();
	str = strConvert;
	return iLength;
}

//내부 버퍼가 없습니다.
//OutBufferSize는 OutPutBuffer 버퍼의 크기를 넣어주세요
//strWideByteLen은 strWideByte안에 들어있는 글자수를 넣어주세요
//와이드바이트를 UTF8로 변경합니다.
//OutBufferSize 를 넘어갈 경우 0을리턴합니다.
//성공할경우는 사이즈를 리턴합니다.
static int WideByteToUTF8(OUT char* OutputBuffer,IN size_t OutBufferSize,IN wchar_t* strWideByte,IN size_t strWideByteLen)
{
	if(OutputBuffer == nullptr || strWideByte == nullptr || strWideByteLen > OutBufferSize)
	{
		return 0;
	}

	int nLen = WideCharToMultiByte(CP_UTF8, 0, strWideByte, -1, NULL, 0, NULL, NULL);
	if(OutBufferSize > (size_t)nLen)
	{
		WideCharToMultiByte(CP_UTF8, 0, strWideByte, (int)strWideByteLen, OutputBuffer, nLen, NULL, NULL);
		return nLen;
	}
	return 0;
}

//내부 버퍼가 없습니다.
//OutBufferSize는 OutPutBuffer 버퍼의 크기를 넣어주세요
//strUTF8Len은 strUTF8안에 들어있는 글자수를 넣어주세요
//UTF8을 와이드바이트로 변경합니다.
//OutBufferSize 를 넘어갈 경우 0을리턴합니다.
//성공할경우는 사이즈를 리턴합니다.
static int UTF8ToWideByte(OUT wchar_t* OutputBuffer,IN size_t OutBufferSize,IN char* strUTF8,IN size_t strUTF8Len)
{
	if(OutputBuffer == nullptr || strUTF8 || strUTF8Len > OutBufferSize)
	{
		return 0;
	}

	int nLen = MultiByteToWideChar(CP_UTF8, 0, strUTF8, (int)strUTF8Len, NULL, NULL);
	if(OutBufferSize > (size_t)nLen)
	{
		MultiByteToWideChar(CP_UTF8, 0, strUTF8, (int)strUTF8Len, OutputBuffer, nLen);
		return nLen;
	}
	return 0;
}

static tm GetGMTime()
{
	struct tm newtime;
	__int64 ltime;
	//char buf[26];
	errno_t err;

	_time64(&ltime);

	// Obtain coordinated universal time:   
	err = _gmtime64_s(&newtime, &ltime);
	if (err)
	{
		printf("GetGMTime() Invalid Argument to _gmtime64_s.\n");
	}

	//// Convert to an ASCII representation   
	//err = asctime_s(buf, 26, &newtime);
	//if (err)
	//{
	//	printf("Invalid Argument to asctime_s.");
	//}

	//printf("Coordinated universal time is %s\n", buf);

	return newtime;
}

//////////////////////////////////////////////////////Base64//////////////////////////////////////////////////////
///////////http://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c

static char encoding_table[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/' 
};

static char decoding_table[] = {
	0,0,0,0,0,0,0,0,0,0, //9
	0,0,0,0,0,0,0,0,0,0, //19
	0,0,0,0,0,0,0,0,0,0, //29
	0,0,0,0,0,0,0,0,0,0, //39
	0,0,0,62,0,0,0,63,52,53, //49
	54,55,56,57,58,59,60,61,0,0, //59
	0,0,0,0,0,0,1,2,3,4, //69
	5,6,7,8,9,10,11,12,13,14, //79
	15,16,17,18,19,20,21,22,23,24, //89
	25,0,0,0,0,0,0,26,27,28,//99
	29,30,31,32,33,34,35,36,37,38, //109
	39,40,41,42,43,44,45,46,47,48, //119
	49,50,51,0,0,0,0,0,0,0, //129
	0,0,0,0,0,0,0,0,0,0, //139
	0,0,0,0,0,0,0,0,0,0, //149
	0,0,0,0,0,0,0,0,0,0, //159
	0,0,0,0,0,0,0,0,0,0, //169
	0,0,0,0,0,0,0,0,0,0, //179
	0,0,0,0,0,0,0,0,0,0, //189
	0,0,0,0,0,0,0,0,0,0, //199
	0,0,0,0,0,0,0,0,0,0, //209
	0,0,0,0,0,0,0,0,0,0, //219
	0,0,0,0,0,0,0,0,0,0, //229
	0,0,0,0,0,0,0,0,0,0, //239
	0,0,0,0,0,0,0,0,0,0, //249
	0,0,0,0,0,0
};

static int mod_table[] = { 0, 2, 1 };


static size_t base64_encode(const unsigned char *data,
							size_t input_length,
							std::string& coded_base64)
{
	if(data == nullptr || input_length == 0)
	{
		return 0;
	}

	size_t return_length = (4 * ((input_length + 2) / 3));

	for (size_t n = 0; n < input_length;) 
	{

		uint32_t octet_a = n < input_length ? (unsigned char)data[n++] : 0;
		uint32_t octet_b = n < input_length ? (unsigned char)data[n++] : 0;
		uint32_t octet_c = n < input_length ? (unsigned char)data[n++] : 0;

		uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

		coded_base64 += encoding_table[(triple >> 3 * 6) & 0x3F];
		coded_base64 += encoding_table[(triple >> 2 * 6) & 0x3F];
		coded_base64 += encoding_table[(triple >> 1 * 6) & 0x3F];
		coded_base64 += encoding_table[(triple >> 0 * 6) & 0x3F];
	}

	int nForLoopCount = mod_table[input_length % 3];

	for (int n = 0; n < nForLoopCount; ++n)
	{
		coded_base64[return_length - 1 - n] = '=';
	}

	return return_length;
}

static size_t base64_decode(const char *data,
							size_t input_length,
							std::string& decoded_base64)
{
	if(data == nullptr)
	{
		return 0;
	}

	////디코딩 테이블은 미리 생성했습니다.
	//char decoding_table[256] = { 0, };

	////디코딩 테이블 생성
	//for (int n = 0; n < 64; ++n)
	//{
	//	unsigned char nArrayNum = (unsigned char)encoding_table[n];
	//	decoding_table[nArrayNum] = n;
	//}

	if(input_length % 4 != 0)
	{
		return 0;
	}

	size_t return_length = input_length / 4 * 3;
	if(data[input_length - 1] == '=')
	{
		--return_length;
	}

	if(data[input_length - 2] == '=')
	{
		--return_length;
	}

	for (size_t n = 0, j = 0; n < input_length;)
	{

		uint32_t sextet_a = data[n] == '=' ? 0 & n++ : decoding_table[data[n++]];
		uint32_t sextet_b = data[n] == '=' ? 0 & n++ : decoding_table[data[n++]];
		uint32_t sextet_c = data[n] == '=' ? 0 & n++ : decoding_table[data[n++]];
		uint32_t sextet_d = data[n] == '=' ? 0 & n++ : decoding_table[data[n++]];

		uint32_t triple = (sextet_a << 3 * 6)
						+ (sextet_b << 2 * 6)
						+ (sextet_c << 1 * 6)
						+ (sextet_d << 0 * 6);

		if(j < return_length)
		{
			decoded_base64 += (triple >> 2 * 8) & 0xFF;
			++j;
		}

		if(j < return_length)
		{
			decoded_base64 += (triple >> 1 * 8) & 0xFF;
			++j;
		}

		if(j < return_length)
		{
			decoded_base64 += (triple >> 0 * 8) & 0xFF;
			++j;
		}
	}

	return return_length;
}

#define HEAP_STANDARD 0
#define HEAP_LAL 1
#define HEAP_LFH 2

#ifdef Netlib_global_extern_initialize

wchar_t global_lfh_buffer[1024];
std::vector<std::wstring> global_lfh_log;

#else

extern wchar_t global_lfh_buffer[1024];
extern std::vector<std::wstring> global_lfh_log;

#endif

static int CheckUsingLFH(BOOL& bUsingLFH)
{
	std::vector<ULONG> heap_list;
	BOOL bResult;
	HANDLE heaps[1025];
	ULONG HeapInformation;
	bUsingLFH = FALSE;

	//
	// Get a handle to the default process heap.
	//
	DWORD nheaps = GetProcessHeaps((sizeof(heaps) / sizeof(HANDLE)) - 1, heaps);
	for (DWORD i = 0; i < nheaps; ++i)
	{
		if (heaps[i] == NULL)
		{
			DWORD dwError = GetLastError();
			swprintf_s(global_lfh_buffer, L"Failed to retrieve default process heap with LastError %d.\n", dwError);
			global_lfh_log.push_back(global_lfh_buffer);

			GetErrorString(global_lfh_buffer, 1024, dwError);
			global_lfh_log.push_back(global_lfh_buffer);
			return 1;
		}

		//
		// Query heap features that are enabled.
		//
		bResult = HeapQueryInformation(heaps[i],
			HeapCompatibilityInformation,
			&HeapInformation,
			sizeof(HeapInformation),
			NULL);

		if (bResult == FALSE)
		{
			DWORD dwError = GetLastError();
			swprintf_s(global_lfh_buffer, L"Failed to retrieve heap features with LastError %d.\n", dwError);
			global_lfh_log.push_back(global_lfh_buffer);

			GetErrorString(global_lfh_buffer, 1024, dwError);
			global_lfh_log.push_back(global_lfh_buffer);
			return 1;
		}

		//
		// Print results of the query.
		//
		/*swprintf_s(global_lfh_buffer, L"HeapCompatibilityInformation is %d.\n", HeapInformation);
		global_lfh_log.push_back(global_lfh_buffer);*/

		switch (HeapInformation)
		{
		case HEAP_STANDARD:
			swprintf_s(global_lfh_buffer, L"The default process heap is a standard heap.\n");
			global_lfh_log.push_back(global_lfh_buffer);
			break;
		case HEAP_LAL:
			swprintf_s(global_lfh_buffer, L"The default process heap supports look-aside lists.\n");
			global_lfh_log.push_back(global_lfh_buffer);
			break;
		case HEAP_LFH:
			swprintf_s(global_lfh_buffer, L"The default process heap has the low - fragmentation heap enabled.\n");
			global_lfh_log.push_back(global_lfh_buffer);
			break;
		default:
			swprintf_s(global_lfh_buffer, L"Unrecognized HeapInformation reported for the default process heap.\n");
			global_lfh_log.push_back(global_lfh_buffer);
			break;
		}

		heap_list.push_back(HeapInformation);
	}

	// 전체 힙이 LFH사용 중일 경우에만 TRUE로 처리 합니다.
	for (size_t n = 0; n < heap_list.size(); ++n)
	{
		if (heap_list[n] != HEAP_LFH)
		{
			bUsingLFH = FALSE;
			return 0;
		}
	}

	bUsingLFH = TRUE;
	return 0;
}

static void SetLFH()
{
	// enablue LFH code, execute this code [Windows XP and Windows Server 2003] or lower than them
	HANDLE heaps[1025];
	ULONG HeapInformation;
	DWORD nheaps = GetProcessHeaps((sizeof(heaps) / sizeof(HANDLE)) - 1, heaps);
	for (DWORD i = 0; i < nheaps; ++i)
	{
		BOOL bResult = HeapQueryInformation(heaps[i],
			HeapCompatibilityInformation,
			&HeapInformation,
			sizeof(HeapInformation),
			NULL);

		if (bResult == FALSE)
		{
			DWORD dwError = GetLastError();
			swprintf_s(global_lfh_buffer, L"Failed to retrieve heap features with LastError %d.\n", dwError);
			global_lfh_log.push_back(global_lfh_buffer);

			GetErrorString(global_lfh_buffer, 1024, dwError);
			global_lfh_log.push_back(global_lfh_buffer);
			continue;
		}

		// 이미 LFH인 경우에는 HeapSetInformation 시도 하지 않음
		if (HeapInformation == HEAP_LFH)
			continue;

		ULONG HeapInformation = HEAP_LFH;
		BOOL bReturn = HeapSetInformation(heaps[i], HeapCompatibilityInformation, &HeapInformation, sizeof(HeapInformation));
		if (bReturn != FALSE)
		{
			swprintf_s(global_lfh_buffer, L"The low-fragmentation heap has been enabled.\n");
			global_lfh_log.push_back(global_lfh_buffer);
		}
		else
		{
			DWORD dwError = GetLastError();
			swprintf_s(global_lfh_buffer, L"Failed to enable the low-fragmentation heap with LastError %d.\n", dwError);
			global_lfh_log.push_back(global_lfh_buffer);

			GetErrorString(global_lfh_buffer, 1024, dwError);
			global_lfh_log.push_back(global_lfh_buffer);
		}
	}
}

static void PrintLFHLog()
{
	for (size_t n = 0; n < global_lfh_log.size(); ++n)
	{
		wprintf(global_lfh_log[n].c_str());
	}
}

static void ActivateLFH(BOOL bUsingLFH = FALSE)
{
	global_lfh_log.clear();

	swprintf_s(global_lfh_buffer, L"=========== first check LFH\n");
	global_lfh_log.push_back(global_lfh_buffer);

	CheckUsingLFH(bUsingLFH);

	// LFH를 전체 적용중이 아닐경우 전체 사용 하도록 변경
	if (!bUsingLFH)
	{
		swprintf_s(global_lfh_buffer, L"=========== set LFH to all process heap\n");
		global_lfh_log.push_back(global_lfh_buffer);
		SetLFH();

		swprintf_s(global_lfh_buffer, L"=========== second check LFH\n");
		global_lfh_log.push_back(global_lfh_buffer);
		CheckUsingLFH(bUsingLFH);
	}

	swprintf_s(global_lfh_buffer, L"=========== finish check LFH\n");
	global_lfh_log.push_back(global_lfh_buffer);

	PrintLFHLog();
}

static size_t safe_strlen(const char *str, size_t max_len)
{
	const char * end = (const char *)memchr(str, '\0', max_len);
	if (end == NULL)
		return max_len;
	else
		return end - str;
}

static tstring GetDomainToIP(const TCHAR* domain_addr)
{
	TCHAR szServerIP[INET_ADDRSTRLEN] = {0, };

#ifdef UNICODE
	ADDRINFOW addrInfo;
	ADDRINFOW* pAddrInfo = NULL;
#else
	ADDRINFO addrInfo;
	ADDRINFO* pAddrInfo = NULL;
#endif
	
	ZeroMemory(&addrInfo, sizeof(addrInfo));
	addrInfo.ai_family = AF_UNSPEC;
	addrInfo.ai_socktype = SOCK_STREAM;
	addrInfo.ai_protocol = IPPROTO_TCP;

	
	GetAddrInfo(domain_addr, NULL, &addrInfo, &pAddrInfo);
	DWORD szIPAddrLen = INET_ADDRSTRLEN;
	if(nullptr != pAddrInfo)
		WSAAddressToString(pAddrInfo->ai_addr, static_cast<DWORD>(pAddrInfo->ai_addrlen), NULL, szServerIP, &szIPAddrLen);

	return tstring(szServerIP);
}

/*
returns the utc timezone offset
(e.g. -8 hours for PST)
*/
static int get_utc_offset()
{
	struct tm newtime;
	__time64_t zero = 24 * 60 * 60L;
	int gmtime_hours;

	/* get the local time for Jan 2, 1900 00:00 UTC */
	_localtime64_s(&newtime, &zero);
	gmtime_hours = newtime.tm_hour;

	/* if the local time is the "day before" the UTC, subtract 24 hours
	from the hours to get the UTC offset */
	if (newtime.tm_mday < 2)
		gmtime_hours -= 24;

	return gmtime_hours;
}

static DWORD CountSetBits(ULONG_PTR bitMask)
{
	DWORD LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
	DWORD bitSetCount = 0;
	ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;
	DWORD i;

	for (i = 0; i <= LSHIFT; ++i) {
		bitSetCount += ((bitMask & bitTest) ? 1 : 0);
		bitTest /= 2;
	}

	return bitSetCount;
}

//출처: http://codeng.tistory.com/1 [도전!]
static int GetProcessorNumber(int& core, int& logical) 
{
	typedef BOOL(WINAPI *LPFN_GLPI)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);
	LPFN_GLPI glpi;

	DWORD returnLength = 0;
	DWORD logicalProcessorCount = 0;
	DWORD numaNodeCount = 0;
	DWORD processorCoreCount = 0;
	DWORD processorL1CacheCount = 0;
	DWORD processorL2CacheCount = 0;
	DWORD processorL3CacheCount = 0;
	DWORD processorPackageCount = 0;
	DWORD byteOffset = 0;
	PCACHE_DESCRIPTOR Cache;

	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;

	core = logical = 0;

	glpi = (LPFN_GLPI)GetProcAddress(GetModuleHandleA("kernel32"), "GetLogicalProcessorInformation");
	if (NULL == glpi) {
		printf("\nGetLogicalProcessorInformation is not supported.\n");
		return (0);
	}

	DWORD rc = glpi(buffer, &returnLength);
	if (FALSE == rc) {
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
			if (buffer)
				free(buffer);

			buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(returnLength);
		}
		rc = glpi(buffer, &returnLength);
	}

	if (FALSE == rc) {
		printf("\nError: GetLogicalProcessorInformation() returned false\n");
		return (0);
	}

	ptr = buffer;
	while (byteOffset < returnLength) {
		switch (ptr->Relationship)
		{
		case RelationNumaNode:
			numaNodeCount++;
			break;

		case RelationProcessorCore:
			processorCoreCount++;
			logicalProcessorCount += CountSetBits(ptr->ProcessorMask);
			break;

		case RelationCache:
			Cache = &ptr->Cache;
			if (Cache->Level == 1) {
				processorL1CacheCount++;
			}
			else if (Cache->Level == 2) {
				processorL2CacheCount++;
			}
			else if (Cache->Level == 3) {
				processorL3CacheCount++;
			}
			break;

		case RelationProcessorPackage:
			processorPackageCount++;
			break;

		default:
			printf("\nError: Unsupported LOGICAL_PROCESSOR_RELATIONSHIP value.\n");
			break;
		}
		byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
		ptr++;
	}
	free(buffer);
	core = processorCoreCount;
	logical = logicalProcessorCount;
	return 1;
}

/*
 * 18. Asm 폴더
 * StackPointer.asm 에 정의되어있습니다.
*/

typedef LPBYTE(*fpStackPointer)();

#ifdef _M_AMD64
extern "C" LPBYTE GetStackPointerX64();
static const fpStackPointer g_StackPointer = &GetStackPointerX64;
#else
extern "C" LPBYTE GetStackPointerX86();
static const fpStackPointer g_StackPointer = &GetStackPointerX86;
#endif

static MEMORY_BASIC_INFORMATION GetStackPointerInfo()
{
	LPBYTE lpByte = g_StackPointer();

	MEMORY_BASIC_INFORMATION mbi;

	VirtualQuery(lpByte, &mbi, sizeof(mbi));

	return mbi;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined( _DEBUG )
#define Assert(exp, desc) assert(exp && desc)
#else
#define Assert(exp, desc) if( exp == 0 )MSG_BOX( desc);
#endif

namespace util
{

	typedef void(*fpCtrlHandler)(BOOL);

#ifdef Netlib_global_extern_initialize
	fpCtrlHandler g_fpCtrlHandler;

	BOOL CtrlHandler(DWORD fdwCtrlType)
	{
		switch (fdwCtrlType)
		{
		case CTRL_C_EVENT:
			break;
		case CTRL_CLOSE_EVENT:	//	콘솔 X 눌렀을때
		{

			for (std::vector<std::wstring>::iterator iter = global_lfh_log.begin(); iter != global_lfh_log.end();)
			{
				(*iter).clear();

				iter = global_lfh_log.erase(iter);
			}

			global_lfh_log.clear();

			if (g_fpCtrlHandler != nullptr)
			{
				g_fpCtrlHandler(FALSE);
			}
		}
		break;
		case CTRL_BREAK_EVENT:
			break;
		case CTRL_LOGOFF_EVENT:
			break;
		case CTRL_SHUTDOWN_EVENT:
			break;
		default:
			break;
		}

		return FALSE;
	}

	bool SetCtrlHandler(fpCtrlHandler fp)
	{
		if (fp != nullptr)
		{
			g_fpCtrlHandler = fp;
			return ::SetConsoleCtrlHandler(reinterpret_cast<PHANDLER_ROUTINE>(CtrlHandler), TRUE) != 0;
		}

		return false;
	}


#else
	extern fpCtrlHandler g_fpCtrlHandler;
	extern bool SetCtrlHandler(fpCtrlHandler fp);
	extern BOOL CtrlHandler(DWORD fdwCtrlType);
#endif

	template <class Parent, class Child>
	class up_casting_checker
	{
	private:
		static short up_casting_checker_function(Parent*);
		static char up_casting_checker_function(...);

	public:
		enum
		{
			Valid = sizeof(up_casting_checker_function((Child*)nullptr)) - 1,
		};
	};

	template<class Parent, class Child>
	int compile_time_up_casting_checker()
	{
		up_casting_checker<Parent, Child> checker;
		return checker.Valid;
	}

	template<class child>
	static child* down_casting_func(void* parent)
	{
		// 임시 변수를 생성하여 "l-value"를 사용합니다.
		child tmp;
		if (*(size_t**)parent == *(size_t**)&tmp)
		{
			return reinterpret_cast<child*>(parent);
		}

		return nullptr;
	}

	/*
	emplate<class child>
	static child* down_casting_func(void* parent)
	{
		if (*(size_t**)parent == *(size_t**)&child()) // 오류	C2102	'&'에 l-value가 있어야 합니다.
		{
			return reinterpret_cast<child*>(parent);
		}

		return nullptr;
	}
	
	*/
	
};