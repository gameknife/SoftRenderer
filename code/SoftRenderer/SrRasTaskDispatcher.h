/**
  @file SrRasTaskDispatcher.h
  
  @author Kaiming

  ������־ history
  ver:1.0
   
 */

#ifndef SrRasTaskDispatcher_h__
#define SrRasTaskDispatcher_h__

#include "prerequisite.h"

class SrRasTaskDispatcher;

/**
 @brief ��դ������ṹ
 @remark ��դ��������࣬�̳�֮�ɹ���������Ҫ�߳���ɵ�����
 */
struct SrRasTask
{
	virtual ~SrRasTask() {}
	virtual void Execute() =0;
};

class SrTaskThread;

typedef std::vector<SrTaskThread*> SrTaskThreadPool;
typedef std::stack<SrRasTask*> SrTaskStack;
typedef std::vector<SrRasTask*> SrTaskList;

/**
 @brief �����߳�
 @remark ��ȡ��ִ�й�դ������Ĵ����̡߳����̴߳�������״̬ʱ��
 �̻߳᲻��������ַ�����ȡ�������ִ�У�ֱ��û������ַ����߳�ִ����ϣ�
 ���Լ�״̬��Ϊ�ȴ���
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

	IEvent* getWaitingEvent()
	{
		return m_runningFlag;
	}

	void PrePushTask(SrRasTask* task)
	{
		m_internalTasks.push(task);
	}


private:
	CManualResetEvent* m_runningFlag;		/// ������ɱ�־
	CManualResetEvent* m_waitFlag;			/// �ȴ��´����б�־
	SrRasTaskDispatcher* m_creator;			/// ����ַ���
	SrTaskStack m_internalTasks;
	int m_threadId;
};



/**
 @brief ����ַ���
 @remark ���߳̿�������ַ����ύ��Ҫ���������Ȼ�����Flush��ʼִ�У�����Wait�ȴ������߳�ִ����ϡ�
 FlushCoop���ú����߳�Ҳ��Ϊ��ҵ�߳�֮һ���������̹߳�ͬ����Task��
 */
class SrRasTaskDispatcher
{
public:
	SrRasTaskDispatcher(void);
	~SrRasTaskDispatcher(void);

	void Init();
	void Destroy();

	void PushTask(SrRasTask* task);
	void PrePushTask(SrRasTask* task);
	void FlushCoop();
	void Flush();
	void Wait();

	SrRasTask* RequestTask();

	gkScopedLock<gkMutexLock> *m_resLock;
	SrTaskThreadPool m_pool;			///< 线程池
	SrTaskStack m_taskStack;			///< 任务堆栈
	SrTaskList  m_taskList;			///< 任务列表

	uint32 m_preTaskToken;			///< 任务分配令牌
};

#endif


