#pragma once

#ifndef NETLIB_EXPORTS
//#define NETDLL// __declspec(dllimport)
#define EXTERN_TEMPLATE
#else
//#define NETDLL// __declspec(dllexport)
#define EXTERN_TEMPLATE extern
#endif

#define MAKE_SAFE_STRING_BUFFER(buffer) buffer[_countof(buffer) - 1] = 0

#define ONCE_INIT(DeclaredType, DeclaredVariable, InitFunction) static struct DeclaredType\
																{\
																private:\
																	friend void InitFunction();\
																	bool ONCE_INIT;\
																public:\
																	DeclaredType()\
																	{\
																		ONCE_INIT = false;\
																		InitFunction();\
																	}\
																}DeclaredVariable;