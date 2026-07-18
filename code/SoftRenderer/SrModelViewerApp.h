/**
  @file SrSponzaApp.h
  
  @author Kaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef SrModelViewerApp_h__
#define SrModelViewerApp_h__

#include "prerequisite.h"
#include "SrAppFramework.h"
#include "InputManager.h"

class SrModelViewerApp :
	public SrAppFramework, public IInputEventListener
{
public:
	void* operator new(size_t size) {return _mm_malloc_custom(size, 16);}
	void operator delete(void* memoryToBeDeallocated) {_mm_free_custom(memoryToBeDeallocated);}

	SrModelViewerApp(void);
	~SrModelViewerApp(void);

	virtual void OnInit();

	virtual void OnUpdate();

	virtual void OnDestroy();

	virtual bool OnInputEvent( const SInputEvent &event );

	void updateCam();

	void selectEnt(int index);

private:
	void SwitchSSAO();

	void UpdateShader();

	bool m_ssao;
	float m_camdist;
	int m_curr_ent;
	int m_shade_mode;

	struct SrCamera* m_camera;
	class SrScene* m_scene;
	class SrEntity* m_ent;

	class std::vector<SrEntity*> m_ents;
	
};


#endif


