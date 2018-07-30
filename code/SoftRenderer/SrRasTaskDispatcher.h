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



private:
	CManualResetEvent* m_runningFlag;		/// ������ɱ�־
	CManualResetEvent* m_waitFlag;			/// �ȴ��´����б�־
	SrRasTaskDispatcher* m_creator;			/// ����ַ���
	int m_threadId;
};

typedef std::vector<SrTaskThread*> SrTaskThreadPool;
typedef std::stack<SrRasTask*> SrTaskStack;
typedef std::vector<SrRasTask*> SrTaskList;

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
	void FlushCoop();
	void Flush();
	void Wait();

	SrRasTask* RequestTask();

	gkScopedLock<gkMutexLock> *m_resLock;
	SrTaskThreadPool m_pool;			///< ����������̳߳أ������̸߳���ΪCPU������-1
	SrTaskStack m_taskStack;			///< ���ڴ�������ջ����ȡ����ֱ�Ӵ�ջ��ȡ����ֱ��ȡ�ꡣ
	SrTaskList  m_taskList;				///< ���ڴ���������ݵ����飬��������������ڡ�
};

#endif


