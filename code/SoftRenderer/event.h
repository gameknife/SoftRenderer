/**
  @file event.h

  @brief 操作系统 event 封装，线程同步工具
  
  @author Kaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef event_h__
#define event_h__

///////////////////////////

static HANDLE Create(
					 LPSECURITY_ATTRIBUTES lpEventAttributes, 
					 bool bManualReset, 
					 bool bInitialState, 
					 LPCTSTR lpName)
{
	HANDLE hEvent = ::CreateEvent(lpEventAttributes, bManualReset, bInitialState, lpName);

	if (hEvent == NULL)
	{
		
	}

	return hEvent;
}

class IEvent 
{
public :
	IEvent(
		LPSECURITY_ATTRIBUTES lpEventAttributes,
		bool manualReset,
		bool initialState):m_hEvent(Create(lpEventAttributes, manualReset, initialState, 0)) {}

	IEvent(
		LPSECURITY_ATTRIBUTES lpEventAttributes,
		bool manualReset,
		bool initialState,
		const LPCTSTR &name):m_hEvent(Create(lpEventAttributes, manualReset, initialState, name)) {}

	virtual ~IEvent();

	HANDLE GetEvent() const;

	void Wait() const;

	bool Wait(
		DWORD timeoutMillis) const;

	void Reset();

	void Set();

	void Pulse();

private :

	HANDLE m_hEvent;

	// No copies do not implement
	IEvent(const IEvent &rhs);
	IEvent &operator=(const IEvent &rhs);
};


///////////////////////////////////////////////////////////////////////////////
// CEvent
///////////////////////////////////////////////////////////////////////////////

inline IEvent::~IEvent()
{
	::CloseHandle(m_hEvent);
}

inline HANDLE IEvent::GetEvent() const
{
	return m_hEvent;
}

inline void IEvent::Wait() const
{
	if (!Wait(INFINITE))
	{
		
	}
}

inline bool IEvent::Wait(DWORD timeoutMillis) const
{
	bool ok;

	DWORD result = ::WaitForSingleObject(m_hEvent, timeoutMillis);

	if (result == WAIT_TIMEOUT)
	{
		ok = false;
	}
	else if (result == WAIT_OBJECT_0)
	{
		ok = true;
	}
	else
	{
		
	}

	return ok;
}

inline void IEvent::Reset()
{
	if (!::ResetEvent(m_hEvent))
	{
		
	}
}

inline void IEvent::Set()
{
	if (!::SetEvent(m_hEvent))
	{
		
	}
}

inline void IEvent::Pulse()
{
	if (!::PulseEvent(m_hEvent))
	{
		
	}
}


class CManualResetEvent : public IEvent
{
public :

	explicit CManualResetEvent(
		bool initialState = false);

private :

	// No copies do not implement
	CManualResetEvent(const CManualResetEvent &rhs);
	CManualResetEvent &operator=(const CManualResetEvent &rhs);
};

inline CManualResetEvent::CManualResetEvent(
									 bool initialState /* = false */)
									 :  IEvent(0, true, initialState)
{

}

#endif