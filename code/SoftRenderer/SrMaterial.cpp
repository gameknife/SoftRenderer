#include "stdafx.h"
#include "SrMaterial.h"

#include "SrObjLoader.h"

#include "SrResourceManager.h"
#include "SrDefaultMedia.h"

#include "mmgr.h"


SrMaterial::SrMaterial( const char* name ):SrResource(name,  eRT_Material)
{
	m_shader = NULL;
	m_textures.clear();
	m_alphaTest = 0;
	m_alphaBlend = false;
	//LoadFromFile();
	SetShader(gEnv->resourceMgr->GetShader("default"));

	GtLog("[ResourceManager] Material[%s] Created.", m_name.c_str());
}

void SrMaterial::ApplyTextures() const
{
	SrBitmapArray::const_iterator it = m_textures.begin();

	int index = 0;
	for (; it != m_textures.end(); ++it, ++index)
	{
		gEnv->renderer->SetTextureStage( *it , index );
	}
}

void SrMaterial::ApplyShaderConstants() const
{
	gEnv->renderer->SetShaderConstant(eSC_PS0 + 5, &(m_matDiffuse.x), 1);
	gEnv->renderer->SetShaderConstant(eSC_PS0 + 6, &(m_matSpecular.x), 1);
	gEnv->renderer->SetShaderConstant(eSC_PS0 + 7, &(m_glossness), 1);
}
