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

typedef std::vector<IRenderer*> SrRendererList;

class SoftRenderApp : public IInputEventListener
{
public:
	SoftRenderApp(void);
	virtual ~SoftRenderApp(void);
	BOOL Init( HINSTANCE hInstance );
	void Destroy();
	void Run();

	bool Update();
	
	void RegisterTask(SrAppFramework* task);
	void UnRegisterTasks();

	bool InitRenderers();
	void ShutdownRenderers();

private:
	HINSTANCE m_hInst;								// current instance
	HWND m_hWnd;
	SrApps m_tasks;
	IRenderer* m_renderer;
	uint32 m_currRendererIndex;

	void LoadShaderList();

	virtual bool OnInputEvent( const SInputEvent &event );

	int m_curr_task;

};

#endif // SoftRenderApp_h__