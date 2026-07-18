/**
  @file pathutil.h
  
  @brief 路径工具函数

  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef pathutil_h__
#define pathutil_h__

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <limits.h>
#include <unistd.h>
#endif

inline void getMediaPath(std::string& origin)
{
#ifdef _WIN32
	char buffer[MAX_PATH];
	char* strLastSlash = NULL;
	GetModuleFileName( NULL, buffer, MAX_PATH );
	buffer[MAX_PATH - 1] = 0;

	strLastSlash = strrchr( buffer, '\\' );
	if (strLastSlash)
	{
		*(strLastSlash + 1) = '\0';
	}

	origin = buffer + origin;
#else
	std::string base;
	if (std::getenv("SR_RUNTIME_PATH"))
		base = std::getenv("SR_RUNTIME_PATH");
#if defined(__APPLE__)
	else
	{
		uint32_t size = 0;
		_NSGetExecutablePath(NULL, &size);
		std::vector<char> executable(size);
		_NSGetExecutablePath(executable.data(), &size);
		base = executable.data();
		const std::string::size_type slash = base.find_last_of('/');
		if (slash != std::string::npos) base.resize(slash);
	}
#elif defined(__linux__)
	else
	{
		char executable[PATH_MAX + 1];
		const ssize_t length = readlink("/proc/self/exe", executable, PATH_MAX);
		if (length > 0)
		{
			executable[length] = 0;
			base.assign(executable);
			const std::string::size_type slash = base.find_last_of('/');
			if (slash != std::string::npos) base.resize(slash);
		}
		else
			base = ".";
	}
#else
	else base = ".";
#endif
	for (std::string::iterator it = origin.begin(); it != origin.end(); ++it)
		if (*it == '\\') *it = '/';
	if (!origin.empty() && origin[0] == '/') origin.erase(0, 1);
	origin = base + "/" + origin;
#endif
}

#endif // pathutil_h__
