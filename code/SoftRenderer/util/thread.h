/**
  @file thread.h
  
  @brief 线程封装 线程工具

  @author Kaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef thread_h__
#define thread_h__

#include <process.h>
///////////////////////////////////////////////////////////////////////////////
// IThread
///////////////////////////////////////////////////////////////////////////////

class IThread 
{
public :

	IThread(): m_hThread(INVALID_HANDLE_VALUE)	{}

	virtual ~IThread() 
	{
		if (m_hThread != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(m_hThread);
		}
	}

	HANDLE GetHandle() const;

	void Wait() const;

	bool Wait(DWORD timeoutMillis) const;

	void Start();

	virtual int Run() = 0;

	void Terminate(
		DWORD exitCode = 0);

private :

	

	static unsigned int __stdcall ThreadFunction(void *pV);

	HANDLE m_hThread;

	// No copies do not implement
	IThread(const IThread &rhs);
	IThread &operator=(const IThread &rhs);
};

inline HANDLE IThread::GetHandle() const
{
	return m_hThread;
}

inline void IThread::Start()
{
	if (m_hThread == INVALID_HANDLE_VALUE)
	{
		unsigned int threadID = 0;

		m_hThread = (HANDLE)::_beginthreadex(0, 0, ThreadFunction, (void*)this, 0, &threadID);

		if (m_hThread == INVALID_HANDLE_VALUE)
		{
			
		}
	}
	else
	{
		
	}
}

inline void IThread::Wait() const
{
	if (!Wait(INFINITE))
	{
		
	}
}

inline bool IThread::Wait(DWORD timeoutMillis) const
{

	bool ok;

	DWORD result = ::WaitForSingleObject(m_hThread, timeoutMillis);
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

inline unsigned int __stdcall IThread::ThreadFunction(void *pV)
{
	int result = 0;

	IThread* pThis = (IThread*)pV;

	if (pThis)
	{
		try
		{
			result = pThis->Run();
		}
		catch(...)
		{
		}
	}

	return result;
}

inline void IThread::Terminate(
						DWORD exitCode /* = 0 */)
{
	if (!::TerminateThread(m_hThread, exitCode))
	{
		
	}
}

#endif