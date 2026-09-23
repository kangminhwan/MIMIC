#pragma once
#include "TableServerHeader.h"

// 마켓별로 버젼관리를 한다.
// 버젼 데이터가 없으면 버젼 체킹을 하지 않는 것으로 간주한다.
// 주기적인 체크가 필요해 보인다.
class cGameVersionChecker
{
public:
    enum eUpdateStatus
    {
        eUpdateStatus_None = 0 ,
        eUpdateStatus_Update_Force = 1 ,        // 강제 업데이트
        eUpdateStatus_Update_Recommend = 2 ,    // 권장 업데이트
        eUpdateStatus_Error = 10,               // 업데이트 체크 실패
    };

protected:
	std::map<General::StoreChannel , Server::Version> m_versions;

public:
	void PushVersion( const Server::Version& version ) {

		General::StoreChannel _market = static_cast< General::StoreChannel >( version.market() );

		auto iter = m_versions.find( _market );
		if ( iter == m_versions.end() ) {

			// 추가한다.
			m_versions.insert( std::make_pair( _market , version ) );
		}
		else {

			// 갱신한다.
			iter->second = version;
		}

	}

    Server::Version GetVersion( General::StoreChannel _market ) {

        auto iter = m_versions.find( _market );
        if ( iter != m_versions.end() )
            return iter->second;
        else
            return Server::Version::default_instance();
    }

    // 버젼에 . 이 있는지 확인한다.
    bool IsVersionStringValid( const std::string& version ) {
        if ( version.empty() ) return false;

        std::vector<int> parts = SplitVersionString( version );

        // 최소 메이저.마이너는 있어야 함 (패치는 선택적)
        if ( parts.size() < 2 || parts.size() > 3 ) return false;

        // 각 부분이 음수가 아닌 정수여야 함
        for ( int part : parts ) {
            if ( part < 0 ) return false;
        }

        return true;
    }

    // Helper function to split version string
    std::vector<int> SplitVersionString( const std::string& version ) {
        std::vector<int> result;
        std::stringstream ss( version );
        std::string item;

        while ( std::getline( ss , item , '.' ) ) {
            try {
                // 빈 문자열이나 공백만 있는 경우 체크
                if ( item.empty() || item.find_first_not_of( " \t" ) == std::string::npos ) {
                    result.push_back( 0 );
                }
                else {
                    result.push_back( std::stoi( item ) );
                }
            }
            catch ( const std::exception& ) {
                // 변환 실패 시 0으로 처리
                result.push_back( 0 );
            }
        }

        return result;
    }

    /*int VersionStringToInt( const std::string& version ) {

        std::vector<int> versionParts = SplitVersionString( version );
        int result = 0;
        int multiplier = 1;

        for ( int i = 0; i < versionParts.size(); ++i ) {
            result += versionParts[ versionParts.size() - 1 - i ] * multiplier;
            multiplier *= 10000;
        }

        return result;
    }*/

    int VersionStringToInt( const std::string& version ) {

        std::vector<int> versionParts = SplitVersionString( version );

        // 유의적 버전관리: 메이저.마이너.패치
        // 예: 1.2.3 -> 1002003 (메이저*1000000 + 마이너*1000 + 패치)
        int major = versionParts.size() > 0 ? versionParts[ 0 ] : 0;
        int minor = versionParts.size() > 1 ? versionParts[ 1 ] : 0;
        int patch = versionParts.size() > 2 ? versionParts[ 2 ] : 0;

        return major * 1000000 + minor * 1000 + patch;
    }

    Server::ClientUpdatePolicy CompareVersions( const General::StoreChannel& market , const std::string version  ) {

        if ( !IsVersionStringValid( version ) ) {
            return Server::ClientUpdatePolicy::ClientUpdate_CheckFailed;
        }

        // 마켓이 없으면 접속 처리
        auto iter = m_versions.find( market );
        if ( iter == m_versions.end() )
            return Server::ClientUpdatePolicy::ClientUpdate_None;

        const auto& server_version = iter->second;
        std::string version_min = server_version.version_min();
        std::string version_latest = server_version.version_latest();

        int v = VersionStringToInt( version );
        int v_min = VersionStringToInt( version_min );
        int v_latest = VersionStringToInt( version_latest );

        // 강제 업데이트
        if ( v < v_min )
            return Server::ClientUpdatePolicy::ClientUpdate_Required;

        // 권장 업데이트
        if ( v >= v_min && v < v_latest ) {
            return Server::ClientUpdatePolicy::ClientUpdate_Recommended;
        }

        // 버젼이 더 높아 해당 사항 없음
        if ( v >= v_latest ) {
            return Server::ClientUpdatePolicy::ClientUpdate_None;
        }

        // 에러로 판단
        return Server::ClientUpdatePolicy::ClientUpdate_CheckFailed;
    }

};
