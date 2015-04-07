/**
  @file GtLogger.h
  
  @author Kaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef SrLogger_h__
#define SrLogger_h__

#include "prerequisite.h"

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

#endif

