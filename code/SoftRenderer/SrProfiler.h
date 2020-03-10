/**
  @file SrProfiler.h
  
  @author Kaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef SrProfiler_h__
#define SrProfiler_h__

#define AVGTIME_COUNT 300

enum EProfilerElement
{
	ePe_FlushTime = 0,
	ePe_ClearTime,
	ePe_VertexShaderTime,
	ePe_VertexShaderFlushTime,
	ePe_RasterizeShaderTime,
	ePe_RasterizeShaderFlushTime,
	ePe_PixelShaderTime,
	ePe_PixelShaderFlushTime,
	ePe_FrameTime,
	ePe_FlushTextTime,
	ePe_PostProcessTime,
	ePe_PostClearTime,
	ePe_DispatcherWaitTime,

	ePe_DrawCallTime,
	ePe_DrawCallAllocTime,

	ePe_BatchCount,
	ePe_TriangleCount,
	ePe_RasterizeCount,
	ePe_PixelCount,
	ePe_VertexCount,
	ePe_ZError,

	ePe_SamplerTime,

	ePe_MainThreadTaskNum,
	ePe_Thread0TaskNum,
	ePe_Thread1TaskNum,
	ePe_Thread2TaskNum,
	ePe_Thread3TaskNum,
	ePe_Thread4TaskNum,
	ePe_Thread5TaskNum,
	ePe_Thread6TaskNum,
	ePe_Thread7TaskNum,
	ePe_Thread8TaskNum,
	ePe_Thread9TaskNum,
	ePe_Thread10TaskNum,
	ePe_Thread11TaskNum,
	ePe_Thread12TaskNum,
	ePe_Thread13TaskNum,
	ePe_Thread14TaskNum,
	ePe_Thread15TaskNum,
	ePe_Thread16TaskNum,
	ePe_Thread17TaskNum,
	ePe_Thread18TaskNum,
	ePe_Thread19TaskNum,
	ePe_Thread20TaskNum,
	ePe_Thread21TaskNum,
	ePe_Thread22TaskNum,
	ePe_Thread23TaskNum,

	ePe_Thread0IdleTime,
	ePe_Thread1IdleTime,
	ePe_Thread2IdleTime,
	ePe_Thread3IdleTime,
	ePe_Thread4IdleTime,
	ePe_Thread5IdleTime,
	ePe_Thread6IdleTime,
	ePe_Thread7IdleTime,
	ePe_Thread8IdleTime,
	ePe_Thread9IdleTime,
	ePe_Thread10IdleTime,
	ePe_Thread11IdleTime,
	ePe_Thread12IdleTime,
	ePe_Thread13IdleTime,
	ePe_Thread14IdleTime,
	ePe_Thread15IdleTime,
	ePe_Thread16IdleTime,
	ePe_Thread17IdleTime,
	ePe_Thread18IdleTime,
	ePe_Thread19IdleTime,
	ePe_Thread20IdleTime,
	ePe_Thread21IdleTime,
	ePe_Thread22IdleTime,
	ePe_Thread23IdleTime,

	// hair testing
	ePe_HairCount,
	ePe_HairColliderCount,
	ePe_HairSimulateTime,
	ePe_HairFlushTime,

	// gpu mark
	ePe_GpuFlushTime,
	ePe_GpuHairTime,
	ePe_GpuPostProce,

	ePe_Count,
};

struct IProfiler
{
	virtual ~IProfiler() {}

	virtual char* getProfileData() = 0;
	virtual char* getProfileDataConsole() = 0;
	virtual void Update() =0;

	virtual void setBegin( EProfilerElement element ) =0;
	virtual void setEnd( EProfilerElement element ) =0;
	virtual void IncreaseTime( EProfilerElement element, float time ) =0;
	virtual void setIncrement( EProfilerElement element, int count = 1 ) =0;
	virtual float getTime( EProfilerElement element ) =0;
	virtual float getAverageTime( EProfilerElement element ) =0;
	virtual int getCount( EProfilerElement element ) =0;

// 	virtual void setGPUBegin( EProfilerElement element ) =0;
// 	virtual void setGPUEnd( EProfilerElement element ) =0;
};


struct SrProfilerElement
{
	SrProfilerElement()
	{
		memset(this, 0, sizeof(*this));
		for (int i=0; i < AVGTIME_COUNT; ++i)
		{
			prevElapsedTime[i] = 0.03f;
		}
		dontClear = false;
	}

	// time element
	float prevTime;
	float elapsedTime;
	float avgElapsedTime;
	float avgTimer;

	float prevElapsedTime[AVGTIME_COUNT];
	int nextTimeIdx;
	// count element
	int	count;
	bool dontClear;
	bool gpuIssue;

	void Clear()
	{
		if(!dontClear)
		{
			count = 0;
		}		

		// TODO: average frametime set
		// 每0.25秒一次？
		
		prevElapsedTime[nextTimeIdx] = elapsedTime;
		if (++nextTimeIdx == AVGTIME_COUNT)
		{
			nextTimeIdx = 0;
		}

		// 计算平均时间
		avgElapsedTime = 0;
		for ( int i=0; i < AVGTIME_COUNT; ++i )
		{
			avgElapsedTime += prevElapsedTime[i];
		}
		avgElapsedTime /= ((float)AVGTIME_COUNT);

		if (avgElapsedTime < 0)
		{
			avgElapsedTime = 0;
		}

		elapsedTime = 0;
	}
};


class SrProfiler : public IProfiler
{
public:
	SrProfiler();
	~SrProfiler();

	void Update();
	char* getProfileData();
	char* getProfileDataConsole();

	void setBegin( EProfilerElement element );
	void setEnd( EProfilerElement element );
	void IncreaseTime( EProfilerElement element, float time );
	void setIncrement( EProfilerElement element, int count = 1 );
	float getTime( EProfilerElement element );
	float getAverageTime( EProfilerElement element );
	int getCount( EProfilerElement element );

	void setGPUBegin( EProfilerElement element );
	void setGPUEnd( EProfilerElement element );

private:
	char m_buffer[2048];
	char m_buffer_simple[2048];
	std::vector<SrProfilerElement> m_profileElements;
};

#endif