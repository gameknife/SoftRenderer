/**
  @file SrScene.h
  
  @author yikaiming   
 */

#ifndef SrScene_h__
#define SrScene_h__

class SrScene
{
public:
	typedef std::map<std::string, SrEntity*> SrEntityLibrary;
	typedef std::map<std::string, SrCamera*> SrCameraLibrary;
	typedef std::stack<SrCamera*> SrCameraStack;

public:
	SrScene(void);
	~SrScene(void);

	// access
	SrEntity* CreateEntity(const char* name, const char* meshFilename, 
		const char* matFilename, 
		float3& pos, 
		Quat& rot,
		SrEntity* ent = NULL);

	SrEntity* GetEntity(const char* name);

	void RemoveEntity(SrEntity* target);
	void RemoveEntity(const char* name);

	// access
	SrCamera* CreateCamera(const char* name);
	SrCamera* GetCamera(const char* name);

	void PushCamera(SrCamera* cam);
	void PopCamera();

	void Update();
	void RenderVisbility(SrCamera* cam);


	// 
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


