#ifndef event_h__
#define event_h__

#include <chrono>
#include <condition_variable>
#include <mutex>

class IEvent
{
public:
	IEvent(bool manualReset, bool initialState) : m_manualReset(manualReset), m_signaled(initialState) {}
	virtual ~IEvent() {}
	HANDLE GetEvent() const { return reinterpret_cast<HANDLE>(const_cast<IEvent*>(this)); }
	void Wait() const
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_condition.wait(lock, [this] { return m_signaled; });
		if (!m_manualReset) m_signaled = false;
	}
	bool Wait(DWORD timeoutMillis) const
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		const bool ready = m_condition.wait_for(lock, std::chrono::milliseconds(timeoutMillis), [this] { return m_signaled; });
		if (ready && !m_manualReset) m_signaled = false;
		return ready;
	}
	void Reset() { std::lock_guard<std::mutex> lock(m_mutex); m_signaled = false; }
	void Set()
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_signaled = true;
		}
		if (m_manualReset) m_condition.notify_all(); else m_condition.notify_one();
	}
	void Pulse() { Set(); }

private:
	bool m_manualReset;
	mutable bool m_signaled;
	mutable std::mutex m_mutex;
	mutable std::condition_variable m_condition;
	IEvent(const IEvent&);
	IEvent& operator=(const IEvent&);
};

class CManualResetEvent : public IEvent
{
public:
	explicit CManualResetEvent(bool initialState = false) : IEvent(true, initialState) {}
};

#endif
