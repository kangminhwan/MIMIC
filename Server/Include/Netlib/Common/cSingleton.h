// cSingleton.h: interface for the cSingleton class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_CSINGLETON_H_)
#define _CSINGLETON_H_

#pragma once
#include "Netlib.h"

BEGIN_NETLIB

template <class cObject>
class cSingleton
{
private:
	static cObject* m_pInstance;

private:
	cSingleton() {}
	~cSingleton() {}

public:
	static cObject* GetInstance()
	{
		if(m_pInstance == nullptr)
		{
			m_pInstance = new cObject;
		}
		return m_pInstance;
	}

	static cObject* ExistsInstance()
	{
		return m_pInstance;
	}

	static void DeleteInstance()
	{
		if(m_pInstance)
		{
			delete m_pInstance;
			m_pInstance = nullptr;
		}
	}

	static void SetInstance(cObject* pObject)
	{
		if(m_pInstance != nullptr)
		{
			m_pInstance = pObject;
		}
	}
};

template <class cObject>
cObject* cSingleton<cObject>::m_pInstance = nullptr;

END_NETLIB

#endif
