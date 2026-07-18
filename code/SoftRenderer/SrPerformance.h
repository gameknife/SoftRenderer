#ifndef SrPerformance_h__
#define SrPerformance_h__

#include "SrPlatform.h"
#if defined(__APPLE__) || defined(__linux__)
#include <pthread.h>
#endif

#if defined(SR_SUPERLUMINAL_ENABLED)
#include <Superluminal/PerformanceAPI.h>
#else
#define PERFORMANCEAPI_INSTRUMENT(InstrumentationID)
#define PERFORMANCEAPI_INSTRUMENT_DATA(InstrumentationID, InstrumentationData)
#endif

inline void SrSetCurrentThreadName(const char* name)
{
#if defined(SR_SUPERLUMINAL_ENABLED)
	PerformanceAPI::SetCurrentThreadName(name);
#endif

#ifdef _WIN32
	typedef HRESULT (WINAPI *SetThreadDescriptionFunction)(HANDLE, PCWSTR);
	HMODULE kernel = GetModuleHandleW(L"Kernel32.dll");
	SetThreadDescriptionFunction setThreadDescription = kernel
		? reinterpret_cast<SetThreadDescriptionFunction>(GetProcAddress(kernel, "SetThreadDescription")) : NULL;
	if (!setThreadDescription)
		return;
	wchar_t wideName[64];
	const int length = MultiByteToWideChar(CP_UTF8, 0, name, -1, wideName, static_cast<int>(sizeof(wideName) / sizeof(wideName[0])));
	if (length > 0)
		setThreadDescription(GetCurrentThread(), wideName);
#elif defined(__APPLE__)
	pthread_setname_np(name);
#elif defined(__linux__)
	char shortName[16];
	std::snprintf(shortName, sizeof(shortName), "%.15s", name);
	pthread_setname_np(pthread_self(), shortName);
#endif
}

#endif
