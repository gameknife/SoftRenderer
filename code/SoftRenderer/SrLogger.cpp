#include "stdafx.h"
#include "SrLogger.h"
#include <chrono>
#include <ctime>

const int LOG_FILE_SZIE = 1024 * 50;

SrLogger::SrLogger(void)
{
	// 1mb
	m_data = new char[LOG_FILE_SZIE];
	m_size = 0;

}

SrLogger::~SrLogger(void)
{
	FlushToFile();

	delete[] m_data;

}

void SrLogger::Log( const char* line )
{
	uint32 length = strlen(line);
	
	// 缓存装不下了，写入文件
	if (m_size + length + 2 > LOG_FILE_SZIE)
	{
		FlushToFile();
	}
	
	// 装入缓存
	{
		const std::time_t now = std::time(NULL);
		std::tm time;
#ifdef _WIN32
		localtime_s(&time, &now);
#else
		localtime_r(&now, &time);
#endif

		char* buffer = new char[length + 100];

		std::snprintf(buffer, length + 100, "[%d/%d/%d %02d:%02d] %s\n",
			time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, line + 2);


		memcpy( m_data + m_size, buffer, strlen(buffer) );
		m_size += strlen(buffer);

		std::cout << buffer;

		delete[] buffer;
	}
	
}

void SrLogger::Log( const char* format, va_list args )
{
	char buffer[512];
	vsnprintf( buffer, sizeof(buffer), format, args );

	Log( buffer );
}

void SrLogger::FlushToFile()
{
	// write to file
	FILE* pFile = NULL;

	std::string path("log.txt");
	getMediaPath( path );

	fopen_s( &pFile, path.c_str(), "ab");

	if (pFile) { fwrite( m_data, 1, m_size, pFile ); fclose(pFile); }

	// remove size to 0
	m_size = 0;
}
