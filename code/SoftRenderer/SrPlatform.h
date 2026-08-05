#ifndef SrPlatform_h__
#define SrPlatform_h__

#ifdef _WIN32
#include <windows.h>
#else
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
#define SRFASTCALL
#define MAX_PATH 4096

typedef std::mutex CRITICAL_SECTION;
typedef CRITICAL_SECTION* LPCRITICAL_SECTION;
inline void InitializeCriticalSection(CRITICAL_SECTION*) {}
inline void DeleteCriticalSection(CRITICAL_SECTION*) {}
inline void EnterCriticalSection(CRITICAL_SECTION* value) { value->lock(); }
inline void LeaveCriticalSection(CRITICAL_SECTION* value) { value->unlock(); }

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
