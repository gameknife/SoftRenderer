/**
  @file SrSponzaApp.h
  
  @author Kaiming

  ������־ history
  ver:1.0
   
 */

#ifndef SrSponzaApp_h__
#define SrSponzaApp_h__

#include "prerequisite.h"
#include "SrAppFramework.h"

class SrSponzaApp :
	public SrAppFramework 
{
public:
	SrSponzaApp(void);
	~SrSponzaApp(void);

	virtual void OnInit();

	virtual void OnUpdate();

	virtual void OnDestroy();

private:
	void SwitchSSAO();

	bool m_ssao;

	struct SrCamera* m_camera;
	class SrScene* m_scene;
	class SrEntity* m_ent;
};


#endif


