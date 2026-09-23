#include "../../Include/Netlib/Timer/cTimer.h"



NetLib::cTimer::cTimer()
{
	Init();
}


NetLib::cTimer::~cTimer()
{
}

BOOL NetLib::cTimer::Init()
{
	memset(&m_base_time, 0x00, sizeof(tm));

	m_i64Elapsetime = 0;

	return QueryPerformanceFrequency(&m_Frequency);
}

LONGLONG NetLib::cTimer::ProfileStart()
{
	LARGE_INTEGER start;
	QueryPerformanceCounter(&start);
	return start.QuadPart;
}

double NetLib::cTimer::EndProfile(LONGLONG start)
{
	LARGE_INTEGER end;
	QueryPerformanceCounter(&end);
	return (double)((double)(end.QuadPart - start) / (double)m_Frequency.QuadPart * 1000.0);
}

BOOL NetLib::cTimer::set_base_time_and_calculate_time(_In_ const unsigned int year, _In_ const unsigned int month, _In_ const unsigned int day, _In_ const unsigned int hour, _In_ const unsigned int minute, _In_ const unsigned int second)
{
	m_base_time.tm_year = year - 1900;
	m_base_time.tm_mon = month - 1;
	m_base_time.tm_mday = day;
	m_base_time.tm_hour = hour;
	m_base_time.tm_min = minute;
	m_base_time.tm_sec = second;
	
	__int64 cur_seconds = 0;
	_time64(&cur_seconds);

	__int64 base_time_seconds = _mktime64(&m_base_time);

	m_i64Elapsetime = cur_seconds - base_time_seconds;

	if (base_time_seconds == -1)
	{
		return FALSE;
	}

	return TRUE;
}

__int64 NetLib::cTimer::get_next_day_gmt_time_sec(_In_opt_ const int gmt_base)
{
	struct tm new_gmt_time;

	__int64 cur_seconds = 0;
	_time64(&cur_seconds);

	cur_seconds -= m_i64Elapsetime;

	cur_seconds += (gmt_base * 3600) + 86400;	//	하루의 시간을 더 해줍니다.

	_gmtime64_s(&new_gmt_time, &cur_seconds);

	new_gmt_time.tm_hour = new_gmt_time.tm_min = new_gmt_time.tm_sec = 0;	//	시 분 초 를 0으로 만듭니다.

	return _mktime64(&new_gmt_time);
}

/*
 * UTC 기준 몇주차 인지 구합니다.
 * 기준시를 처리하고 GMT 시각 기준으로 처리 합니다.
*/
std::string NetLib::cTimer::get_year_of_week_gmt_time_string(_In_opt_ const int gmt_base)
{
	struct tm new_gmt_time;
	char timebuf[26];

	__int64 cur_seconds = 0;
	_time64(&cur_seconds);	// 1970년 1월 1일 자정 이후 경과된 시간(초)을 반환하거나 오류가 발생한 경우 -1을 반환합니다.

	cur_seconds -= m_i64Elapsetime;	//	서버의 로컬 시간과 Global DB의 시간의 차이를 계산해줍니다.

	cur_seconds += gmt_base * 3600;	// UTC 0 시 + 운영 기준시간을 더해 줍니다.(실질 운영 UTC 시간)

	// Obtain coordinated universal time:
	errno_t err = _gmtime64_s(&new_gmt_time, &cur_seconds);
	if (err)
	{
		printf("Invalid Argument to _gmtime64_s\n");
		return std::string();
	}

	if (strftime(timebuf, sizeof timebuf, "%W", &new_gmt_time) == 0)
	{
		printf("Invalid Argument to strftime\n");
		return std::string();
	}

	char format_string[128];
	sprintf_s(format_string, "%d%s", 1900 + new_gmt_time.tm_year, timebuf);

	return format_string;
}

__int64 NetLib::cTimer::add_seconds_to_gmt_current_time_sec(_In_ const int add_seconds, _In_opt_ const int gmt_base)
{
	struct tm new_gmt_time;

	__int64 cur_seconds = 0;
	_time64(&cur_seconds);

	cur_seconds -= m_i64Elapsetime;

	cur_seconds += (gmt_base * 3600) + add_seconds;

	_gmtime64_s(&new_gmt_time, &cur_seconds);

	return _mktime64(&new_gmt_time);
}