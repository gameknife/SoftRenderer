#ifndef SrPlatform_h__
#define SrPlatform_h__

#ifdef _WIN32
#include <windows.h>
#define SR_EXPORT __declspec(dllexport)

inline const char* SrGetLastDynamicLibraryError()
{
	static thread_local char message[512];
	const DWORD error = GetLastError();
	DWORD length = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, error, 0, message, static_cast<DWORD>(sizeof(message)), NULL);
	if (length == 0)
		return "Unknown Windows dynamic-library error";
	while (length > 0 && (message[length - 1] == '\r' || message[length - 1] == '\n'))
		message[--length] = 0;
	return message;
}
#else
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <mutex>
#include <strings.h>

typedef std::uint32_t DWORD;
typedef std::int32_t LONG;
typedef int BOOL;
typedef int HRESULT;
typedef void* HMODULE;
typedef void* HINSTANCE;
typedef void* HWND;
typedef void* HANDLE;
#define TRUE 1
#define FALSE 0
#define S_OK 0
#define S_FALSE 1
#define FAILED(value) ((value) < 0)
#define SUCCEEDED(value) ((value) >= 0)
#define APIENTRY
#define SR_EXPORT __attribute__((visibility("default")))
#define SRFASTCALL
#define MAX_PATH 4096

typedef std::mutex CRITICAL_SECTION;
typedef CRITICAL_SECTION* LPCRITICAL_SECTION;
inline void InitializeCriticalSection(CRITICAL_SECTION*) {}
inline void DeleteCriticalSection(CRITICAL_SECTION*) {}
inline void EnterCriticalSection(CRITICAL_SECTION* value) { value->lock(); }
inline void LeaveCriticalSection(CRITICAL_SECTION* value) { value->unlock(); }

inline HMODULE LoadLibraryA(const char* path) { return dlopen(path, RTLD_NOW | RTLD_LOCAL); }
inline void* GetProcAddress(HMODULE module, const char* name) { return dlsym(module, name); }
inline BOOL FreeLibrary(HMODULE module) { return dlclose(module) == 0; }
inline const char* SrGetLastDynamicLibraryError() { const char* error = dlerror(); return error ? error : "Unknown dynamic-library error"; }
inline int _stricmp(const char* a, const char* b) { return strcasecmp(a, b); }
inline int _putenv_s(const char* name, const char* value) { return value && value[0] ? setenv(name, value, 1) : unsetenv(name); }
inline int fopen_s(FILE** file, const char* path, const char* mode) { *file = std::fopen(path, mode); return *file ? 0 : -1; }

#define sprintf_s(buffer, ...) std::snprintf(buffer, sizeof(buffer), __VA_ARGS__)
#define strcpy_s(buffer, source) std::snprintf(buffer, sizeof(buffer), "%s", source)
#define GetRValue(color) ((std::uint8_t)((color) & 0xff))
#define GetGValue(color) ((std::uint8_t)(((color) >> 8) & 0xff))
#define GetBValue(color) ((std::uint8_t)(((color) >> 16) & 0xff))
#endif

#endif
