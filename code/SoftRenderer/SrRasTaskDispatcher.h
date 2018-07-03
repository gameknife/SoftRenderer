/**
  @file SrRasTaskDispatcher.h
  
  @author Kaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef SrRasTaskDispatcher_h__
#define SrRasTaskDispatcher_h__

#include "prerequisite.h"

class SrRasTaskDispatcher;

/**
 @brief 光栅化任务结构
 @remark 光栅化任务基类，继承之可构建各种需要线程完成的任务。
 */
struct SrRasTask
{
	virtual ~SrRasTask() {}
	virtual void Execute() =0;
};

/**
 @brief 任务线程
 @remark 索取，执行光栅化任务的处理线程。当线程处于运行状态时，
 线程会不断向任务分发器索取任务进行执行，直到没有任务分发。线程执行完毕，
 将自己状态置为等待。
 */
class SrTaskThread : public IThread
{
public:
	SrTaskThread( int threadId, SrRasTaskDispatcher* creator );
	virtual ~SrTaskThread();
	virtual int Run();

	void SetReady()
	{
		m_runningFlag->Reset();
		m_waitFlag->Set();
	}

	HANDLE getWaitingHandle()
	{
		return m_runningFlag->GetEvent();
	}

private:
	int m_threadId;

	CManualResetEvent* m_runningFlag;		/// 运行完成标志
	CManualResetEvent* m_waitFlag;			/// 等待下次运行标志

	SrRasTaskDispatcher* m_creator;			/// 任务分发器
};

typedef std::vector<SrTaskThread*> SrTaskThreadPool;
typedef std::stack<SrRasTask*> SrTaskStack;
typedef std::vector<SrRasTask*> SrTaskList;

/**
 @brief 任务分发器
 @remark 主线程可向任务分发器提交需要处理的任务。然后调用Flush开始执行，调用Wait等待其他线程执行完毕。
 FlushCoop调用后，主线程也作为作业线程之一，和其他线程共同处理Task。
 */
class SrRasTaskDispatcher
{
public:
	SrRasTaskDispatcher(void);
	~SrRasTaskDispatcher(void);

	void Init();
	void Destroy();

	void PushTask(SrRasTask* task);
	void FlushCoop();
	void Flush();
	void Wait();

	SrRasTask* RequestTask();

	gkScopedLock<gkMutexLock> *m_resLock;
	SrTaskThreadPool m_pool;			///< 处理任务的线程池，任务线程个数为CPU核心数-1
	SrTaskStack m_taskStack;			///< 用于存放任务的栈，索取任务直接从栈顶取出，直至取完。
	SrTaskList  m_taskList;				///< 用于存放任务数据的数组，负责任务的生存期。
};

#endif


