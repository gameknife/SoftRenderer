/**
  @file timer.h
  
  @brief 计时器类，sr使用的时间接口

  @author Kaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef timer_h__
#define timer_h__

#ifdef OS_WIN32
#include <MMSystem.h>
#else
#include <mach/mach_time.h>

inline unsigned long timeGetTime()
{
	uint64_t time = mach_absolute_time();

	mach_timebase_info_data_t m_sTimeBaseInfo;
	mach_timebase_info(&m_sTimeBaseInfo);
	uint64_t millis = (time * (m_sTimeBaseInfo.numer / m_sTimeBaseInfo.denom)) / 1000000.0;
	return static_cast<unsigned long>(millis);
	return 0;

}
#endif
class SrTimer
{
public:
	SrTimer(): m_time(0)
		, m_elapsedTime(0)
		, m_frameCount(0)
	{

	}
	~SrTimer()
	{

	}

	void Init()
	{
		m_time = timeGetTime() / 1000.f;
		m_elapsedTime = 0;
		//QueryPerformanceFrequency(&m_freq); // 获取cpu时钟周期   
	}

	void Update()
	{
		float newTime = timeGetTime() / 1000.f;
		m_elapsedTime = newTime - m_time;
		m_time = newTime;
		m_frameCount++;
	}

	void Stop()
	{
		m_elapsedTime = 0;
	}

	float getElapsedTime()
	{
		return m_elapsedTime;
	}

	float getTime()
	{
		float newTime = timeGetTime() / 1000.f;
		return newTime;
	}

	float getRealTime()
	{
		// LARGE_INTEGER now;
		// QueryPerformanceCounter(&now); // 获取cpu时钟计数
		// return (float)(now.QuadPart)/m_freq.QuadPart;

		return getTime();
	}

	int getFramecount()
	{
		return m_frameCount;
	}

private:
	float m_time;
	float m_elapsedTime;
	int m_frameCount;

	// high precision
	//LARGE_INTEGER m_freq;
};

#endif