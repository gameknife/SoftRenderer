/**
  @file SoftRenderApp.h
  
  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef SoftRenderApp_h__
#define SoftRenderApp_h__

#include "SrAppFramework.h"
#include "InputManager.h"

class SoftRenderApp : public IInputEventListener
{
public:
	SoftRenderApp(void);
	virtual ~SoftRenderApp(void);
	BOOL Init();
	void Destroy();
	void Run();

	bool Update();
	
	void RegisterTask(SrAppFramework* task);
	void UnRegisterTasks();

	bool InitRenderers();
	void ShutdownRenderers();

private:
	HWND m_hWnd;
	SrApps m_tasks;

	void LoadShaderList();

	virtual bool OnInputEvent( const SInputEvent &event );

	int m_curr_task;
	float m_lastTitleUpdate;

};

#endif // SoftRenderApp_h__
