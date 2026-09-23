#pragma once
#include "../Common/Netlib.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <locale> // 추가
#include <codecvt> // 추가

BEGIN_NETLIB

struct Record
{
	std::vector<std::wstring> m_vecField;
	Record();
	~Record();
};

class CRecordConverter
{
private:
	Record* m_pRecord;

public:
	BYTE GetByte(unsigned int uiField);
	float GetFloat(unsigned int uiField);
	std::wstring GetStringA(unsigned int uiField);
	int GetInt(unsigned int uiField);
	double GetDouble(unsigned int uiField);

public:
	CRecordConverter(Record* pRecord);
	~CRecordConverter();
};

class csvFileLoader
{
private:
	TCHAR* m_pCsvFilePath;
	std::vector<Record*> m_vecRecord;

public:
	bool Create(TCHAR* pFilePath);
	bool Open(bool bOpenTAB = false);
	bool OpenTAB();

	bool ParseLine(NetLib::Record* pRecord, wchar_t* pszLineData);
	bool ParseLineTAB(Record* pRecord, char* pszLineData);
	bool ParseLineTAB(Record* pRecord, wchar_t* pszLineData);
	bool TokenCheckParsing(char* pszLineData);
	void Clear();

	std::wstring& GetField(unsigned int uiRecord, unsigned int uiField);
	Record* GetRecord(unsigned int uiRecord);

	BYTE GetBYTE(unsigned int uiRecord, unsigned int uiField);
	float Getfloat(unsigned int uiRecord, unsigned int uiField);
	std::wstring GetStringA(unsigned int uiRecord, unsigned int uiField);
	int GetInt(unsigned int uiRecord, unsigned int uiField);
	double GetDouble(unsigned int uiRecord, unsigned int uiField);

	size_t GetRecordCount() { return m_vecRecord.size(); }

public:
	csvFileLoader(TCHAR* filePath);
	virtual ~csvFileLoader();
	csvFileLoader() { Clear(); };

public:
	static TCHAR* GetExecutableDirectory();
};

END_NETLIB