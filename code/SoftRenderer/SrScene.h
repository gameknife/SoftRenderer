/**
  @file SrScene.h
  
  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef SrScene_h__
#define SrScene_h__
#include "prerequisite.h"

class SrEntity;
struct SrCamera;




class SrScene
{
public:
	typedef std::map<std::string, SrEntity*> SrEntityLibrary;
	typedef std::map<std::string, SrCamera*> SrCameraLibrary;
	typedef std::stack<SrCamera*> SrCameraStack;

public:
	SrScene(void);
	~SrScene(void);

	// 实体access
	SrEntity* CreateEntity(const char* name, const char* meshFilename, 
		const char* matFilename, 
		float3& pos = float3(0,0,0), 
		Quat& rot = Quat::CreateIdentity(),
		SrEntity* ent = NULL);

	SrEntity* GetEntity(const char* name);

	void RemoveEntity(SrEntity* target);
	void RemoveEntity(const char* name);

	// 相机access
	SrCamera* CreateCamera(const char* name);
	SrCamera* GetCamera(const char* name);

	void PushCamera(SrCamera* cam);
	void PopCamera();

	void Update();
	void RenderVisbility(SrCamera* cam);


	// 灯光
	SrLight* AddLight();
	void RemoveLight(SrLight* tgt);
	void ClearLight();
	float4 GetSkyLightColor() {return m_skyLightColor;}

	SrLightList m_lightList;

private:
	SrEntityLibrary m_entityLib;
	SrCameraLibrary m_cameraLib;
	SrCameraStack	m_cameraStack;

	

	float4 m_skyLightColor;
};

#endif // SrScene_h__


