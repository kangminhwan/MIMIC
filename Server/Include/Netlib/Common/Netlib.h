#pragma once
// dll 로 뺄때 주석을 풀어주면 됩니다.
//#pragma warning(disable:4251)
#include "includeHeader.h"
#include "Macro.h"
#include "Flag.h"
#include "_Define.h"
#include "Structure.h"


#define BEGIN_NETLIB namespace NetLib{
#define END_NETLIB };

#include "cCriticalSection.h"
#include "cSRWLock_CriticalSection.h"
#include "cMemPooler.h"
#include "cPool.h"
#include "cSkipList.h"
#include "cVector.h"
#include "Utility.h"