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

	SrMesh* LoadMesh(const char* filename) override;
	const SrTexture* LoadTexture(const char* filename, bool bump = false) override;
	SrMaterial* LoadMaterial(const char* filename) override;
	SrMaterial* CreateMaterial(const char* filename) override;
	void LoadMaterialLib(const char* filename) override;
	SrShader* GetShader(const char* name) override;
	void AddShader(SrShader* shader) override;

	SrTexture* CreateRenderTexture(const char* name, int width, int height, int bpp) override;
	SrMaterial* CreateManmualMaterial(const char* name) override;

	void InitDefaultMedia() override;
	SrDefaultMediaPack* getDefaultMediaPack() override { return m_defaultMediaPack; }

	void LoadShaderList() override;

	// render resource move here
	// Buffer access
	SrVertexBuffer* AllocateVertexBuffer(uint32 elementSize, uint32 count, bool fastMode = false) override;
	bool DeleteVertexBuffer(SrVertexBuffer* target) override;
	SrIndexBuffer* AllocateIndexBuffer(uint32 count) override;
	bool DeleteIndexBuffer(SrIndexBuffer* target) override;

	void CleanBufferBinding() override;

	SrResourceLibrary m_shaderLibrary;
private:
	SrResourceLibrary m_meshLibrary;
	SrResourceLibrary m_textureLibrary;
	SrResourceLibrary m_materialLibrary;

	SrDefaultMediaPack* m_defaultMediaPack;

	SrVertexBufferArray m_vertexBuffers;
	SrIndexBufferArray m_indexBuffers;
};

#endif // SrResourceManager_h__
