#pragma once
#include "../TableServerHeader.h"

#include <vector>
#include <mysql.h>

#include "DefProcedure.h"
#include "cMySQL.h"
#include <future>

using namespace std;

// 소멸자에서 커넥션을 자동반환하기 위해 만들었다.
class cConnInstance
{
private:
	int m_db_idx;
	MYSQL* m_MySQLConnection;
	E_DB_TYPE m_db_type;

private:
	cConnInstance() {}
	void Clear()
	{
		m_db_idx = 0;
		m_MySQLConnection = nullptr;
		m_db_type = E_DB_TYPE::E_DB_TYPE_NONE;
	}

public:
	cConnInstance(E_DB_TYPE db_type, const int db_idx = 0);// db_idx 가 0 일경우 글로벌로 풀러로 처리한다우.
	~cConnInstance();

	MYSQL* GetConnection() { return m_MySQLConnection; }
};

class cDBPooler
{
private:
	cDBPooler() {}
public:
	cDBPooler( const int initial_connection_count );

	~cDBPooler()
	{
		size_t stSize = m_pConnections.size();
		for (int n = 0; n < stSize; ++n)
		{
			MYSQL* pMySQL = m_pConnections.front();
			m_pConnections.pop_front();
			if (pMySQL != nullptr)
			{
				//	커넥션 종료 합니다.
				mysql_close(pMySQL);
			}
		}
	}

protected:
	stMysqlConnectionInfo		m_db_info;					// db 정보

private:
	int							m_cur_created_connection_cnt;	// 현재 생성된 커넥션 갯수
	int							m_max_connection_cnt;			// 최초 설정값의 두배로 설정한다.
	NetLib::cCriticalSection	m_lock;
	std::deque<MYSQL*>			m_pConnections;

public:
	void SetDB( stMysqlConnectionInfo& db_info );

	inline void SetInitialCount( const int init_connection_count ) {

		m_cur_created_connection_cnt = init_connection_count;
		m_max_connection_cnt = m_cur_created_connection_cnt * 2;
	}

	bool Push(MYSQL* MySQLConnection);
	MYSQL* Pop();
	static MYSQL* CreateConnection(stMysqlConnectionInfo& db_info);
	void StartKeepAlive();
	void StopKeepAlive();

private:
	void KeepAlive();

	std::atomic<bool> m_keep_alive_running;
	std::future<void> m_keep_alive_future;

};

class cMySQLConnectionPooler
{
public:
	cMySQLConnectionPooler();
	~cMySQLConnectionPooler();

	void Init();
	void Clear();

	void SetAccountDB(stMysqlConnectionInfo& db_info);
	void SetLogDB( stMysqlConnectionInfo& db_info );
	void InsertGameConnectionInfo(stMysqlConnectionInfo& db_info);

	bool PushAccount(MYSQL* MySQLConnection);
	MYSQL* PopAccount();

	bool PushLog( MYSQL* MySQLConnection );
	MYSQL* PopLog();

	bool PushShard(const int db_idx, MYSQL* MySQLConnection);
	MYSQL* PopShard(const int db_idx);

	void StartKeepAlive();
	void StopKeepAlive();

	static bool IsConnectionAlive( MYSQL* connection );
	static bool ReconnectIfNeeded( MYSQL* connection );

protected:
	int m_web_thread_cnt;
	ATL::CAtlMap<UINT, stMysqlConnectionInfo>	m_shard_db_infos;	// shard db 는 여러개로 구성
	ATL::CAtlMap<UINT, stMysqlConnectionInfo>	m_log_db_infos;		// log db 는 여러개로 구성

private:
	cDBPooler* m_pGlobalPooler;										// global connecton 은 따로 관리 합니다.
	cDBPooler* m_pLogPooler;										// Log connecton 은 따로 관리 합니다.

	// db_idx별로 cMemPooler를 구성한다. 
	// shard, log db만 구성한다.
	ATL::CAtlMap<UINT, cDBPooler*> m_db_pooler_list;
};