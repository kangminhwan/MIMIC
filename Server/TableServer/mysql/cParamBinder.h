#pragma once

#include <mysql.h>

class cParamBinder
{
private:
	cParamBinder() {}
	MYSQL_BIND* m_pParams;
	int m_param_count;

public:
    cParamBinder(const int param_count)
        : m_pParams(nullptr), m_param_count(param_count)
    {
        m_pParams = new MYSQL_BIND[param_count];
        memset(m_pParams, 0, sizeof(MYSQL_BIND) * param_count);
    }

    ~cParamBinder()
    {
        if (m_pParams != nullptr)
            delete[] m_pParams;
    }
	
    // 다음과 같이 멤버 함수를 더 명확한 이름으로 변경
    template<typename ValueType>
    void BindParam(const int param_sequence, const enum_field_types field_type, const ValueType& param_value)
    {
        // 파라미터 순서 확인
        if (param_sequence > m_param_count || param_sequence < 1)
            throw "cParamBinder::BindParam Check param_sequence";

        // MYSQL_BIND 설정
        MYSQL_BIND& bind = m_pParams[param_sequence - 1];
        bind.buffer_type = field_type;
        bind.is_null = 0; // 0 means the value is not NULL

        // std::string 타입 처리
        if constexpr (std::is_same_v<ValueType, std::string>) 
        {
            const std::string& str = param_value;
            bind.buffer = (void*)str.c_str();
            bind.buffer_length = static_cast<unsigned long>(str.length());
            bind.length = &bind.buffer_length; // A pointer to the length of the data
        }
        else 
        {
            bind.buffer = (char*)&param_value;
            bind.length = nullptr;
        }
    }

	MYSQL_BIND* GetBinder()
	{
		return m_pParams;
	}
};