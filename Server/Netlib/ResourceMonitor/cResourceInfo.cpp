#include "../../Include/Netlib/ResourceMonitor/cResourceInfo.h"
#include "../../Include/Netlib/Queue/cLogQueue.h"
#include "../../Include/Netlib/Common/cSingleton.h"

using namespace NetLib;

cResourceInfo::cResourceInfo() : 
	m_load_COM(false), 
	m_pLocator(NULL), 
	m_pService(NULL), 
	m_prev_system_network_recv_bytes(NULL),
	m_prev_system_network_send_bytes(NULL), 
	m_system_cpu_query(NULL), 
	m_system_cpu_counter(NULL), 
	m_process_cpu_query(NULL)
{
	//TestPDH(_T("devenv"));
	ZeroMemory(m_process_cpu_counter_list, sizeof(m_process_cpu_counter_list));
	ZeroMemory(m_manage_list, sizeof(m_manage_list));

	for (int n = 0; n<SERVER_TYPE_MAX; ++n)
	{
		m_manage_list[n].server_type = n;

		switch (n)
		{
		case LOBBY_SERVER:
			_tcscpy_s(m_manage_list[n].server_file_name, _T("LobbyServer.exe"));
			break;
		/*case WEBSERVER:
			_tcscpy_s(m_manage_list[n].server_file_name, _T("W3SVC.exe")); // IIS 
			break;*/
			//case GLOBAL_FTP: // NeoMatchingServer 때문에 임시로 이렇게 씁니다. FTP의 리소스를 모니터링 할일은 없다고 봅니다.
			//	_tcscpy(m_manage_list[n].server_file_name, _T("NeoMatchingServer.exe"));
			//	break;
		default:
			break;
		}
	}

	Free();
}

cResourceInfo::~cResourceInfo()
{
	Free();
}

// 매니저 서버에서 SID 리스트를 전송해준다.
void cResourceInfo::Update_ManageInfo(const WORD server_type, const WORD SID)
{
	for (int n = 0; n<E_SERVER_TYPE::SERVER_TYPE_MAX; ++n)
	{
		if(m_manage_list[n].server_type == server_type)
			m_manage_list[n].server_id = SID;
	}
}

bool cResourceInfo::Init()
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//															WMI Init
	//
	// 0. initialize COM
	if(!m_load_COM)
	{
		HRESULT hres = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if(FAILED(hres))
		{
			cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("%s - CoInitializeEx(), Error code = 0x%08x"), _T(__FUNCTION__), hres);
			return false;
		}

		// 1. Initialize security
		hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);

		if(FAILED(hres) && (RPC_E_TOO_LATE != hres))
		{
			cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("%s - CoInitializeSecurity(),. Error code = 0x%08x"), _T(__FUNCTION__), hres);
			Clear();
			return false;
		}

		m_load_COM = true;

		// 2. set locator
		hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&m_pLocator);

		if(FAILED(hres))
		{
			cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("%s - CoCreateInstance(), Error code = 0x%08x"), _T(__FUNCTION__), hres);
			Clear();
			return false;
		}

		// 3. connect local
		hres = m_pLocator->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &m_pService);

		if(FAILED(hres))
		{
			cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("%s - ConnectServer(), Error code = 0x%08x"), _T(__FUNCTION__), hres);
			Clear();
			return false;
		}

		// 4. set service proxy
		hres = CoSetProxyBlanket(m_pService, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

		if(FAILED(hres))
		{
			cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("%s - CoSetProxyBlanket(), Error code = 0x%08x"), _T(__FUNCTION__), hres);
			Clear();
			return false;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//															PDH Init
	//
	// 5. set system cpu query & counter
	if(!m_system_cpu_query)
	{
		// 0. open system cpu query
		PDH_STATUS status = PdhOpenQuery(NULL, (DWORD_PTR)NULL, &m_system_cpu_query);
		if(status != ERROR_SUCCESS)
		{
			cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("%s - system cpu : PdhOpenQuery(), Error code = 0x%08x"), _T(__FUNCTION__), status);
			Clear();
			return false;
		}

		// 1. add system cpu counter
		status = PdhAddCounter(m_system_cpu_query, _T("\\Processor(_Total)\\% Processor Time"), 0, &m_system_cpu_counter);
		if(status != ERROR_SUCCESS)
		{
			cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("%s - system cpu : PdhAddCounter(), Error code = 0x%08x"), _T(__FUNCTION__), status);

			PdhRemoveCounter(m_system_cpu_counter);
			m_system_cpu_counter = NULL;

			PdhCloseQuery(m_system_cpu_query);
			m_system_cpu_query = NULL;

			Clear();
			return false;
		}

		// 2. execute query(read a performance data record)
		PdhCollectQueryData(m_system_cpu_query);
	}

	// 6. get process cpu query & counter
	if(!m_process_cpu_query)
	{
		// 0. open process cpu query
		PDH_STATUS status = PdhOpenQuery(NULL, (DWORD_PTR)NULL, &m_process_cpu_query);
		if(status != ERROR_SUCCESS)
		{
			cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("%s - process cpu : PdhOpenQuery(), Error code = 0x%08x"), _T(__FUNCTION__), status);
			Clear();
			return false;
		}

		// 1. add process cpu counter
		for (int i = 0; i < SERVER_TYPE_MAX; i++)
		{
			if(_tcslen(m_manage_list[i].server_file_name) == 0)
				continue;

			TCHAR szPath[MAX_PATH] = { 0, };
			TCHAR szFileName[CSDef::MAX_DEF_LEN_SERVER_NAME] = { 0, };
			TCHAR* szNext = nullptr;
			_tcscpy_s(szFileName, m_manage_list[i].server_file_name);
			_sntprintf_s(szPath, sizeof(szPath), _T("\\Process(%s)\\%c Processor Time"), _tcstok_s(szFileName,_T("."),&szNext), _T('%'));
			//_sntprintf(szPath, sizeof(szPath), _T("\\Process(explorer)\\%% Processor Time"), _tcstok(szFileName, _T("."))) ;
			PDH_HCOUNTER phCounter;
			//PDH_STATUS status = PdhAddCounter(m_process_cpu_query, szPath, 0, &m_process_cpu_counter_list[i]);
			PDH_STATUS status = PdhAddCounter(m_process_cpu_query, szPath, 0, &phCounter);
			if(status == ERROR_SUCCESS)
			{
				m_process_cpu_counter_list[i] = phCounter;
			}
			else
			{
				_tprintf(_T("cResourceInfo::Init() - process cpu : PdhAddCounter(), %s, Error code = 0x%08x"), szPath, status);

				while (i >= 0)
				{
					if(m_process_cpu_counter_list[i])
					{
						PdhRemoveCounter(m_process_cpu_counter_list[i]);
						m_process_cpu_counter_list[i] = NULL;
					}
					i--;
				}

				PdhCloseQuery(m_process_cpu_query);
				m_process_cpu_query = NULL;

				// with system cpu
				PdhRemoveCounter(m_system_cpu_counter);
				m_system_cpu_counter = NULL;

				PdhCloseQuery(m_system_cpu_query);
				m_system_cpu_query = NULL;

				Clear();
				return false;
			}
		}

		// 2. execute query(read a performance data record)
		PdhCollectQueryData(m_process_cpu_query);
	}

	// 7. wait a minimum of one second between collections
	//Sleep(1000);

	return true;
}

void cResourceInfo::Free()
{
	// 0. clear system cpu query & counter
	if(m_system_cpu_query)
	{
		PdhRemoveCounter(m_system_cpu_counter);
		m_system_cpu_counter = NULL;

		PdhCloseQuery(m_system_cpu_query);
		m_system_cpu_query = NULL;
	}

	// 1. clear process cpu query & counter
	if(m_process_cpu_query)
	{
		for (int i = 0; i < SERVER_TYPE_MAX; i++)
		{
			PdhRemoveCounter(m_process_cpu_counter_list[i]);
			m_process_cpu_counter_list[i] = NULL;
		}

		PdhCloseQuery(m_process_cpu_query);
		m_process_cpu_query = NULL;
	}

	// 2. clear COM(WMI)
	Clear();
}

void cResourceInfo::Clear()
{
	ZeroMemory(&m_prev_data, sizeof(m_prev_data));

	if(NULL != m_pService)
	{
		m_pService->Release();
		m_pService = NULL;
	}

	if(NULL != m_pLocator)
	{
		m_pLocator->Release();
		m_pLocator = NULL;
	}

	if(m_load_COM)
	{
		CoUninitialize();
		m_load_COM = false;
	}
}

int cResourceInfo::VariantToString(VARIANT * v, TCHAR * sz, int len, size_t szArraySize)
{
	if(!v || !sz) return -1;

	switch (v->vt)
	{
	case VT_NULL:
	{
		_stprintf_s(sz, szArraySize, _T("NULL"));
	}
	break;

	case VT_BOOL:
	{
		if(TRUE == V_BOOL(v))
			_stprintf_s(sz, szArraySize, _T("TRUE"));
		else
			_stprintf_s(sz, szArraySize, _T("FALSE"));
	}
	break;

	case VT_I4:
	{
		_stprintf_s(sz, szArraySize, _T("%d"), V_I4(v));
	}
	break;

	case (VT_I4 | VT_ARRAY):
	{
		long i = 0;
		signed int lv = 0;
		SAFEARRAY * saArray = NULL;

		saArray = V_ARRAY(v);

		if(NULL == saArray) return -1;

		HRESULT hRes = SafeArrayGetElement(saArray, &i, &lv);
		if(hRes != S_OK) return -1;

		_stprintf_s(sz, szArraySize, _T("%d"), lv);
	}
	break;

	case VT_BSTR:
	{
		BSTR wsztmp(V_BSTR(v));

		if(NULL == wsztmp) return -1;

		BSTRtoTCHAR(wsztmp, sz, szArraySize);
	}
	break;

	case (VT_BSTR | VT_ARRAY):
	{
		long i = 0;
		BSTR wsztmp = NULL;
		SAFEARRAY * saArray = NULL;

		saArray = V_ARRAY(v);

		if(NULL == saArray) return -1;

		HRESULT hRes = SafeArrayGetElement(saArray, &i, &wsztmp);
		if(hRes != S_OK) return -1;

		BSTRtoTCHAR(wsztmp, sz, szArraySize);
	}
	break;

	default:
		break;
	} //switch

	return 0;
}

int cResourceInfo::BSTRtoTCHAR(BSTR bstr, TCHAR * p_char, size_t p_charArraySize)
{
	if(NULL == bstr || NULL == p_char) return -1;

	_bstr_t tmp(bstr);
	
	_stprintf_s(p_char, p_charArraySize, _T("%s"), (LPCTSTR)tmp);

	return 0;
}

bool cResourceInfo::GetResource(TCHAR* exeFilename, std::vector<WORD>& sid_list, std::vector<PROCESS_RESOURCE_DATA>& data_list)
{
	if(exeFilename == nullptr)
		return false;

	if(!m_load_COM)
		if(!Init())
			return false;

	PROCESS_RESOURCE_DATA res_data;
	ZeroMemory(&res_data, sizeof(res_data));

	// 0. get system cpu usage
	if(!GetSystemCpuUsage(res_data.sys_use_cpu))
		res_data.sys_use_cpu = m_prev_data.sys_use_cpu;

	// 1. get system total memory
	if(!GetSystemTotalMemory(res_data.sys_total_mem))
		res_data.sys_total_mem = m_prev_data.sys_total_mem;

	// 2. get system network traffic
	if(!GetSystemNetworkTraffic(res_data.recv_bytes, res_data.send_bytes))
	{
		res_data.recv_bytes = m_prev_data.recv_bytes;
		res_data.send_bytes = m_prev_data.send_bytes;
	}

	for (int i = 0; i < SERVER_TYPE_MAX; i++)
	{
		// 0. compare execute file name with, server_file_name
		// if not same then do continue
		if(_tcscmp(exeFilename, m_manage_list[i].server_file_name) != 0)
			continue;

		// 1. get process cpu usage
		//if( !GetProcessCpuUsage(i, m_manage_list[i].server_file_name, res_data.proc_use_cpu ) )	
		GetProcessCpuUsage(m_manage_list[i].server_file_name, res_data.proc_use_cpu);
		res_data.proc_use_cpu = m_prev_data.proc_use_cpu;

		// 2. get process use memory
		if(!GetProcessUseMemory(m_manage_list[i].server_file_name, res_data.proc_use_mem))
			res_data.proc_use_mem = m_prev_data.proc_use_mem;

		_tcscpy_s(res_data.server_file_name, m_manage_list[i].server_file_name);
		sid_list.push_back(m_manage_list[i].server_id);
		data_list.push_back(res_data);
	}

	return true;
}

bool cResourceInfo::AgentGetResource(std::vector<WORD>& sid_list, std::vector<PROCESS_RESOURCE_DATA>& data_list)
{
	if(!m_load_COM)
		if(!Init())
			return false;

	PROCESS_RESOURCE_DATA res_data;
	ZeroMemory(&res_data, sizeof(res_data));

	// 0. get system cpu usage
	if(!GetSystemCpuUsage(res_data.sys_use_cpu))
		res_data.sys_use_cpu = m_prev_data.sys_use_cpu;

	// 1. get system total memory
	if(!GetSystemTotalMemory(res_data.sys_total_mem))
		res_data.sys_total_mem = m_prev_data.sys_total_mem;

	// 2. get system network traffic
	if(!GetSystemNetworkTraffic(res_data.recv_bytes, res_data.send_bytes))
	{
		res_data.recv_bytes = m_prev_data.recv_bytes;
		res_data.send_bytes = m_prev_data.send_bytes;
	}

	for (int i = 0; i < SERVER_TYPE_MAX; i++)
	{
		// 0. if server_file_name is empty then do continue
		if(_tcslen(m_manage_list[i].server_file_name) == 0)
			continue;

		// 1. get process cpu usage
		if(!GetProcessCpuUsage(i, m_manage_list[i].server_file_name, res_data.proc_use_cpu))
			res_data.proc_use_cpu = m_prev_data.proc_use_cpu;

		// 2. get process use memory
		if(!GetProcessUseMemory(m_manage_list[i].server_file_name, res_data.proc_use_mem))
			res_data.proc_use_mem = m_prev_data.proc_use_mem;

		_tcscpy_s(res_data.server_file_name, m_manage_list[i].server_file_name);
		res_data.server_id = m_manage_list[i].server_id;

		if(res_data.server_id)
		{
			sid_list.push_back(m_manage_list[i].server_id);
			data_list.push_back(res_data);
		}
	}

	return true;
}

bool cResourceInfo::GetSystemCpuUsage(BYTE& CPUUsage)
{
	PDH_STATUS status = PdhCollectQueryData(m_system_cpu_query);
	if(status != ERROR_SUCCESS)
	{
		if(status == PDH_NO_DATA)
		{
			return true;
		}
		else
		{
			cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("%s - PdhCollectQueryData(), Error code = 0x%08x"), _T(__FUNCTION__), status);
			return false;
		}
	}

	PDH_FMT_COUNTERVALUE counter_val;

	status = PdhGetFormattedCounterValue(m_system_cpu_counter, PDH_FMT_DOUBLE, (LPDWORD)NULL, &counter_val);
	if(status != ERROR_SUCCESS)
	{
		if(status == PDH_INVALID_DATA)
		{
			return true;
		}
		else
		{
			cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("%s - PdhGetFormattedCounterValue(), Error code = 0x%08x"), _T(__FUNCTION__), status);
			return false;
		}
	}

	CPUUsage = (BYTE)counter_val.doubleValue;

	m_prev_data.sys_use_cpu = CPUUsage;

	return true;
}

bool cResourceInfo::GetSystemTotalMemory(DWORD& dwTotalMemoryCapacity)
{
	IEnumWbemClassObject* pPhyMemEnumerator = NULL;
	HRESULT hresPhyMem = m_pService->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_PhysicalMemory"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pPhyMemEnumerator);

	if(FAILED(hresPhyMem))
	{
		cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("%s - Query for Win32_PhysicalMemory failed. Error code = 0x%08x"), _T(__FUNCTION__), hresPhyMem);
		return false;
	}
	else
	{
		IWbemClassObject *pclsObj = NULL;
		ULONG uReturn = 0;
		ULONGLONG ullMemSize = 0;

		while (TRUE)
		{
			HRESULT hResult = pPhyMemEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

			if(WBEM_S_NO_ERROR == hResult && 1 == uReturn)
			{
				VARIANT vtProp;
				VariantInit(&vtProp);

				// Get the value of the Name property
				hResult = pclsObj->Get(L"Capacity", 0, &vtProp, 0, 0);

				if(WBEM_S_NO_ERROR != hResult)
				{
					cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("%s - Get for Capacity failed. Error code = 0x%08x"), _T(__FUNCTION__), hResult);
					return false;
				}

				TCHAR szMemSize[DEF_128B] = { 0, };
				VariantToString(&vtProp, szMemSize, sizeof(szMemSize),sizeof(szMemSize) / sizeof(szMemSize[0]));		// 어떤 타입이든간에 일단 문자열로 변환한다

				ullMemSize += static_cast<ULONGLONG>(::_ttoi64(szMemSize));

				VariantClear(&vtProp);

				if(NULL != pclsObj)
				{
					pclsObj->Release();
					pclsObj = NULL;
				}
			}
			else
			{
				if(WBEM_S_FALSE != hResult)
				{
					cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI,
						_T("%s - Capacity pPhyMemEnumerator->Next() Failed. Error code = 0x%08x, uReturn Value : %d, GetLastError : %d\n"), _T(__FUNCTION__), hResult, uReturn, ::GetLastError());
					return false;
				}
				break;
			}
		}

		dwTotalMemoryCapacity = static_cast<DWORD>(ullMemSize / DEF_1KB);

		m_prev_data.sys_total_mem = dwTotalMemoryCapacity;
	}

	if(NULL != pPhyMemEnumerator)
	{
		pPhyMemEnumerator->Release();
		pPhyMemEnumerator = NULL;
	}

	return true;
}

bool cResourceInfo::GetSystemNetworkTraffic(DWORD& dwRecvBytes, DWORD& dwSendBytes)
{
	bool bSuccess = true;

	IEnumWbemClassObject* pEnumerator = NULL;
	HRESULT hres = m_pService->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_PerfRawData_Tcpip_NetworkInterface"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);

	if(FAILED(hres))
	{
		cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("%s - Query for Win32_PerfFormattedData_Tcpip_NetworkInterface failed. Error code = 0x%08x"), _T(__FUNCTION__), hres);
		bSuccess = false;           // Program has failed.
	}
	else
	{
		IWbemClassObject *pclsObj = NULL;
		ULONG uReturn = 0;
		ULONGLONG ullMostRecvBytes = 0;
		ULONGLONG ullMostSendBytes = 0;
		ULONGLONG ullCurRecvBytes = 0;
		ULONGLONG ullCurSendBytes = 0;

		// 0. get most recv/send device
		while (TRUE)
		{
			HRESULT hResult = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

			if(WBEM_S_NO_ERROR == hResult && 1 == uReturn)
			{
				VARIANT vtProp;
				VariantInit(&vtProp);

				// Get the value of the Name property
				hResult = pclsObj->Get(L"BytesReceivedPersec", 0, &vtProp, 0, 0);

				if(WBEM_S_NO_ERROR != hResult)
				{
					cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("%s - Get for BytesReceivedPersec failed. Error code = 0x%08x"), _T(__FUNCTION__), hResult);
					return false;
				}

				TCHAR szMemSize[DEF_128B] = { 0, };
				VariantToString(&vtProp, szMemSize, sizeof(szMemSize), sizeof(szMemSize) / sizeof(szMemSize[0]));		// 어떤 타입이든간에 일단 문자열로 변환한다

				ullCurRecvBytes = static_cast<ULONGLONG>(::_ttoi64(szMemSize));

				if(ullMostRecvBytes < ullCurRecvBytes)
					ullMostRecvBytes = ullCurRecvBytes;

				VariantClear(&vtProp);


				hResult = pclsObj->Get(L"BytesSentPersec", 0, &vtProp, 0, 0);

				if(WBEM_S_NO_ERROR != hResult)
				{
					cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI, _T("%s - Get for BytesSentPersec failed. Error code = 0x%08x"), _T(__FUNCTION__), hResult);
					return false;
				}

				ZeroMemory(szMemSize, DEF_128B);
				VariantToString(&vtProp, szMemSize, sizeof(szMemSize), sizeof(szMemSize) / sizeof(szMemSize[0]));		// 어떤 타입이든간에 일단 문자열로 변환한다

				ullCurSendBytes = static_cast<ULONGLONG>(::_ttoi64(szMemSize));

				if(ullMostSendBytes < ullCurSendBytes)
					ullMostSendBytes = ullCurSendBytes;

				VariantClear(&vtProp);

				if(NULL != pclsObj)
				{
					pclsObj->Release();
					pclsObj = NULL;
				}
			}
			else
			{
				if(WBEM_S_FALSE != hResult)
				{
					cSingleton<cLogQueue>::GetInstance()->PushCommand(LOG_CRI,
						_T("%s - pEnumerator->Next() Failed. Error code = 0x%08x, uReturn Value : %d, GetLastError : %d"), _T(__FUNCTION__), hResult, uReturn, ::GetLastError());
					bSuccess = false;
				}
				break;
			}
		}

		// 1. calc traffic
		DWORD cur_recv_bytes = static_cast<DWORD>(ullMostRecvBytes / DEF_1KB);
		DWORD cur_send_bytes = static_cast<DWORD>(ullMostSendBytes / DEF_1KB);

		// 2. is prev
		if(!m_prev_data.recv_bytes && !m_prev_data.send_bytes)
		{
			dwRecvBytes = 1;
			dwSendBytes = 1;
		}
		else
		{
			dwRecvBytes = static_cast<DWORD>(cur_recv_bytes - m_prev_system_network_recv_bytes);
			dwSendBytes = static_cast<DWORD>(cur_send_bytes - m_prev_system_network_send_bytes);
		}

		// 3. set cur bytes
		m_prev_system_network_recv_bytes = cur_recv_bytes;
		m_prev_system_network_send_bytes = cur_send_bytes;
		m_prev_data.recv_bytes = dwRecvBytes;
		m_prev_data.send_bytes = dwSendBytes;
	}

	if(NULL != pEnumerator)
	{
		pEnumerator->Release();
		pEnumerator = NULL;
	}

	return bSuccess;
}

bool cResourceInfo::GetProcessUseMemory(TCHAR * p_server_name, DWORD& dwMemoryUsage)
{
	bool bSuccess = true;

	TCHAR query[128] = { 0, };
	_sntprintf_s(query, sizeof(query), _T("SELECT * FROM Win32_Process WHERE Name = '%s'"), p_server_name);

	IEnumWbemClassObject* pEnumerator = NULL;
	HRESULT hres = m_pService->ExecQuery(bstr_t("WQL"), bstr_t(query), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);

	if(FAILED(hres))
	{
		_tprintf(_T("%s - Query for Win32_Process failed. Error code = 0x%08x, ProcessName : %s\n"), _T(__FUNCTION__), hres, p_server_name);
		bSuccess = false;           // Program has failed.
	}
	else
	{
		IWbemClassObject *pclsObj;
		ULONG uReturn = 0;
		__int64 ullMemSize = 0;

		while (TRUE)
		{
			HRESULT hResult = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

			if(WBEM_S_NO_ERROR == hResult && 1 == uReturn)
			{
				VARIANT vtProp;
				VariantInit(&vtProp);

				// Get the value of the Name property
				hResult = pclsObj->Get(L"WorkingSetSize", 0, &vtProp, 0, 0);

				if(WBEM_S_NO_ERROR != hResult)
				{
					_tprintf(_T("%s - Get for WorkingSetSize failed. Error code = 0x%08x, ProcessName : %s\n"), _T(__FUNCTION__), hResult, p_server_name);
					return false;
				}

				TCHAR szMemSize[DEF_128B] = { 0, };
				VariantToString(&vtProp, szMemSize, sizeof(szMemSize), sizeof(szMemSize) / sizeof(szMemSize[0]));		// 어떤 타입이든간에 일단 문자열로 변환한다

				ullMemSize += ::_ttoi64(szMemSize);

				VariantClear(&vtProp);

				if(NULL != pclsObj)
				{
					pclsObj->Release();
					pclsObj = NULL;
				}
			}
			else
			{
				if(WBEM_S_FALSE != hResult)
				{
					_tprintf(_T("%s - pEnumerator->Next() Failed. Error code = 0x%08x, uReturn Value : %d, GetLastError : %d, ProcessName : %s\n"),
						_T(__FUNCTION__), hResult, uReturn, ::GetLastError(), p_server_name);
					bSuccess = false;
				}
				break;
			}
		}

		dwMemoryUsage = static_cast<DWORD>(ullMemSize / DEF_1KB);

		m_prev_data.proc_use_mem = dwMemoryUsage;
	}

	if(NULL != pEnumerator)
	{
		pEnumerator->Release();
		pEnumerator = NULL;
	}

	return bSuccess;
}

bool cResourceInfo::GetProcessCpuUsage(BYTE idx, TCHAR* p_server_name, double& CPUUsage)
{
	if(idx == 0)
	{
		PDH_STATUS status = PdhCollectQueryData(m_process_cpu_query);
		if(status != ERROR_SUCCESS)
		{
			if(status == PDH_NO_DATA)
			{
				return true;
			}
			else
			{
				_tprintf(_T("%s - PdhCollectQueryData(), Error code = 0x%08x"), _T(__FUNCTION__), status);
				return false;
			}
		}
	}

	PDH_FMT_COUNTERVALUE counter_val;

	PDH_STATUS status = PdhGetFormattedCounterValue(m_process_cpu_counter_list[idx], PDH_FMT_DOUBLE, NULL, &counter_val);
	if(status != ERROR_SUCCESS)
	{
		if(status == PDH_INVALID_DATA)
		{
			//_tprintf(_T("cResourceInfo::GetProcessCpuUsage - PdhGetFormattedCounterValue(), Error code = 0x%08x, ProcessName : %s\n"), status, p_server_name);
			//CheckStatus(status);
			return true;
		}
		else
		{
			if(status != PDH_CALC_NEGATIVE_VALUE)
				_tprintf(_T("cResourceInfo::GetProcessCpuUsage - PdhGetFormattedCounterValue(), Error code = 0x%08x, ProcessName : %s\n"), status, p_server_name);
			return false;
		}
	}

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	CPUUsage = counter_val.doubleValue / (double)si.dwNumberOfProcessors;

	m_prev_data.proc_use_cpu = CPUUsage;

	return true;
}

bool cResourceInfo::CheckStatus(PDH_STATUS status)
{
	switch (status)
	{
	case ERROR_SUCCESS:
		return true;

		// general

	case PDH_INVALID_ARGUMENT:
		printf("CPUUsageWin32PDH(), status => PDH_INVALID_ARGUMENT\n");
		break;

	case PDH_INVALID_HANDLE:
		printf("CPUUsageWin32PDH(), status => PDH_INVALID_HANDLE\n");
		break;

		// PdhAddCounter

	case PDH_CSTATUS_BAD_COUNTERNAME:
		printf("CPUUsageWin32PDH(), status => PDH_CSTATUS_BAD_COUNTERNAME\n");
		break;

	case PDH_CSTATUS_NO_COUNTER:
		printf("CPUUsageWin32PDH(), status => PDH_CSTATUS_NO_COUNTER\n");
		break;

	case PDH_CSTATUS_NO_COUNTERNAME:
		printf("CPUUsageWin32PDH(), status => PDH_CSTATUS_NO_COUNTERNAME\n");
		break;

	case PDH_CSTATUS_NO_MACHINE:
		printf("CPUUsageWin32PDH(), status => PDH_CSTATUS_NO_MACHINE\n");
		break;

	case PDH_CSTATUS_NO_OBJECT:
		printf("CPUUsageWin32PDH(), status => PDH_CSTATUS_NO_OBJECT\n");
		break;

	case PDH_FUNCTION_NOT_FOUND:
		printf("CPUUsageWin32PDH(), status => PDH_FUNCTION_NOT_FOUND\n");
		break;

	case PDH_MEMORY_ALLOCATION_FAILURE:
		printf("CPUUsageWin32PDH(), status => PDH_MEMORY_ALLOCATION_FAILURE\n");
		break;

		// PdhGetFormattedCounterValue

	case PDH_INVALID_DATA:
		printf("CPUUsageWin32PDH(), status => PDH_INVALID_DATA\n");
		break;

	default:
		printf("CPUUsageWin32PDH(), status => UNKNOWN\n");
		break;
	}

	return false;
}

bool cResourceInfo::GetProcessCpuUsage(TCHAR * p_server_name, double& CPUUsage)
{
	CPUUsage = 0.0f;

	HQUERY  hQuery;

	PDH_STATUS lStatus = PdhOpenQuery(NULL, NULL, &hQuery);
	if(lStatus != ERROR_SUCCESS)
		return false;

	HCOUNTER hcCPU = NULL;

	PDH_COUNTER_PATH_ELEMENTS elements;
	TCHAR szBuf[1024] = _T("");
	DWORD dwBufSize = 0;
	//TCHAR szInstanceName[256] = _T("360se");
	TCHAR szInstanceName[256];
	memset(szInstanceName, 0x00, sizeof(szInstanceName));
	_tcscpy_s(szInstanceName, _T("CommunityServer"));

	elements.szMachineName = NULL;
	elements.szObjectName = const_cast< TCHAR* >( _T( "Process" ) );
	elements.szInstanceName = szInstanceName;
	elements.szParentInstance = NULL;
	elements.dwInstanceIndex = -1;

	elements.szCounterName = const_cast<TCHAR *>(_T("% Processor Time"));
	dwBufSize = sizeof(szBuf);
	PdhMakeCounterPath(&elements, szBuf, &dwBufSize, 0);
	lStatus = PdhAddCounter(hQuery, szBuf, NULL, &hcCPU);
	if(lStatus != ERROR_SUCCESS)
		return false;

	Sleep(100);

	lStatus = PdhCollectQueryData(hQuery);
	if(lStatus != ERROR_SUCCESS)
		return false;

	PDH_FMT_COUNTERVALUE cv;
	lStatus = PdhGetFormattedCounterValue(hcCPU, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, NULL, &cv);
	if(lStatus == ERROR_SUCCESS)
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		CPUUsage = cv.doubleValue / (double)si.dwNumberOfProcessors;
	}

	PdhRemoveCounter(hcCPU);
	PdhCloseQuery(hQuery);

	return true;
}

void cResourceInfo::TestPDH(TCHAR* processName)
{
	//CString strInfo = _T("系统性能：\n");
	CString strInfo = _T("System Mornitoring：\n");
	CString strTemp = _T("");

	HQUERY  hQuery;
	HCOUNTER hcCommitTotal, hcCommitLimit;
	HCOUNTER hcKernelPaged, hcKernelNonpaged;
	HCOUNTER hcSysHandleCount, hcProcesses, hcThreads;

	PDH_STATUS lStatus = PdhOpenQuery(NULL, NULL, &hQuery);
	if(lStatus != ERROR_SUCCESS)
		return;

	PdhAddCounter(hQuery, _T("\\Memory\\Committed Bytes"), NULL, &hcCommitTotal);
	PdhAddCounter(hQuery, _T("\\Memory\\Commit Limit"), NULL, &hcCommitLimit);
	PdhAddCounter(hQuery, _T("\\Memory\\Pool Paged Bytes"), NULL, &hcKernelPaged);
	PdhAddCounter(hQuery, _T("\\Memory\\Pool Nonpaged Bytes"), NULL, &hcKernelNonpaged);
	PdhAddCounter(hQuery, _T("\\Process(_Total)\\Handle Count"), NULL, &hcSysHandleCount);
	PdhAddCounter(hQuery, _T("\\System\\Processes"), NULL, &hcProcesses);
	PdhAddCounter(hQuery, _T("\\System\\Threads"), NULL, &hcThreads);

	HCOUNTER hcCPU = NULL;
	HCOUNTER hcThreadCount = NULL;
	HCOUNTER hcHandleCount = NULL;
	HCOUNTER hcWorkingSet = NULL;
	HCOUNTER hcWorkingSetPeak = NULL;
	HCOUNTER hcPageFileBytes = NULL;
	PDH_COUNTER_PATH_ELEMENTS elements;
	TCHAR szBuf[1024] = _T("");
	DWORD dwBufSize = 0;
	//TCHAR szInstanceName[256] = _T("360se");
	TCHAR szInstanceName[256];
	memset(szInstanceName, 0x00, sizeof(szInstanceName));
	_tcscpy_s(szInstanceName, processName);

	elements.szMachineName = NULL;
	elements.szObjectName = const_cast< TCHAR* >( _T( "Process" ) );
	elements.szInstanceName = szInstanceName;
	elements.szParentInstance = NULL;
	elements.dwInstanceIndex = -1;

	elements.szCounterName = const_cast<TCHAR *>(_T("% Processor Time"));
	dwBufSize = sizeof(szBuf);
	PdhMakeCounterPath(&elements, szBuf, &dwBufSize, 0);
	lStatus = PdhAddCounter(hQuery, szBuf, NULL, &hcCPU);
	if(lStatus != ERROR_SUCCESS)
		return;

	elements.szCounterName = const_cast<TCHAR *>(_T("Thread Count"));
	dwBufSize = sizeof(szBuf);
	PdhMakeCounterPath(&elements, szBuf, &dwBufSize, 0);
	lStatus = PdhAddCounter(hQuery, szBuf, NULL, &hcThreadCount);
	if(lStatus != ERROR_SUCCESS)
		return;

	elements.szCounterName = const_cast<TCHAR *>(_T("Handle Count"));
	dwBufSize = sizeof(szBuf);
	PdhMakeCounterPath(&elements, szBuf, &dwBufSize, 0);
	lStatus = PdhAddCounter(hQuery, szBuf, NULL, &hcHandleCount);
	if(lStatus != ERROR_SUCCESS)
		return;

	elements.szCounterName = const_cast<TCHAR *>(_T("Working set"));
	dwBufSize = sizeof(szBuf);
	PdhMakeCounterPath(&elements, szBuf, &dwBufSize, 0);
	lStatus = PdhAddCounter(hQuery, szBuf, NULL, &hcWorkingSet);
	if(lStatus != ERROR_SUCCESS)
		return;

	elements.szCounterName = const_cast<TCHAR *>(_T("Working set Peak"));
	dwBufSize = sizeof(szBuf);
	PdhMakeCounterPath(&elements, szBuf, &dwBufSize, 0);
	lStatus = PdhAddCounter(hQuery, szBuf, NULL, &hcWorkingSetPeak);
	if(lStatus != ERROR_SUCCESS)
		return;

	elements.szCounterName = const_cast<TCHAR *>(_T("Page File Bytes"));
	dwBufSize = sizeof(szBuf);
	PdhMakeCounterPath(&elements, szBuf, &dwBufSize, 0);
	lStatus = PdhAddCounter(hQuery, szBuf, NULL, &hcPageFileBytes);
	if(lStatus != ERROR_SUCCESS)
		return;

	PDH_FMT_COUNTERVALUE cv;

	/*lStatus = PdhCollectQueryData(hQuery);
	if(lStatus != ERROR_SUCCESS)
	return;*/

	// CPU时间，必须等待一下
	//Sleep(100);

	lStatus = PdhCollectQueryData(hQuery);
	if(lStatus != ERROR_SUCCESS)
		return;

	// 句柄总数
	lStatus = PdhGetFormattedCounterValue(hcSysHandleCount, PDH_FMT_LONG, NULL, &cv);
	if(lStatus == ERROR_SUCCESS)
	{
		//strTemp.Format(_T("句柄总数 : %u\n"), cv.longValue);
		strTemp.Format(_T("SysHandleCount : %u\n"), cv.longValue);
		strInfo += strTemp;
	}
	// 线程总数
	lStatus = PdhGetFormattedCounterValue(hcThreads, PDH_FMT_LONG, NULL, &cv);
	if(lStatus == ERROR_SUCCESS)
	{
		//strTemp.Format(_T("线程总数 : %u\n"), cv.longValue);
		strTemp.Format(_T("Threads : %u\n"), cv.longValue);
		strInfo += strTemp;
	}
	// 进程总数
	lStatus = PdhGetFormattedCounterValue(hcProcesses, PDH_FMT_LONG, NULL, &cv);
	if(lStatus == ERROR_SUCCESS)
	{
		//strTemp.Format(_T("进程总数 : %u\n"), cv.longValue);
		strTemp.Format(_T("Processes : %u\n"), cv.longValue);
		strInfo += strTemp;
	}
	// 核心内存：分页数
	lStatus = PdhGetFormattedCounterValue(hcKernelPaged, PDH_FMT_LARGE, NULL, &cv);
	if(lStatus == ERROR_SUCCESS)
	{
		//strTemp.Format(_T("核心内存：分页数 : %u\n"), cv.largeValue / 1024);
		strTemp.Format(_T("KernelPaged : %u\n"), cv.largeValue / 1024);
		strInfo += strTemp;
	}
	// 核心内存：未分页数
	lStatus = PdhGetFormattedCounterValue(hcKernelNonpaged, PDH_FMT_LARGE, NULL, &cv);
	if(lStatus == ERROR_SUCCESS)
	{
		//strTemp.Format(_T("核心内存：未分页数 : %u\n"), cv.largeValue / 1024);
		strTemp.Format(_T("KernelNonpaged : %u\n"), cv.largeValue / 1024);
		strInfo += strTemp;
	}
	// 认可用量：总数
	lStatus = PdhGetFormattedCounterValue(hcCommitTotal, PDH_FMT_LARGE, NULL, &cv);
	if(lStatus == ERROR_SUCCESS)
	{
		//strTemp.Format(_T("认可用量：总数 : %u\n"), cv.largeValue / 1024);
		strTemp.Format(_T("CommitTotal : %u\n"), cv.largeValue / 1024);
		strInfo += strTemp;
	}
	// 认可用量：限制
	lStatus = PdhGetFormattedCounterValue(hcCommitLimit, PDH_FMT_LARGE, NULL, &cv);
	if(lStatus == ERROR_SUCCESS)
	{
		//strTemp.Format(_T("认可用量：限制 : %u\n"), cv.largeValue / 1024);
		strTemp.Format(_T("CommitLimit : %u\n"), cv.largeValue / 1024);
		strInfo += strTemp;
	}

	// 以下360rp进程信息
	strInfo += _T("\n");
	strInfo += szInstanceName;
	//strInfo += _T("进程：\n");
	strInfo += _T(" Program：\n");
	// CPU时间，注意必须加上PDH_FMT_NOCAP100参数，否则多核CPU会有问题，详见MSDN
	lStatus = PdhGetFormattedCounterValue(hcCPU, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, NULL, &cv);
	if(lStatus == ERROR_SUCCESS)
	{
		strTemp.Format(_T("Process CPU : %f\n"), cv.doubleValue / 2);
		strInfo += strTemp;
	}
	// 线程数
	lStatus = PdhGetFormattedCounterValue(hcThreadCount, PDH_FMT_LONG, NULL, &cv);
	if(lStatus == ERROR_SUCCESS)
	{
		//strTemp.Format(_T("线程数 : %u\n"), cv.longValue);
		strTemp.Format(_T("Process ThreadCount : %u\n"), cv.longValue);
		strInfo += strTemp;
	}
	// 句柄数
	lStatus = PdhGetFormattedCounterValue(hcHandleCount, PDH_FMT_LONG, NULL, &cv);
	if(lStatus == ERROR_SUCCESS)
	{
		//strTemp.Format(_T("句柄数 : %u\n"), cv.longValue);
		strTemp.Format(_T("Process HandleCount : %u\n"), cv.longValue);
		strInfo += strTemp;
	}
	// 内存使用
	lStatus = PdhGetFormattedCounterValue(hcWorkingSet, PDH_FMT_LARGE, NULL, &cv);
	if(lStatus == ERROR_SUCCESS)
	{
		//strTemp.Format(_T("内存使用 : %u\n"), static_cast<LONG>(cv.largeValue / 1024));
		strTemp.Format(_T("Process WorkingSet : %u\n"), static_cast<LONG>(cv.largeValue / 1024));
		strInfo += strTemp;
	}
	// 高峰内存使用
	lStatus = PdhGetFormattedCounterValue(hcWorkingSetPeak, PDH_FMT_LARGE, NULL, &cv);
	if(lStatus == ERROR_SUCCESS)
	{
		//strTemp.Format(_T("高峰内存使用 : %u\n"), static_cast<LONG>(cv.largeValue / 1024));
		strTemp.Format(_T("Process WorkingSetPeak: %u\n"), static_cast<LONG>(cv.largeValue / 1024));
		strInfo += strTemp;
	}
	// 虚拟内存大小
	lStatus = PdhGetFormattedCounterValue(hcPageFileBytes, PDH_FMT_LARGE, NULL, &cv);
	if(lStatus == ERROR_SUCCESS)
	{
		//strTemp.Format(_T("虚拟内存大小 : %u\n"), static_cast<LONG>(cv.largeValue / 1024));
		strTemp.Format(_T("Process PageFileBytes : %u\n"), static_cast<LONG>(cv.largeValue / 1024));
		strInfo += strTemp;
	}

	_tprintf(strInfo);
	PdhRemoveCounter(hcCommitTotal);
	PdhRemoveCounter(hcCommitLimit);
	PdhRemoveCounter(hcKernelPaged);
	PdhRemoveCounter(hcKernelNonpaged);
	PdhRemoveCounter(hcSysHandleCount);
	PdhRemoveCounter(hcProcesses);
	PdhRemoveCounter(hcThreads);
	PdhRemoveCounter(hcCPU);
	PdhRemoveCounter(hcThreadCount);
	PdhRemoveCounter(hcHandleCount);
	PdhRemoveCounter(hcWorkingSet);
	PdhRemoveCounter(hcWorkingSetPeak);
	PdhRemoveCounter(hcPageFileBytes);
	PdhCloseQuery(hQuery);
}
