#pragma once
#define _WIN32_DCOM

#include "../Common/Netlib.h"
#include <comdef.h>
#include <WbemIdl.h>
#include <Pdh.h>
#include <PdhMsg.h>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "pdh.lib")

#define	DEF_128B										128
#define	DEF_256B										256
#define	DEF_512B										512
#define DEF_1KB											1024
#define	DEF_2KB											(1024 * 2)
#define DEF_4KB											(1024 * 4)
#define	DEF_8KB											(1024 * 8)
#define DEF_16KB										(1024 * 16)
#define DEF_24KB										(1024 * 24)
#define DEF_32KB										(1024 * 32)
#define DEF_63KB										(1024 * 63)
#define DEF_64KB										(1024 * 64)
#define DEF_128KB									(1024 * 128)

BEGIN_NETLIB

class cResourceInfo
{
private:
	bool					m_load_COM;

	IWbemLocator			*m_pLocator;
	IWbemServices			*m_pService;

	PROCESS_RESOURCE_DATA	m_prev_data;

	ULONGLONG				m_prev_system_network_recv_bytes;
	ULONGLONG				m_prev_system_network_send_bytes;

	// for system cpu usage
	HQUERY					m_system_cpu_query;
	HCOUNTER				m_system_cpu_counter;

	// for process cpu usage
	HQUERY					m_process_cpu_query;
	//BYTE					m_process_cpu_counter_cnt;
	HCOUNTER				m_process_cpu_counter_list[SERVER_TYPE_MAX];
	MANAGE_INFO				m_manage_list[SERVER_TYPE_MAX];

public:
	// server func
	bool					Init();
	void					Free();
	void					Clear();
	bool					CheckStatus(PDH_STATUS status);
	int						VariantToString(VARIANT * v, TCHAR * sz, int len, size_t szArraySize);
	int						BSTRtoTCHAR(BSTR bstr, TCHAR * p_char, size_t p_charArraySize);

	bool					GetResource(TCHAR* exeFilename, std::vector<WORD>& sid_list, std::vector<PROCESS_RESOURCE_DATA>& data_list);
	bool					AgentGetResource(std::vector<WORD>& sid_list, std::vector<PROCESS_RESOURCE_DATA>& data_list);

	bool					GetSystemCpuUsage(BYTE& CPUUsage);
	bool					GetSystemTotalMemory(DWORD& dwTotalMemoryCapacity);
	bool					GetSystemNetworkTraffic(DWORD& dwInputBytes, DWORD& dwOutputBytes);
	bool					GetProcessUseMemory(TCHAR * p_server_name, DWORD& dwMemoryUsage);
	bool					GetProcessCpuUsage(BYTE idx, TCHAR * p_server_name, double& CPUUsage);

	void					Update_ManageInfo(const WORD server_type, const WORD SID);// 매니저 서버에서 SID 리스트를 전송해준다.

	void					TestPDH(TCHAR* processName);
	bool					GetProcessCpuUsage(TCHAR * p_server_name, double& CPUUsage);

public:
	cResourceInfo();
	virtual ~cResourceInfo();
};

END_NETLIB