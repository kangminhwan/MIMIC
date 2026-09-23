#pragma once
#pragma warning(disable:4251)

#include "TableServerHeader.h"

#include "tinyxml\tinyxml.h"
#include <iostream>

class cConfigReader
{
public:
	TServerConfiguration	m_tServerConfiguration;

	inline void readXml()
	{
		memset(&m_tServerConfiguration, 0x00, sizeof(TServerConfiguration));

        // E_SERVER_INFO_CEHCK_TIMER 타이머에서 ServerStatusReport 함수를 호출 여부 처리 플래그
        m_tServerConfiguration.bUseConsole = TRUE;

		TiXmlDocument doc("TableServer.xml");
		if (!doc.LoadFile())
		{
			std::cerr << "Failed to load XML file." << std::endl;
			return;
		}

		TiXmlElement* configurationElement = doc.FirstChildElement("Configuration");
		if (configurationElement)
		{
			TiXmlElement* serverElement = configurationElement->FirstChildElement("SERVER");
			if (serverElement) {
				const char* appName = serverElement->FirstChildElement("AppName")->GetText();
				int defaultServerPort = std::stoi(serverElement->FirstChildElement("Default_Server_Port")->GetText());
				int bufferServerPort = std::stoi(serverElement->FirstChildElement("Buffer_Server_Port")->GetText());
				int socketPoolSize = std::stoi(serverElement->FirstChildElement("SocketPoolSize")->GetText());
				int maxUser = std::stoi(serverElement->FirstChildElement("Max_User")->GetText());
				int backLog = std::stoi(serverElement->FirstChildElement("BackLog")->GetText());
                const char* publicIpAddres = serverElement->FirstChildElement( "publicIpAddres" )->GetText();

				std::cout << "AppName: " << appName << std::endl;
				std::cout << "Default Server Port: " << defaultServerPort << std::endl;
				std::cout << "Buffer Server Port: " << bufferServerPort << std::endl;
				std::cout << "Socket Pool Size: " << socketPoolSize << std::endl;
				std::cout << "Max User: " << maxUser << std::endl;
				std::cout << "BackLog: " << backLog << std::endl;

				_tcscpy_s(m_tServerConfiguration.szAppName, sizeof(m_tServerConfiguration.szAppName) / sizeof(TCHAR), _T("TableServer"));
				m_tServerConfiguration.wDefaultServerPort = defaultServerPort;
				m_tServerConfiguration.wBufferServerPort = bufferServerPort;
				m_tServerConfiguration.nSocketPoolSize = socketPoolSize;
				m_tServerConfiguration.wMaxUser = maxUser;
				m_tServerConfiguration.wBackLog = backLog;
                m_tServerConfiguration.PublicIpAddres = std::string(publicIpAddres);

                // 콘솔창 제목 변경
                {
                    std::string consoleTitle = std::format("[{}] : [{}]", appName , defaultServerPort );
                    HWND consoleWindow = GetConsoleWindow();
                    if ( consoleWindow != NULL ) {
                        SetConsoleTitleA( consoleTitle.c_str() );
                    }
                }
			}
			else
			{
				std::cerr << "Missing SERVER element in XML." << std::endl;
				return;
			}

			// <SERVER> 요소 외의 다른 데이터를 읽어들이는 부분
			int logThreadCount = std::stoi(configurationElement->FirstChildElement("LogThreadCnt")->GetText());
			int logQueueCount = std::stoi(configurationElement->FirstChildElement("LogQueueCnt")->GetText());
			int commandQueueCount = std::stoi(configurationElement->FirstChildElement("CommandQueueCnt")->GetText());
			int webQueueCount = std::stoi(configurationElement->FirstChildElement("WebQueueCnt")->GetText());
			int clientOvPool = std::stoi(configurationElement->FirstChildElement("Client_OvPool")->GetText());
			int logReCreateMinute = std::stoi(configurationElement->FirstChildElement("LogReCreateMinute")->GetText());
			int logGrade = std::stoi(configurationElement->FirstChildElement("LogGrade")->GetText());

			std::cout << "LogThreadCnt: " << logThreadCount << std::endl;
			std::cout << "LogQueueCnt: " << logQueueCount << std::endl;
			std::cout << "CommandQueueCnt: " << commandQueueCount << std::endl;
			std::cout << "WebQueueCnt: " << webQueueCount << std::endl;
			std::cout << "Client_OvPool: " << clientOvPool << std::endl;
			std::cout << "LogReCreateMinute: " << logReCreateMinute << std::endl;
			std::cout << "LogGrade: " << logGrade << std::endl;

			m_tServerConfiguration.nLogThreadCnt = logThreadCount;
			m_tServerConfiguration.nLogQueueCnt = logQueueCount;
			m_tServerConfiguration.nCommandQueueCnt = commandQueueCount;
			m_tServerConfiguration.nWebQueueCnt = webQueueCount;
			m_tServerConfiguration.nPerSocket_OvPoolCnt = clientOvPool;
			m_tServerConfiguration.nReCreateLogMinute = logReCreateMinute;
			m_tServerConfiguration.LogGrade = static_cast<LOG_GRADE>(logGrade);

            // CONNECTOR
            TiXmlElement* connectorElement = configurationElement->FirstChildElement("CONNECTOR");
            if (connectorElement)
            {
                // LOBBY LIST
                for ( TiXmlElement* serverElement = connectorElement->FirstChildElement( "LOBBY" );
                     serverElement != nullptr;
                     serverElement = serverElement->NextSiblingElement( "LOBBY" ) ) {

                    TiXmlElement* ipElement = serverElement->FirstChildElement( "ServerIP" );
                    TiXmlElement* portElement = serverElement->FirstChildElement( "ServerPort" );

                    if ( ipElement != nullptr && portElement != nullptr ) {

                        ConnectorInfo connector;
                        connector.ipAddress = ipElement->GetText();
                        connector.nPort = std::stoi( portElement->GetText() );

                        m_tServerConfiguration.lobbies.push_back( connector );
                        //lobby_connectors.push_back( connector );
                    }
                    else {
                        std::cerr << "Failed to find IP or Port element in a Server node." << std::endl;
                    }
                }

                // SLOT LIST
                
            }
            else
            {
                std::cerr << "Missing CONNECTOR element in XML." << std::endl;
            }

            //CommandThread, WorkerThread
            SYSTEM_INFO sys;
            GetSystemInfo(&sys);

            // Worker Thread Cnt
            int workerThreadCount = sys.dwNumberOfProcessors * 2;
            m_tServerConfiguration.nWorkerThreadCnt = workerThreadCount;
            std::cerr << "WorkerThreadCnt: " << workerThreadCount << std::endl;

            // Command Thread Cnt
            //int commandThreadCount = sys.dwNumberOfProcessors * 2;
            int commandThreadCount = 1;
            m_tServerConfiguration.nCommandThreadCnt = commandThreadCount;
            std::cerr << "CommandThreadCnt: " << commandThreadCount << std::endl;

            // Web Thread Cnt
            int webThreadCount = sys.dwNumberOfProcessors << 2;
            m_tServerConfiguration.nWebThreadCnt = webThreadCount;
            std::cerr << "WebThreadCount: " << webThreadCount << std::endl;

            // SERVERINFO
            TiXmlElement* serverInfoElement = configurationElement->FirstChildElement("SERVERINFO");
            if (serverInfoElement) 
            {
                const char* serverType = serverInfoElement->FirstChildElement("SERVERTYPE")->GetText();
                int serverId = std::stoi( serverInfoElement->FirstChildElement( "SERVERTID" )->GetText() );
                int useConsole = std::stoi( serverInfoElement->FirstChildElement("USECONSOLE")->GetText() );
                int useslack = std::stoi( serverInfoElement->FirstChildElement( "USESLACK" )->GetText() );
                //const char* pinballVersion = serverInfoElement->FirstChildElement( "PINBALLVERSION" )->GetText();
                
                const char* serverStage = serverInfoElement->FirstChildElement("SERVERSTAGE")->GetText();
                
                E_SERVER_TYPE _server_type = E_SERVER_TYPE::BASE_SERVER;

                for ( int n = 0; n < E_SERVER_TYPE::SERVER_TYPE_MAX; ++n ) {

                    if ( ::strcmp( G_SERVERNAME[ n ] , serverType ) == 0 ) {
                        _server_type = ( E_SERVER_TYPE ) ( n );
                    }

                }

                E_SERVER_STAGE _server_stage = E_SERVER_STAGE::LOCAL;

                for ( int n = 0; n < E_SERVER_STAGE::E_SERVER_STAGE_MAX; ++n ) {

                    if ( ::strcmp( G_SERVERSTAGE[ n ] , serverStage ) == 0 ) {
                        _server_stage = ( E_SERVER_STAGE ) ( n );
                    }

                }

                std::cout << "ServerType: " << serverType << std::endl;
                std::cout << "UseConsole: " << useConsole << std::endl;
                std::cout << "SID_FOR_MANAGE: " << serverId << std::endl;

                m_tServerConfiguration.ServerType = _server_type;
                strcpy_s( m_tServerConfiguration.ServerTypeString, sizeof( m_tServerConfiguration.ServerTypeString ), serverType );
                m_tServerConfiguration.ServerStage = _server_stage;
                strcpy_s( m_tServerConfiguration.ServerStageString, sizeof( m_tServerConfiguration.ServerStageString ), serverStage  );
                m_tServerConfiguration.SID_FOR_MANAGE = serverId;
                if ( useslack == 1 )
                    m_tServerConfiguration.bSLackAlert = true;
                else
                    m_tServerConfiguration.bSLackAlert = false;


               // m_tServerConfiguration.pinballVersion = ( std::string)pinballVersion ;
            }
            else 
            {
                std::cerr << "Missing SERVERINFO element in XML." << std::endl;
            }

            // GAMEWEB
            TiXmlElement* gameWebElement = configurationElement->FirstChildElement("GAMEWEB");
            if (gameWebElement)
            {
                const char* webDNS = gameWebElement->FirstChildElement("WebDNS")->GetText();
                int webPort = std::stoi(gameWebElement->FirstChildElement("WebPort")->GetText());
                const char* controller = gameWebElement->FirstChildElement("Controller")->GetText();
                const char* action = gameWebElement->FirstChildElement("Action")->GetText();

                std::cout << "WebDNS: " << webDNS << std::endl;
                std::cout << "WebPort: " << webPort << std::endl;
                std::cout << "Controller: " << controller << std::endl;
                std::cout << "Action: " << action << std::endl;

                int dnsSize = MultiByteToWideChar(CP_ACP, 0, webDNS, -1, NULL, 0);
                MultiByteToWideChar(CP_ACP, 0, webDNS, -1, m_tServerConfiguration.webserver.szDNS, dnsSize);

                m_tServerConfiguration.webserver.nPort = webPort;

                int controllerSize = MultiByteToWideChar(CP_ACP, 0, controller, -1, NULL, 0);
                MultiByteToWideChar(CP_ACP, 0, controller, -1, m_tServerConfiguration.webserver.szController, controllerSize);

                int actionSize = MultiByteToWideChar(CP_ACP, 0, action, -1, NULL, 0);
                MultiByteToWideChar(CP_ACP, 0, action, -1, m_tServerConfiguration.webserver.szAction, actionSize);
                
            }
            else 
            {
                std::cerr << "Missing GAMEWEB element in XML." << std::endl;
            }

            // VERSION
            TiXmlElement* versionElement = configurationElement->FirstChildElement("VERSION");
            if (versionElement) 
            {
                const char* serverVersion = versionElement->FirstChildElement("SERVER_VERSION")->GetText();

                std::cout << "ServerVersion: " << serverVersion << std::endl;
            }
            else 
            {
                std::cerr << "Missing VERSION element in XML." << std::endl;
            }

            // REDIS
            TiXmlElement* redisElement = configurationElement->FirstChildElement("REDIS");
            if (redisElement) 
            {
                int redisPort = std::stoi(redisElement->FirstChildElement("RedisPort")->GetText());
                const char* redisDns = redisElement->FirstChildElement("RedisDns")->GetText();

                std::cout << "RedisPort: " << redisPort << std::endl;
                std::cout << "RedisDns: " << redisDns << std::endl;

                int redisDnsSize = MultiByteToWideChar( CP_ACP , 0 , redisDns , -1 , NULL , 0 );
                MultiByteToWideChar( CP_ACP , 0 , redisDns , -1 , m_tServerConfiguration.szRedisDns , redisDnsSize );

                m_tServerConfiguration.nRedisPort = redisPort;
            }
            else 
            {
                std::cerr << "Missing REDIS element in XML." << std::endl;
            }

            // GAME_DB
            TiXmlElement* mysqlElement = configurationElement->FirstChildElement("GAME_DB");
            if (mysqlElement)
            {
                const char* dbHost = mysqlElement->FirstChildElement("DB_HOST")->GetText();
                int dbPort = std::stoi(mysqlElement->FirstChildElement("DB_PORT")->GetText());
                const char* dbUser = mysqlElement->FirstChildElement("DB_USER")->GetText();
                const char* dbPass = mysqlElement->FirstChildElement("DB_PASS")->GetText();
                const char* dbName = mysqlElement->FirstChildElement("DB_NAME")->GetText();

                std::cout << "DBHost: " << dbHost << std::endl;
                std::cout << "DBPort: " << dbPort << std::endl;
                std::cout << "DBUser: " << dbUser << std::endl;
                std::cout << "DBPass: " << dbPass << std::endl;
                std::cout << "DBName: " << dbName << std::endl;

                int dbHostSize = MultiByteToWideChar(CP_ACP, 0, dbHost, -1, NULL, 0);
                MultiByteToWideChar(CP_ACP, 0, dbHost, -1, m_tServerConfiguration.gameDBs[ 0 ].szHost, dbHostSize);

                m_tServerConfiguration.gameDBs[ 0 ].nPORT = dbPort;

                int dbUserSize = MultiByteToWideChar(CP_ACP, 0, dbUser, -1, NULL, 0);
                MultiByteToWideChar(CP_ACP, 0, dbUser, -1, m_tServerConfiguration.gameDBs[ 0 ].szUser, dbHostSize);

                int dbPassSize = MultiByteToWideChar(CP_ACP, 0, dbPass, -1, NULL, 0);
                MultiByteToWideChar(CP_ACP, 0, dbPass, -1, m_tServerConfiguration.gameDBs[ 0 ].szPASS, dbHostSize);

                int dbNameSize = MultiByteToWideChar(CP_ACP, 0, dbName, -1, NULL, 0);
                MultiByteToWideChar(CP_ACP, 0, dbName, -1, m_tServerConfiguration.gameDBs[ 0 ].szNAME, dbHostSize);

                // Exception 발생 시킬것인지 결정한다.
                // DB 를 사용해야 하는 서버는 발생시키는 것이 맞다.
            }
            else
            {
                std::cerr << "Missing MYSQL element in XML." << std::endl;
            }

            // Log DB
            TiXmlElement* logdbElement = configurationElement->FirstChildElement( "LOGDB" );
            if ( logdbElement )
            {
                const char* dbHost = logdbElement->FirstChildElement( "DB_HOST" )->GetText();
                int dbPort = std::stoi( logdbElement->FirstChildElement( "DB_PORT" )->GetText() );
                const char* dbUser = logdbElement->FirstChildElement( "DB_USER" )->GetText();
                const char* dbPass = logdbElement->FirstChildElement( "DB_PASS" )->GetText();
                const char* dbName = logdbElement->FirstChildElement( "DB_NAME" )->GetText();

                std::cout << "LOGDB DBHost: " << dbHost << std::endl;
                std::cout << "LOGDB DBPort: " << dbPort << std::endl;
                std::cout << "LOGDB DBUser: " << dbUser << std::endl;
                std::cout << "LOGDB DBPass: " << dbPass << std::endl;
                std::cout << "LOGDB DBName: " << dbName << std::endl;

                int dbHostSize = MultiByteToWideChar( CP_ACP , 0 , dbHost , -1 , NULL , 0 );
                MultiByteToWideChar( CP_ACP , 0 , dbHost , -1 , m_tServerConfiguration.szLogHost , dbHostSize );

                m_tServerConfiguration.nLogPORT = dbPort;

                int dbUserSize = MultiByteToWideChar( CP_ACP , 0 , dbUser , -1 , NULL , 0 );
                MultiByteToWideChar( CP_ACP , 0 , dbUser , -1 , m_tServerConfiguration.szLogUser , dbHostSize );

                int dbPassSize = MultiByteToWideChar( CP_ACP , 0 , dbPass , -1 , NULL , 0 );
                MultiByteToWideChar( CP_ACP , 0 , dbPass , -1 , m_tServerConfiguration.szLogPASS , dbHostSize );

                int dbNameSize = MultiByteToWideChar( CP_ACP , 0 , dbName , -1 , NULL , 0 );
                MultiByteToWideChar( CP_ACP , 0 , dbName , -1 , m_tServerConfiguration.szLogNAME , dbHostSize );

                // Exception 발생 시킬것인지 결정한다.
                // DB 를 사용해야 하는 서버는 발생시키는 것이 맞다.
            }
            else
            {
                std::cerr << "Missing Log DB element in XML." << std::endl;
            }

            // Account DB
            TiXmlElement* accountdbElement = configurationElement->FirstChildElement( "ACCOUNT_DB" );
            if ( accountdbElement )
            {
                const char* dbHost = accountdbElement->FirstChildElement( "DB_HOST" )->GetText();
                int dbPort = std::stoi( accountdbElement->FirstChildElement( "DB_PORT" )->GetText() );
                const char* dbUser = accountdbElement->FirstChildElement( "DB_USER" )->GetText();
                const char* dbPass = accountdbElement->FirstChildElement( "DB_PASS" )->GetText();
                const char* dbName = accountdbElement->FirstChildElement( "DB_NAME" )->GetText();

                std::cout << "Account DBHost: " << dbHost << std::endl;
                std::cout << "Account DBPort: " << dbPort << std::endl;
                std::cout << "Account DBUser: " << dbUser << std::endl;
                std::cout << "Account DBPass: " << dbPass << std::endl;
                std::cout << "Account DBName: " << dbName << std::endl;

                int dbHostSize = MultiByteToWideChar( CP_ACP , 0 , dbHost , -1 , NULL , 0 );
                MultiByteToWideChar( CP_ACP , 0 , dbHost , -1 , m_tServerConfiguration.accountDB.szHost , dbHostSize );

                m_tServerConfiguration.accountDB.nPORT = dbPort;

                int dbUserSize = MultiByteToWideChar( CP_ACP , 0 , dbUser , -1 , NULL , 0 );
                MultiByteToWideChar( CP_ACP , 0 , dbUser , -1 , m_tServerConfiguration.accountDB.szUser , dbHostSize );

                int dbPassSize = MultiByteToWideChar( CP_ACP , 0 , dbPass , -1 , NULL , 0 );
                MultiByteToWideChar( CP_ACP , 0 , dbPass , -1 , m_tServerConfiguration.accountDB.szPASS , dbHostSize );

                int dbNameSize = MultiByteToWideChar( CP_ACP , 0 , dbName , -1 , NULL , 0 );
                MultiByteToWideChar( CP_ACP , 0 , dbName , -1 , m_tServerConfiguration.accountDB.szNAME , dbHostSize );

                // Exception 발생 시킬것인지 결정한다.
                // DB 를 사용해야 하는 서버는 발생시키는 것이 맞다.
            }
            else
            {
                std::cerr << "Missing Account DB element in XML." << std::endl;
            }

            // GameServer 환경 로딩
            TiXmlElement* gameserverElement = configurationElement->FirstChildElement("GAMESERVER");
            if (mysqlElement)
            {
                int numberOfRoomsPerThread = std::stoi(gameserverElement->FirstChildElement("NumberOfRoomsPerThread")->GetText());

                m_tServerConfiguration.nRoomCntPerThread = numberOfRoomsPerThread;

                std::cout << "GAMESERVER NumberOfRoomsPerThread: " << numberOfRoomsPerThread << std::endl;
            }
		}
		else
		{
			std::cerr << "Missing Configuration element in XML." << std::endl;
			return;
		}
	}

	inline TServerConfiguration* getConfig() { return &m_tServerConfiguration; }
};

