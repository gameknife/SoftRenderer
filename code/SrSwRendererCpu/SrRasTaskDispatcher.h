/**
  @file SrRasTaskDispatcher.h
  
  @author Kaiming

  ������־ history
  ver:1.0
   
 */

#ifndef SrRasTaskDispatcher_h__
#define SrRasTaskDispatcher_h__

#include "prerequisite.h"
#include <atomic>

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

	HANDLE getWaitingHandle()
	{
		return m_runningFlag->GetEvent();
	}

private:
	int m_threadId;

	CManualResetEvent* m_runningFlag;		/// ������ɱ�־
	CManualResetEvent* m_waitFlag;			/// �ȴ��´����б�־

	SrRasTaskDispatcher* m_creator;			/// ����ַ���
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
	typedef void (*BatchFunction)(uint32 jobIndex, void* context);
	typedef void (*MainFunction)(void* context);
	SrRasTaskDispatcher(void);
	~SrRasTaskDispatcher(void);

	void Init();
	void Destroy();

	void PushTask(SrRasTask* task);
	void FlushCoop();
	void Flush();
	void Wait();
	void RunBatch(uint32 jobCount, void* context, BatchFunction function);
	// Workers execute the batch while the caller performs mainFunction. The
	// caller never consumes batch jobs and only waits after mainFunction returns.
	void RunBatchWorkerOnly(uint32 jobCount, void* context, BatchFunction function,
		void* mainContext, MainFunction mainFunction);
	bool HasActiveBatch() const { return m_batchActive.load(std::memory_order_acquire); }
	void ExecuteBatch(uint32 participantIndex);
	uint32 GetParticipantCount() const;
	uint32 GetWorkerParticipantCount() const;
	float GetLastBatchBusyRatio() const { return m_lastBatchBusyRatio; }
	float GetLastBatchBalance() const { return m_lastBatchBalance; }

	SrRasTask* RequestTask();

	SrTaskThreadPool m_pool;			///< ����������̳߳أ������̸߳���ΪCPU������-1
	SrTaskStack m_taskStack;			///< ���ڴ�������ջ����ȡ����ֱ�Ӵ�ջ��ȡ����ֱ��ȡ�ꡣ
	SrTaskList  m_taskList;				///< ���ڴ���������ݵ����飬��������������ڡ�

private:
	struct SrBatchState
	{
		std::atomic<uint32> nextJob;
		uint32 jobCount;
		uint32 grainSize;
		void* context;
		BatchFunction function;
	};

	struct SrBatchWorkerStats
	{
		SrBatchWorkerStats() : jobs(0), busyTicks(0) {}
		uint32 jobs;
		uint64_t busyTicks;
		uint8 padding[48];
	};

	SrBatchState m_batchState;
	std::vector<SrBatchWorkerStats> m_batchWorkerStats;
	std::atomic<bool> m_batchActive;
	LARGE_INTEGER m_batchTimerFrequency;
	float m_lastBatchBusyRatio;
	float m_lastBatchBalance;
};

#endif


