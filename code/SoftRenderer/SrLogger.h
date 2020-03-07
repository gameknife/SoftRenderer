/**
  @file GtLogger.h
  
  @author Kaiming

	 history
  ver:1.0
   
 */

#ifndef SrLogger_h__
#define SrLogger_h__

typedef void (*LogCallback)(const char*);

struct ILogger
{
	virtual ~ILogger(void) {}

	virtual void Log(const char* line) =0;
	virtual void Log(const char* format, va_list args) =0;

	virtual void SetExternalLogCallback( LogCallback callback ) = 0;
};

class SrLogger : public ILogger
{
public:
	SrLogger(void);
	~SrLogger(void);

	virtual void Log(const char* line);
	virtual void Log(const char* format, va_list args);

	virtual void SetExternalLogCallback( LogCallback callback );

private:
	void FlushToFile();
	char* m_data;
	uint32 m_size;
	LogCallback m_logCallback;
};

//	 Simple logs of data with low verbosity.
inline void GtLog(const char* format, ...)
{
	#ifndef OS_LINUX
	if (gEnv->logger)
	{
		va_list args;
		va_start(args, format);
		char buffer[1024];
		strcpy(buffer, "#0");
		strcat(buffer, format);
		gEnv->logger->Log(buffer, args);
		va_end(args);
	}
	#endif
}

inline void GtLogInfo(const char* format, ...)
{
	#ifndef OS_LINUX
	if (gEnv->logger)
	{
		va_list args;
		va_start(args, format);
		char buffer[1024];
		strcpy(buffer, "#1");
		strcat(buffer, format);
		gEnv->logger->Log(buffer, args);
		va_end(args);
	}
	#endif
}

inline void GtLogWarning(const char* format, ...)
{
	#ifndef OS_LINUX
	if (gEnv->logger)
	{
		va_list args;
		va_start(args, format);
		char buffer[1024];
		strcpy(buffer, "#2");
		strcat(buffer, format);
		gEnv->logger->Log(buffer, args);
		va_end(args);
	}
	#endif
}

inline void GtLogError(const char* format, ...)
{
	#ifndef OS_LINUX
	if (gEnv->logger)
	{
		va_list args;
		va_start(args, format);
		char buffer[1024];
		strcpy(buffer, "#3");
		strcat(buffer, format);
		gEnv->logger->Log(buffer, args);
		va_end(args);
	}
	#endif
}

#endif

