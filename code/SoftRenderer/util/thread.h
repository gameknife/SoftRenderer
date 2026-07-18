#ifndef thread_h__
#define thread_h__

#include <atomic>
#include <thread>

class IThread
{
public:
	IThread() : m_thread(NULL), m_stop(false) {}
	virtual ~IThread()
	{
		if (m_thread && m_thread->joinable()) m_thread->detach();
		delete m_thread;
	}

	HANDLE GetHandle() const { return reinterpret_cast<HANDLE>(m_thread); }
	void Wait() const { if (m_thread && m_thread->joinable()) m_thread->join(); }
	bool Wait(DWORD) const { Wait(); return true; }
	void Start() { if (!m_thread) m_thread = new std::thread(&IThread::ThreadFunction, this); }
	void Terminate(DWORD = 0) { m_stop.store(true); }
	bool StopRequested() const { return m_stop.load(); }
	virtual int Run() = 0;

private:
	static void ThreadFunction(IThread* thread)
	{
		try { thread->Run(); } catch (...) {}
	}
	mutable std::thread* m_thread;
	std::atomic<bool> m_stop;
	IThread(const IThread&);
	IThread& operator=(const IThread&);
};

#endif
