#include "stdafx.h"
#include "SrResourceManager.h"
#include "SrMesh.h"
#include "SrRenderTexture.h"
#include "SrBitmap.h"
#include "SrMaterial.h"
#include "SrDefaultMedia.h"
#include "SrObjLoader.h"
#include "SrShader.h"

SrResourceManager::SrResourceManager(void)
{
	m_textureLibrary.clear();
	m_materialLibrary.clear();
	m_meshLibrary.clear();
	m_shaderLibrary.clear();
}


SrResourceManager::~SrResourceManager(void)
{
	// destroy all loaded resources
	GtLogInfo("[ResourceManager] Shutting down.");
	SrResourceLibrary::iterator it = m_meshLibrary.begin();
	for (; it != m_meshLibrary.end(); ++it)
	{
		if (it->second)
		{
			delete it->second;
		}
	}

	it = m_textureLibrary.begin();
	for (; it != m_textureLibrary.end(); ++it)
	{
		if (it->second)
		{
			delete it->second;
		}
	}

	it = m_materialLibrary.begin();
	for (; it != m_materialLibrary.end(); ++it)
	{
		if (it->second)
		{
			delete it->second;
		}
	}

	it = m_shaderLibrary.begin();
	for (; it != m_shaderLibrary.end(); ++it)
	{
		if (it->second)
		{
			delete it->second;
		}
	}

	delete m_defaultMediaPack;

	GtLogInfo("[ResourceManager] Shutted down.");
}

SrMesh* SrResourceManager::LoadMesh( const char* filename )
{
	SrMesh* ret = NULL;
	SrResourceLibrary::iterator it = m_meshLibrary.find(filename);

	if (it != m_meshLibrary.end())
	{
		ret = static_cast<SrMesh*>(it->second);
	}
	else
	{
		ret = new SrMesh(filename);
		m_meshLibrary.insert(SrResourceLibrary::value_type( filename, ret ));
	}

	return ret;	
}

const SrTexture* SrResourceManager::LoadTexture( const char* filename, bool bump )
{
	SrTexture* ret = NULL;
	SrResourceLibrary::iterator it = m_textureLibrary.find(filename);

	if (it != m_textureLibrary.end())
	{
		// �����ǿ����ҵ����������
		ret = static_cast<SrTexture*>(it->second);
	}
	else
	{
		// ���û�д�����ȥ�����ⲿbitmap
		ret = new SrBitmap(filename);
		// �����п��ܴ������ɹ�
		if ( !ret || !(ret->getBuffer()) )
		{
			delete ret;
			if (bump)
			{
				ret = m_defaultMediaPack->getDefaultFlatTex();
			}
			else
			{
				ret = m_defaultMediaPack->getDefaultTex();
			}
			
		}
		else
		{
			m_textureLibrary.insert(SrResourceLibrary::value_type( filename, ret ));
		}		
	}

	return ret;	
}

SrMaterial* SrResourceManager::LoadMaterial( const char* filename )
{
	SrMaterial* ret = NULL;
	SrResourceLibrary::iterator it = m_materialLibrary.find(filename);

	if (it != m_materialLibrary.end())
	{
		ret = static_cast<SrMaterial*>(it->second);
	}
	else
	{
		// Ĭ�ϲ���
		ret = LoadMaterial( "$srdefualt" );
	}
	return ret;	
}


SrMaterial* SrResourceManager::CreateMaterial( const char* filename )
{
	SrMaterial* ret = NULL;
	SrResourceLibrary::iterator it = m_materialLibrary.find(filename);

	if (it != m_materialLibrary.end())
	{
		ret = static_cast<SrMaterial*>(it->second);
	}
	else
	{
		ret = new SrMaterial(filename);
		m_materialLibrary.insert(SrResourceLibrary::value_type( filename, ret ));
	}

	return ret;	
}


SrShader* SrResourceManager::GetShader( const char* name )
{
	SrShader* ret = NULL;

	SrResourceLibrary::iterator it = m_shaderLibrary.find(name);

	if (it != m_shaderLibrary.end())
	{
		ret = static_cast<SrShader*>(it->second);
	}

	return ret;
}

SrTexture* SrResourceManager::CreateRenderTexture( const char* name, int width, int height, int bpp )
{
	SrTexture* ret = NULL;
	SrResourceLibrary::iterator it = m_textureLibrary.find(name);

	if (it != m_textureLibrary.end())
	{
		ret = static_cast<SrTexture*>(it->second);
	}
	else
	{
		ret = new SrRenderTexture(name, width, height, bpp);
		m_textureLibrary.insert(SrResourceLibrary::value_type( name, ret ));
	}

	return ret;	
}

SrMaterial* SrResourceManager::CreateManmualMaterial( const char* name )
{
	SrMaterial* ret = NULL;
	return ret;
}

void SrResourceManager::InitDefaultMedia()
{
	m_defaultMediaPack = new SrDefaultMediaPack;
}

void SrResourceManager::LoadMaterialLib( const char* filename )
{
	SrObjLoader loader;

	// load file in memory
	SrMemFile matFile;
	std::string realname(filename);
	getMediaPath(realname);
	matFile.Open( realname.c_str() );
	if (matFile.IsOpen())
	{
		loader.LoadMaterialFromMTL( matFile.Data());
	}
	else
	{
	}
}


void SrResourceManager::LoadShaderList()
{

}


void SrResourceManager::AddShader( SrShader* shader )
{
	SrResourceLibrary::iterator it = m_shaderLibrary.find(shader->getName());

	if (it != m_shaderLibrary.end())
	{
	}
	else
	{
		m_shaderLibrary.insert(SrResourceLibrary::value_type( shader->getName(), shader ));
	}

}


SrVertexBuffer* SrResourceManager::AllocateVertexBuffer(uint32 elementSize, uint32 count, bool fastmode)
{
	SrVertexBuffer* vb = new SrVertexBuffer;
	vb->elementSize = elementSize;
	vb->elementCount = count;
	vb->data = (uint8*)(_mm_malloc_custom( elementSize * count, 16 ));

	bool hasEmpty = false;
	for (uint32 i=0; i < m_vertexBuffers.size(); ++i)
	{
		if( m_vertexBuffers[i] == NULL )
		{
			m_vertexBuffers[i] = vb;
			hasEmpty = true;
			break;
		}
	}

	if (!hasEmpty)
	{
		m_vertexBuffers.push_back( vb );
	}	

	return vb;
}

bool SrResourceManager::DeleteVertexBuffer( SrVertexBuffer* target )
{
	if (target)
	{
		for (uint32 i=0; i < m_vertexBuffers.size(); ++i)
		{
			if( m_vertexBuffers[i] == target )
			{
				_mm_free_custom( m_vertexBuffers[i]->data );
				delete (m_vertexBuffers[i]);
				m_vertexBuffers[i] = NULL;

				// ж���˾���������
				return true;
			}
		}
	}


	return false;
}

SrIndexBuffer* SrResourceManager::AllocateIndexBuffer( uint32 count )
{
	SrIndexBuffer* ib = new SrIndexBuffer;
	ib->data = new uint32[count];
	ib->count = count;

	bool hasEmpty = false;
	for (uint32 i=0; i < m_indexBuffers.size(); ++i)
	{
		if( m_indexBuffers[i] == NULL )
		{
			m_indexBuffers[i] = ib;
			hasEmpty = true;

			// װ���˾���������
			break;
		}
	}

	if (!hasEmpty)
	{
		m_indexBuffers.push_back( ib );
	}	

	return ib;


}

bool SrResourceManager::DeleteIndexBuffer( SrIndexBuffer* target )
{
	if (target)
	{
		for (uint32 i=0; i < m_indexBuffers.size(); ++i)
		{
			if( m_indexBuffers[i] == target )
			{
				if (m_indexBuffers[i]->data)
				{
					delete[] m_indexBuffers[i]->data;
					m_indexBuffers[i]->data = NULL;
				}			
				delete m_indexBuffers[i];
				m_indexBuffers[i] = NULL;

				// ж���˾���������
				return true;
			}
		}
	}
	return false;
}

void SrResourceManager::CleanBufferBinding()
{
	// VB binding
	for (uint32 i=0; i < m_vertexBuffers.size(); ++i)
	{
		if (m_vertexBuffers[i])
		{
			m_vertexBuffers[i]->userData = NULL;
		}		
	}

	// IB binding
	for (uint32 i=0; i < m_indexBuffers.size(); ++i)
	{
		if (m_indexBuffers[i])
		{
			m_indexBuffers[i]->userData = NULL;
		}
		
	}

	// texture bindings
	SrResourceLibrary::iterator it = m_textureLibrary.begin();
	for(; it != m_textureLibrary.end(); ++it)
	{
		SrTexture* tex = (SrTexture*)(it->second);
		tex->m_userData = NULL;
	}


}
