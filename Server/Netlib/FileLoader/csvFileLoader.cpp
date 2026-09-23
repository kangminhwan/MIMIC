#include "../../Include/Netlib/FileLoader/csvFileLoader.h"

NetLib::Record::Record()
{
}

NetLib::Record::~Record()
{
	m_vecField.clear();
}

////////////////////CRecordConverter//////////////////////////////

NetLib::CRecordConverter::CRecordConverter(NetLib::Record* pRecord) :
	m_pRecord(pRecord)
{
}

NetLib::CRecordConverter::~CRecordConverter()
{
}

BYTE NetLib::CRecordConverter::GetByte(unsigned int uiField)
{
	assert(uiField < m_pRecord->m_vecField.size());

	if(m_pRecord->m_vecField[uiField].length() == 0)
	{
		return 0;
	}

	return (BYTE)(*const_cast<wchar_t*>(m_pRecord->m_vecField[uiField].c_str()));
}

float NetLib::CRecordConverter::GetFloat(unsigned int uiField)
{
	assert(uiField < m_pRecord->m_vecField.size());

	if(m_pRecord->m_vecField[uiField].length() == 0)
	{
		return 0.f;
	}
	return static_cast<float>(_wtof(m_pRecord->m_vecField[uiField].c_str()));
}

std::wstring NetLib::CRecordConverter::GetStringA(unsigned int uiField)
{
	assert(uiField < m_pRecord->m_vecField.size());

	return m_pRecord->m_vecField[uiField];
}

int NetLib::CRecordConverter::GetInt(unsigned int uiField)
{
	assert(uiField < m_pRecord->m_vecField.size());

	if(m_pRecord->m_vecField[uiField].length() == 0)
		return 0;

	return _wtoi(m_pRecord->m_vecField[uiField].c_str());
}

double NetLib::CRecordConverter::GetDouble(unsigned int uiField)
{
	assert(uiField < m_pRecord->m_vecField.size());

	if(m_pRecord->m_vecField[uiField].length() == 0)
		return 0;

	return std::stod(m_pRecord->m_vecField[uiField].c_str());
}

////////////////////csvFileLoader//////////////////////////////

NetLib::csvFileLoader::csvFileLoader(TCHAR* filePath) :
	m_pCsvFilePath(nullptr)
{
	Clear();
	Create(filePath);
}

NetLib::csvFileLoader::~csvFileLoader()
{
	Clear();
}

bool NetLib::csvFileLoader::Create(TCHAR* filePath)
{
	// 콘솔의 코드 페이지를 유니코드(UTF-8)로 설정
	SetConsoleCP(CP_UTF8);

	size_t length = _tcslen(filePath) + 1; // Including null terminator
	m_pCsvFilePath = new TCHAR[length];
	_tcscpy_s(m_pCsvFilePath, length, filePath);

	// 유니코드 출력 보장
	_tprintf(_T("data fileloading %s\n"), filePath);

	return true;
}

void NetLib::csvFileLoader::Clear()
{
	if(m_pCsvFilePath != nullptr)
	{
		delete m_pCsvFilePath;
		m_pCsvFilePath = nullptr;
	}

	if(m_vecRecord.empty() == false)
	{
		std::vector<NetLib::Record*>::iterator iter = m_vecRecord.begin();

		for (; iter != m_vecRecord.end(); ++iter)
		{
			if((*iter) != nullptr)
			{
				delete (*iter);
				(*iter) = nullptr;
			}
		}

		m_vecRecord.clear();
	}
}

bool NetLib::csvFileLoader::Open(bool bOpenTAB)
{
	char szBuffer[1024];
	wchar_t szLineData[MAX_LINE_CHAR_COUNT] = { 0, };

	std::wifstream fLoadCsv;
	WideCharToMultiByte(CP_ACP, NULL, m_pCsvFilePath, -1, szBuffer, sizeof(szBuffer), NULL, NULL);

	fLoadCsv.open(szBuffer);
	fLoadCsv.imbue(std::locale(fLoadCsv.getloc(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::consume_header>));

	size_t stLineCnt = 0;

	if(!fLoadCsv.is_open())
		return false;

	while (!fLoadCsv.eof())
	{

		memset(szLineData, 0x00, sizeof(szLineData));
		fLoadCsv.getline(szLineData, MAX_LINE_CHAR_COUNT);

		if(wcslen(szLineData) == 0)
			continue;

		NetLib::Record * pRecord = new NetLib::Record;

		if(bOpenTAB == FALSE)
			ParseLine(pRecord, szLineData);
		else
			ParseLineTAB(pRecord, szLineData);

		m_vecRecord.push_back(pRecord);
		stLineCnt = m_vecRecord.size();

		
	}

	return true;
}

bool NetLib::csvFileLoader::OpenTAB()
{
	char szBuffer[1024] = { 0, };
	char szLineData[MAX_LINE_CHAR_COUNT] = { 0, };
	wchar_t wszLineData[MAX_LINE_CHAR_COUNT] = { 0, };

	std::ifstream fLoadCsv;
	WideCharToMultiByte(CP_ACP, NULL, m_pCsvFilePath, -1, szBuffer, sizeof(szBuffer), NULL, NULL);

	fLoadCsv.open(szBuffer);

	if(!fLoadCsv.is_open())
		return false;

	while (!fLoadCsv.eof())
	{
		NetLib::Record * pRecord = new NetLib::Record;

		fLoadCsv.getline(szLineData, MAX_LINE_CHAR_COUNT);

		INT nStrCount = (INT)strlen(szLineData);
		nStrCount = ::MultiByteToWideChar(CP_ACP, 0, szLineData, -1, wszLineData, nStrCount);

		if(fLoadCsv.eof())
			break;

		ParseLineTAB(pRecord, wszLineData);

		m_vecRecord.push_back(pRecord);
		memset(szLineData, 0, sizeof(szLineData));
	}
	fLoadCsv.close();
	return true;
}

bool NetLib::csvFileLoader::ParseLine(NetLib::Record* pRecord, wchar_t* pszLineData) // 변경
{
	std::wstring strLineData(pszLineData); // 변경
	std::wstring strBuf;

	size_t nCheckChange = -1;
	while (nCheckChange != 0)
	{
		nCheckChange = strLineData.find(L",,");
		if (nCheckChange != std::wstring::npos)
		{
			strLineData.replace(nCheckChange, 2, L", ,");
		}
		else
			break;

	}

	size_t nPos = 0; //커서의 위치를 담을 변수

	while (!strLineData.empty())
	{
		size_t nIndex = strLineData.find(L",");
		if (nIndex == std::wstring::npos)
		{
			strBuf = strLineData;
			strLineData.clear();
		}
		else
		{
			strBuf = strLineData.substr(0, nIndex);
			strLineData = strLineData.substr(nIndex + 1);
		}

		pRecord->m_vecField.push_back(strBuf);
	}

	return true;
}

bool NetLib::csvFileLoader::ParseLineTAB(NetLib::Record * pRecord, char * pszLineData)
{
	CString strLineData(pszLineData);
	CString strBuf;

	int nCheckChange = -1;
	while (nCheckChange != 0)
	{
		nCheckChange = strLineData.Replace(_T("\t\t"), _T("\t \t"));
	}

	int nPos = 0; //커서의 위치를 담을 변수

	while (strLineData.IsEmpty() == false)
	{
		strBuf = strLineData.Tokenize(_T("\t"), nPos);

		if(strBuf == _T(""))
		{
			pRecord->m_vecField.push_back(strBuf.GetString());
			return true;
		}

		pRecord->m_vecField.push_back(strBuf.GetString());
	}

	return true;
}

bool NetLib::csvFileLoader::ParseLineTAB(NetLib::Record * pRecord, wchar_t * pszLineData)
{
	CString strLineData(pszLineData);
	CString strBuf;

	int nCheckChange = -1;
	while (nCheckChange != 0)
	{
		nCheckChange = strLineData.Replace(_T("\t\t"), _T("\t \t"));
	}

	int nPos = 0; //커서의 위치를 담을 변수

	while (strLineData.IsEmpty() == false)
	{
		strBuf = strLineData.Tokenize(_T("\t"), nPos);

		if(strBuf == _T(""))
		{
			pRecord->m_vecField.push_back(strBuf.GetString());
			return true;
		}

		pRecord->m_vecField.push_back(strBuf.GetString());
	}

	return true;
}

bool NetLib::csvFileLoader::TokenCheckParsing(char * pszLineData)
{
	TCHAR tzLineDATA[4096] = { 0, };
	CString strLineData(pszLineData);
	CString strBuf;

	int nPos = 0;

	strBuf = strLineData.Tokenize(_T(","), nPos);
	while (strBuf != _T(""))
	{
		strBuf = strLineData.Tokenize(_T(","), nPos);

		m_vecRecord[nPos / 2]->m_vecField.push_back(strBuf.GetString());
	}

	return true;

}

std::wstring& NetLib::csvFileLoader::GetField(unsigned int uiRecord, unsigned int uiField)
{
	assert(uiRecord < m_vecRecord.size());
	assert(uiField < m_vecRecord[uiRecord]->m_vecField.size());

	return m_vecRecord[uiRecord]->m_vecField[uiField];
}

NetLib::Record* NetLib::csvFileLoader::GetRecord(unsigned int uiRecord)
{
	assert(uiRecord < m_vecRecord.size());

	return m_vecRecord[uiRecord];
}

BYTE NetLib::csvFileLoader::GetBYTE(unsigned int uiRecord, unsigned int uiField)
{
	assert(uiRecord < m_vecRecord.size());

	NetLib::CRecordConverter Converter(m_vecRecord[uiRecord]);

	return Converter.GetByte(uiField);
}

float NetLib::csvFileLoader::Getfloat(unsigned int uiRecord, unsigned int uiField)
{
	assert(uiRecord < m_vecRecord.size());

	NetLib::CRecordConverter Converter(m_vecRecord[uiRecord]);

	return Converter.GetFloat(uiField);
}

std::wstring NetLib::csvFileLoader::GetStringA(unsigned int uiRecord, unsigned int uiField)
{
	assert(uiRecord < m_vecRecord.size());

	NetLib::CRecordConverter Converter(m_vecRecord[uiRecord]);

	return Converter.GetStringA(uiField);
}

int NetLib::csvFileLoader::GetInt(unsigned int uiRecord, unsigned int uiField)
{
	assert(uiRecord < m_vecRecord.size());

	NetLib::CRecordConverter Converter(m_vecRecord[uiRecord]);

	return Converter.GetInt(uiField);
}

double NetLib::csvFileLoader::GetDouble(unsigned int uiRecord, unsigned int uiField)
{
	assert(uiRecord < m_vecRecord.size());

	NetLib::CRecordConverter Converter(m_vecRecord[uiRecord]);

	return Converter.GetDouble(uiField);
}

TCHAR* NetLib::csvFileLoader::GetExecutableDirectory()
{
	static TCHAR exeFileDirectory[ MAX_PATH ];
	memset( exeFileDirectory, 0x00, sizeof( exeFileDirectory ) );
	DWORD size = GetModuleFileName( nullptr , exeFileDirectory , MAX_PATH );

	if ( size == 0 ) {
		_tcscpy_s( exeFileDirectory , _T( "" ) );
	}
	else {
		// 경로에서 파일 이름을 제외한 디렉토리 경로만 유지
		PathRemoveFileSpec( exeFileDirectory );
	}

	return exeFileDirectory;
}