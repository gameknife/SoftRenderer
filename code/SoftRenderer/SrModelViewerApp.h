/**
  @file SrSponzaApp.h
  
  @author Kaiming

  ������־ history
  ver:1.0
   
 */

#ifndef SrModelViewerApp_h__
#define SrModelViewerApp_h__

#include "prerequisite.h"
#include "SrAppFramework.h"

class SrModelViewerApp :
	public SrAppFramework
{
public:
	SrModelViewerApp(void);
	~SrModelViewerApp(void);

	virtual void OnInit();

	virtual void OnUpdate();

	virtual void OnDestroy();

	virtual void OnEvent(const char* event, const char* param);

	void updateCam();
	
private:
	void SwitchSSAO();

	void UpdateShader();

	bool m_ssao;
	float m_camdist;
	int m_shade_mode;

	struct SrCamera* m_camera;
	class SrScene* m_scene;
	class SrEntity* m_ent;	
};


#endif


