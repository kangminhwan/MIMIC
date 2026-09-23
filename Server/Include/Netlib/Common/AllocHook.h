#pragma once

#include <crtdbg.h>

#define nNoMansLandSize 4

typedef struct _CrtMemBlockHeader
{
	struct _CrtMemBlockHeader * pBlockHeaderNext;
	struct _CrtMemBlockHeader * pBlockHeaderPrev;
	char *                      szFileName;
	int                         nLine;
#ifdef _WIN64
	/* These items are reversed on Win64 to eliminate gaps in the struct
	* and ensure that sizeof(struct)%16 == 0, so 16-byte alignment is
	* maintained in the debug heap.
	*/
	int                         nBlockUse;
	size_t                      nDataSize;
#else  /* _WIN64 */
	size_t                      nDataSize;
	int                         nBlockUse;
#endif  /* _WIN64 */
	long                        lRequest;
	unsigned char               gap[nNoMansLandSize];
	/* followed by:
	*  unsigned char           data[nDataSize];
	*  unsigned char           anotherGap[nNoMansLandSize];
	*/
} _CrtMemBlockHeader;

#define pbData(pblock) ((unsigned char *)((_CrtMemBlockHeader *)pblock + 1))
#define pHdr(pbData) (((_CrtMemBlockHeader *)pbData)-1)

//사용방법 프로그램 초반부에
//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//_CrtSetAllocHook(AllocHook);
//위 두줄 복사하시고 붙여넣기 하시면 됩니다.
inline int AllocHook(	int nAllocType, void *pvData, size_t nSize, int nBlockUse,
						long lRequest, const unsigned char * szFileName, int nLine)
{
	static size_t sizeAlloc = 0;

	if (nSize == 1024)
	{
		int i = 10;
	}

	_CrtMemBlockHeader* pHead;

	if (nBlockUse == 2)
	{
		//_CRT_BLOCK 2
		return true;
	}

	switch (nAllocType)
	{
	case 1: // Alloc
	{
		sizeAlloc += nSize;
	}
		break;
	case 2: // Realloc
	{

	}
		break;
	case 3: // Free
	{
		pHead = pHdr(pvData);
		sizeAlloc -= pHead->nDataSize;
	}
		break;
	default:
		break;
	}

	return 1;
}