#pragma once
#include "../Common/Netlib.h"

BEGIN_NETLIB

class cTimer
{
private:
	LARGE_INTEGER m_Frequency;

	struct tm m_base_time;
	__int64 m_i64Elapsetime;

private:
	BOOL Init();

public:
	LONGLONG ProfileStart();
	double EndProfile(_In_ LONGLONG start);

public:
	BOOL set_base_time_and_calculate_time(_In_ const unsigned int year, _In_ const unsigned int month, _In_ const unsigned int day, _In_ const unsigned int hour, _In_ const unsigned int minute, _In_ const unsigned int second);

public:
	__int64 get_next_day_gmt_time_sec(_In_opt_ const int gmt_base = 0);
	std::string get_year_of_week_gmt_time_string(_In_opt_ const int gmt_base = 0);

	__int64 add_seconds_to_gmt_current_time_sec(_In_ const int add_seconds, _In_opt_ const int gmt_base = 0);
public:
	cTimer();
	~cTimer();
};

END_NETLIB