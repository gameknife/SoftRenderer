/**
  @file timer.h
  
  @brief 计时器类，sr使用的时间接口

  @author Kaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef timer_h__
#define timer_h__

#include <chrono>

class SrTimer
{
public:
	SrTimer():m_time(0)
		,m_elapsedTime(0)
	{

	}
	~SrTimer()
	{

	}

	void Init()
	{
		m_start = Clock::now();
		m_time = 0.f;
		m_elapsedTime = 0;
	}

	void Update()
	{
		float newTime = getRealTime();
		m_elapsedTime = newTime - m_time;
		m_time = newTime;
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
		return getRealTime();
	}

	float getRealTime()
	{
		return std::chrono::duration<float>(Clock::now() - m_start).count();
	}

private:
	float m_time;
	float m_elapsedTime;

	typedef std::chrono::steady_clock Clock;
	Clock::time_point m_start;
};

#endif
