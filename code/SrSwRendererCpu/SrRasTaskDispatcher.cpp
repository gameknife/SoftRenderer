#include "StdAfx.h"
#include "SrRasTaskDispatcher.h"
#include "SrProfiler.h"

#include "mmgr/mmgr.h"

CRITICAL_SECTION g_taskCS;

SrTaskThread::SrTaskThread( int tileId, SrRasTaskDispatcher* creator ):m_creator(creator),
	m_threadId(tileId)
{
	m_runningFlag = new CManualResetEvent(false);
	m_waitFlag = new CManualResetEvent(false);
}

SrTaskThread::~SrTaskThread()
{
	delete m_runningFlag;
	delete m_waitFlag;
}

/**
 *@brief �̵߳����к���
 *@return int 
 *@remark û�б�SetReadyʱ��һֱwait��ֱ����SetReady��
 ��ʼ�������񣬴�����ɺ�runningFlag��λ��֪ͨ�ⲿ������ɣ�ͬʱ����wait״̬
 */
int SrTaskThread::Run()
{
	while(true)
	{
		m_waitFlag->Wait();
		m_waitFlag->Reset();
		if (m_creator->HasActiveBatch())
		{
			m_creator->ExecuteBatch(static_cast<uint32>(m_threadId));
			m_runningFlag->Set();
			continue;
		}

		while(true)
		{
			SrRasTask* task = m_creator->RequestTask();
			if (!task)
			{
				// û�������ˣ��˳��߳�
				break;
			}

			if (m_threadId < 16)
				gEnv->profiler->setIncrement( (EProfilerElement)(ePe_Thread0TaskNum + m_threadId) );
			// ִ������
			task->Execute();
		}
		m_runningFlag->Set();
	}
	return 0;	
}

SrRasTaskDispatcher::SrRasTaskDispatcher(void)
	: m_batchActive(false), m_lastBatchBusyRatio(0.f), m_lastBatchBalance(0.f)
{
	QueryPerformanceFrequency(&m_batchTimerFrequency);
}


SrRasTaskDispatcher::~SrRasTaskDispatcher(void)
{
}


/**
 *@brief ��ҵ�߳���ȡ����Ľӿڣ������ٽ�����֤������ȡ��ԭ���ԡ�
 *@return SrRasTask* 
 */
SrRasTask* SrRasTaskDispatcher::RequestTask()
{
	SrRasTask* ret =  NULL;

	EnterCriticalSection( &g_taskCS );
	if (!m_taskStack.empty())
	{
		ret = m_taskStack.top();
		m_taskStack.pop();
	}
	LeaveCriticalSection( &g_taskCS );

	return ret;
}

/**
 *@brief ��ʼ������ַ���
 *@return void 
 *@remark ����cpu������������cpu������-1����ҵ�߳�
 */
void SrRasTaskDispatcher::Init()
{
	int threadCount = g_context->processorNum - 1;

	for (int i=0; i < threadCount; ++i)
	{
		m_pool.push_back(new SrTaskThread(i, this));
	}
	m_batchWorkerStats.resize(static_cast<size_t>(threadCount + 1));
	
	SrTaskThreadPool::iterator it = m_pool.begin();
	for ( ; it != m_pool.end(); ++it)
	{
		(*it)->Start();
	}

	InitializeCriticalSection(&g_taskCS);
}

/**
 *@brief �ر�����ַ������ر���ҵ�߳�
 *@return void 
 */
void SrRasTaskDispatcher::Destroy()
{
	//Wait();

	DeleteCriticalSection(&g_taskCS);

	SrTaskThreadPool::iterator it = m_pool.begin();
	for ( ; it != m_pool.end(); ++it)
	{
		(*it)->Terminate();
		delete (*it);
	}
}

/**
 *@brief Э��flushģʽ�����̺߳������߳�һ����taskջ������
 *@return void 
 */
void SrRasTaskDispatcher::FlushCoop()
{
	if( g_context->IsFeatureEnable(eRFeature_MThreadRendering) )
	{
		SrTaskThreadPool::iterator it = m_pool.begin();
		for ( ; it != m_pool.end(); ++it)
		{
			(*it)->SetReady();
		}
	}
	
	// main thread also begin
	while(true)
	{
		SrRasTask* task = RequestTask();
		if (!task)
		{
			// û�������ˣ��˳��߳�
			break;
		}
		gEnv->profiler->setIncrement(ePe_MainThreadTaskNum);

		// ִ������
		task->Execute();
	}
}

/**
 *@brief ����flushģʽ��֪ͨ�߳̿�ʼ�����
 *@return void 
 */
void SrRasTaskDispatcher::Flush()
{
	if( g_context->IsFeatureEnable(eRFeature_MThreadRendering) )
	{
		SrTaskThreadPool::iterator it = m_pool.begin();
		for ( ; it != m_pool.end(); ++it)
		{
			(*it)->SetReady();
		}
	}
}

/**
 *@brief ���̵߳ȴ������߳���ɹ�����
 *@return void 
 */
void SrRasTaskDispatcher::Wait()
{
	if( g_context->IsFeatureEnable(eRFeature_MThreadRendering) )
	{
		SrTaskThreadPool::iterator it = m_pool.begin();
		for ( ; it != m_pool.end(); ++it)
			::WaitForSingleObject((*it)->getWaitingHandle(), INFINITE);
	}

	// destroy tasks
	SrTaskList::iterator itList = m_taskList.begin();
	for ( ; itList != m_taskList.end(); ++itList)
	{
		delete (*itList);
	}
	m_taskList.clear();

}

void SrRasTaskDispatcher::RunBatch(uint32 jobCount, void* context, BatchFunction function)
{
	if (jobCount == 0 || function == NULL)
		return;

	const uint32 participantCount = GetParticipantCount();
	const uint32 targetChunks = participantCount * 8;
	m_batchState.nextJob.store(0, std::memory_order_relaxed);
	m_batchState.jobCount = jobCount;
	m_batchState.grainSize = jobCount > targetChunks ? (std::max<uint32>)(1, jobCount / targetChunks) : 1;
	m_batchState.context = context;
	m_batchState.function = function;
	for (uint32 i = 0; i < participantCount; ++i)
	{
		m_batchWorkerStats[i].jobs = 0;
		m_batchWorkerStats[i].busyTicks = 0;
	}

	m_batchActive.store(true, std::memory_order_release);
	LARGE_INTEGER batchBegin, batchEnd;
	QueryPerformanceCounter(&batchBegin);
	if (g_context->IsFeatureEnable(eRFeature_MThreadRendering))
		for (SrTaskThreadPool::iterator it = m_pool.begin(); it != m_pool.end(); ++it)
			(*it)->SetReady();
	ExecuteBatch(participantCount - 1);
	Wait();
	QueryPerformanceCounter(&batchEnd);
	m_batchActive.store(false, std::memory_order_release);

	const uint32 mainIndex = participantCount - 1;
	uint64_t busyTicksSum = 0;
	uint64_t busyTicksMax = 0;
	for (uint32 i = 0; i < participantCount; ++i)
	{
		busyTicksSum += m_batchWorkerStats[i].busyTicks;
		busyTicksMax = (std::max)(busyTicksMax, m_batchWorkerStats[i].busyTicks);
		const float busyMs = static_cast<float>(static_cast<double>(m_batchWorkerStats[i].busyTicks) * 1000.0 /
			static_cast<double>(m_batchTimerFrequency.QuadPart));
		const uint32 profilerIndex = i == mainIndex ? 0 : i + 1;
		gEnv->profiler->addWorkerStats(profilerIndex, m_batchWorkerStats[i].jobs, busyMs);
		if (i == mainIndex)
			gEnv->profiler->setIncrement(ePe_MainThreadTaskNum, m_batchWorkerStats[i].jobs);
		else if (i < 16)
			gEnv->profiler->setIncrement(static_cast<EProfilerElement>(ePe_Thread0TaskNum + i), m_batchWorkerStats[i].jobs);
	}
	const uint64_t batchTicks = static_cast<uint64_t>(batchEnd.QuadPart - batchBegin.QuadPart);
	m_lastBatchBusyRatio = batchTicks ? static_cast<float>(static_cast<double>(busyTicksSum) /
		(static_cast<double>(batchTicks) * participantCount)) : 0.f;
	const double meanBusyTicks = participantCount ? static_cast<double>(busyTicksSum) / participantCount : 0.0;
	m_lastBatchBalance = meanBusyTicks > 0.0 ? static_cast<float>(busyTicksMax / meanBusyTicks) : 0.f;
	gEnv->profiler->setIncrement(ePe_BatchCountTile);
}

uint32 SrRasTaskDispatcher::GetParticipantCount() const
{
	return g_context->IsFeatureEnable(eRFeature_MThreadRendering)
		? static_cast<uint32>(m_pool.size() + 1) : 1;
}

void SrRasTaskDispatcher::ExecuteBatch(uint32 participantIndex)
{
	_mm_setcsr(0x1f80);
	LARGE_INTEGER begin, end;
	QueryPerformanceCounter(&begin);
	uint32 completed = 0;
	for (;;)
	{
		const uint32 first = m_batchState.nextJob.fetch_add(m_batchState.grainSize, std::memory_order_relaxed);
		if (first >= m_batchState.jobCount)
			break;
		const uint32 last = (std::min)(first + m_batchState.grainSize, m_batchState.jobCount);
		for (uint32 job = first; job < last; ++job)
			m_batchState.function(job, m_batchState.context);
		completed += last - first;
	}
	QueryPerformanceCounter(&end);
	m_batchWorkerStats[participantIndex].jobs = completed;
	m_batchWorkerStats[participantIndex].busyTicks = static_cast<uint64_t>(end.QuadPart - begin.QuadPart);
}

/**
 *@brief ���߳�������ַ�����ѹ������
 *@return void 
 *@param SrRasTask * task 
 */
void SrRasTaskDispatcher::PushTask( SrRasTask* task )
{
	m_taskStack.push(task);
	m_taskList.push_back(task);
}
