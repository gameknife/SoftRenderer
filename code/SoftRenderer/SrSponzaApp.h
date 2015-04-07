/**
  @file SrSponzaApp.h
  
  @author Kaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef SrSponzaApp_h__
#define SrSponzaApp_h__

#include "prerequisite.h"
#include "srappframework.h"
#include "InputManager.h"

class SrSponzaApp :
	public SrAppFramework, public IInputEventListener
{
public:
	SrSponzaApp(void);
	~SrSponzaApp(void);

	virtual void OnInit();

	virtual void OnUpdate();

	virtual void OnDestroy();

	virtual bool OnInputEvent( const SInputEvent &event );

private:
	void SwitchSSAO();

	bool m_ssao;

	class SrCamera* m_camera;
	class SrScene* m_scene;
	class SrEntity* m_ent;
};


#endif


