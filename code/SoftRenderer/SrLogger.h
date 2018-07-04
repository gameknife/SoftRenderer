/**
  @file GtLogger.h
  
  @author Kaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef SrLogger_h__
#define SrLogger_h__

struct ILogger
{
	virtual ~ILogger(void) {}

	virtual void Log(const char* line) =0;
	virtual void Log(const char* format, va_list args) =0;
};

class SrLogger : public ILogger
{
public:
	SrLogger(void);
	~SrLogger(void);

	void Log(const char* line);
	void Log(const char* format, va_list args);

private:
	void FlushToFile();
	char* m_data;
	uint32 m_size;
};

//	 Simple logs of data with low verbosity.
inline void GtLog(const char* format, ...)
{
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
}

inline void GtLogInfo(const char* format, ...)
{
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
}

inline void GtLogWarning(const char* format, ...)
{
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
}

inline void GtLogError(const char* format, ...)
{
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
}

#endif

