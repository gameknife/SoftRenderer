#ifndef SrPerformance_h__
#define SrPerformance_h__

#include <windows.h>

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
}

#endif
