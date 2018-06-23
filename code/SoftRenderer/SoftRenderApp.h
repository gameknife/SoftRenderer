/**
  @file SoftRenderApp.h
  
  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef SoftRenderApp_h__
#define SoftRenderApp_h__

#include "SrAppFramework.h"

typedef std::vector<IRenderer*> SrRendererList;

class SoftRenderApp 
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
	SrApps m_tasks;
	IRenderer* m_renderer;
	uint32 m_currRendererIndex;

	void LoadShaderList();

	int m_curr_task;

};

#endif // SoftRenderApp_h__