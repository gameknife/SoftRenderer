#include "stdafx.h"
#include "SrLogger.h"

const int LOG_FILE_SZIE = 1024 * 50;

SrLogger::SrLogger(void)
{
	// 1mb
	m_data = new char[LOG_FILE_SZIE];
	m_size = 0;
	m_logCallback = NULL;
	
	//AllocConsole();
	#ifdef OS_WIN32
	freopen("CONOUT$","w+t",stdout);  
	freopen("CONIN$","r+t",stdin);
	#endif
}

SrLogger::~SrLogger(void)
{
	FlushToFile();

	delete[] m_data;

	#ifdef OS_WIN32
	fclose(stdout);
	fclose(stdin);
	#endif
	//FreeConsole();
}

void SrLogger::SetExternalLogCallback( LogCallback callback )
{
	m_logCallback = callback;
}

void SrLogger::Log( const char* line )
{
	if(m_logCallback != NULL)
	{
		m_logCallback(line);
	}

	uint32 length = (uint32)strlen(line);
	
	// ����װ�����ˣ�д���ļ�
	//if (m_size + length + 2 > LOG_FILE_SZIE)
	{
		FlushToFile();
	}
	
	#ifdef OS_WIN32
	// װ�뻺��
	{
		SYSTEMTIME time;
		GetSystemTime(&time);

		char* buffer = new char[length + 100];

		int infoType = 0;
		static HANDLE consolehwnd;
		consolehwnd = GetStdHandle(STD_OUTPUT_HANDLE);

		switch(line[1])
		{
		case '0':
			SetConsoleTextAttribute(consolehwnd, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE );
			break;
		case '1':
			SetConsoleTextAttribute(consolehwnd, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			break;
		case '2':
			SetConsoleTextAttribute(consolehwnd, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY );
			break;
		case '3':
			SetConsoleTextAttribute(consolehwnd, FOREGROUND_RED | FOREGROUND_INTENSITY );
			break;
		}

		sprintf(buffer, "[%d/%d/%d %d:%d(+8)] %s \r\n", time.wYear, time.wMonth, time.wDay, (time.wHour + 8) % 24, time.wMinute, line + 2);


		memcpy( m_data + m_size, buffer, strlen(buffer) );
		m_size += (uint32)strlen(buffer);

		std::cout << buffer;

		delete[] buffer;
	
		SetConsoleTextAttribute(consolehwnd, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE);
	}

	#endif
	
}

void SrLogger::Log( const char* format, va_list args )
{
	char buffer[512];
	vsprintf( buffer, format, args );

	Log( buffer );
}

void SrLogger::FlushToFile()
{
	// write to file
	FILE* pFile = NULL;

	std::string path("log.txt");
	getMediaPath( path );

	pFile = fopen( path.c_str(), "ab");

	fwrite( m_data, 1, m_size, pFile );

	fclose(pFile);

	// remove size to 0
	m_size = 0;
}
