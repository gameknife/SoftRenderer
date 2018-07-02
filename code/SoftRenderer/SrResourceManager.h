/**
  @file SrResourceManager.h
  
  @author yikaiming

  ������־ history
  ver:1.0
   
 */

#ifndef SrResourceManager_h__
#define SrResourceManager_h__

#include "prerequisite.h"

struct SrDefaultMediaPack;
typedef std::map<std::string, SrResource*> SrResourceLibrary;

class SrResourceManager : public IResourceManager
{
public:
	SrResourceManager(void);
	virtual ~SrResourceManager(void);

	virtual SrMesh*				LoadMesh(const char* filename);
	virtual const SrTexture*	LoadTexture(const char* filename, bool bump = false);
	virtual SrMaterial*			LoadMaterial(const char* filename);
	virtual SrMaterial*			CreateMaterial(const char* filename);
	virtual void				LoadMaterialLib(const char* filename);
	virtual SrShader*			GetShader(const char* name);
	virtual void				AddShader(SrShader* shader);

	virtual SrTexture*			CreateRenderTexture(const char* name, int width, int height, int bpp);
	virtual SrMaterial*			CreateManmualMaterial(const char* name);

	virtual void				InitDefaultMedia();
	virtual SrDefaultMediaPack*	getDefaultMediaPack() {return m_defaultMediaPack;}

	virtual void				LoadShaderList();

// render resource move here
	// Buffer����
	virtual SrVertexBuffer* AllocateVertexBuffer(uint32 elementSize, uint32 count, bool fastmode = false);
	virtual bool DeleteVertexBuffer(SrVertexBuffer* target);
	virtual SrIndexBuffer*	AllocateIndexBuffer(uint32 count);
	virtual bool DeleteIndexBuffer(SrIndexBuffer* target);

	virtual void CleanBufferBinding();

	SrResourceLibrary	m_shaderLibrary;
private:
	SrResourceLibrary	m_meshLibrary;
	SrResourceLibrary	m_textureLibrary;
	SrResourceLibrary	m_materialLibrary;

	SrDefaultMediaPack* m_defaultMediaPack;

	SrVertexBufferArray m_vertexBuffers;
	SrIndexBufferArray	m_indexBuffers;
};

#endif // SrResourceManager_h__



