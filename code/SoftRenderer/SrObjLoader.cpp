#include "stdafx.h"
#include "SrObjLoader.h"
#include <sstream>
#include "SrMaterial.h"

SrObjLoader::SrObjLoader(void)
{
	nullCache = NULL;
}


SrObjLoader::~SrObjLoader(void)
{
}


//////////////////////////////////////////////////////////////////////////
bool SrObjLoader::IsMatIDExist( uint32 matID )
{
	int num = (int)m_Attributes.size();
	for(int i=0; i < num; i++)
	{
		if (m_Attributes[i] == matID)
		{
			return true;
		}
	}

	return false;
}
//--------------------------------------------------------------------------------------
uint32 SrObjLoader::AddVertex( uint32 hash, SrVertexP3N3T2* pVertex )
{
	// If this vertex doesn't already exist in the Vertices list, create a new entry.
	// Add the index of the vertex to the Indices list.
	bool bFoundInList = false;
	uint32 index = 0;

	// Since it's very slow to check every element in the vertex list, a hashtable stores
	// vertex indices according to the vertex position's index as reported by the OBJ file
	if( ( uint32 )m_VertexCache.size() > hash )
	{
		CacheEntry* pEntry = m_VertexCache[hash];
		while( pEntry != NULL )
		{
			SrVertexP3N3T2* pCacheVertex = m_Vertices.data() + pEntry->index;

			// If this vertex is identical to the vertex already in the list, simply
			// point the index buffer to the existing vertex
			if( 0 == memcmp( pVertex, pCacheVertex, sizeof( SrVertexP3N3T2 ) ) )
			{
				bFoundInList = true;
				index = pEntry->index;
				break;
			}

			pEntry = pEntry->pNext;
		}
	}

	// Vertex was not found in the list. Create a new entry, both within the Vertices list
	// and also within the hashtable cache
	if( !bFoundInList )
	{
		// Add to the Vertices list
		index = (uint32)m_Vertices.size();
		m_Vertices.push_back( *pVertex );

		// Add this to the hashtable
		CacheEntry* pNewEntry = new CacheEntry;
		if( pNewEntry == NULL )
			return 0;

		pNewEntry->index = index;
		pNewEntry->pNext = NULL;

		// Grow the cache if needed
		if ( ( uint32 )m_VertexCache.size() <= hash )
		{
			m_VertexCache.resize( hash + 1, nullCache );
		}
// 		while( ( uint32 )m_VertexCache.size() <= hash )
// 		{
// 			m_VertexCache.push_back( NULL );
// 		}

		// Add to the end of the linked list
		CacheEntry* pCurEntry = m_VertexCache[hash];
		if( pCurEntry == NULL )
		{
			// This is the head element
			m_VertexCache[hash] = pNewEntry;
		}
		else
		{
			// Find the tail
			while( pCurEntry->pNext != NULL )
			{
				pCurEntry = pCurEntry->pNext;
			}

			pCurEntry->pNext = pNewEntry;
		}
	}

	return index;
}


//--------------------------------------------------------------------------------------
void SrObjLoader::DeleteCache()
{
	// Iterate through all the elements in the cache and subsequent linked lists
	for( uint32 i = 0; i < m_VertexCache.size(); i++ )
	{
		CacheEntry* pEntry = m_VertexCache[i];
		while( pEntry != NULL )
		{
			CacheEntry* pNext = pEntry->pNext;
			if (pEntry)
			{
				delete pEntry;
			}
			pEntry = pNext;
		}
	}

	m_VertexCache.clear();
}

//--------------------------------------------------------------------------------------
bool SrObjLoader::LoadGeometryFromOBJ( const char* pMeshData, SrPrimitives& primitives )
{
	// Find the file

	// File input
	char strCommand[256] = {0};
	std::stringstream InFile( pMeshData );
	if( !InFile )
		return false;

	bool flushFace = false;
	std::string currMtlName;

	for(; ; )
	{
		InFile >> strCommand;
		if( !InFile )
			break;

		if( 0 == strcmp( strCommand, "#" ) )
		{
			// Comment
		}
		else if( 0 == strcmp( strCommand, "g" ) )
		{
			flushFace = true;
		}
		else if( 0 == strcmp( strCommand, "usemtl" ) )
		{
			InFile >> currMtlName;
		}
		else if( 0 == strcmp( strCommand, "mtllib" ) )
		{

		}
		else if( 0 == strcmp( strCommand, "v" ) )
		{
			if (flushFace)
			{
				flushFace = false;

				// CreateMesh
				CreateMeshInternal(primitives);

				primitives.back().material = gEnv->resourceMgr->LoadMaterial(currMtlName.c_str());				
			}
			// Vertex Position
			float x, y, z;
			InFile >> x >> y >> z;

			Positions.push_back( float4( x, y, z, 1 ) );
		}
		else if( 0 == strcmp( strCommand, "vt" ) )
		{
			// Vertex TexCoord
			float u, v;
			InFile >> u >> v;
			TexCoords.push_back( float2( u, 1-v ) );
		}
		else if( 0 == strcmp( strCommand, "vn")  )
		{
			// Vertex Normal
			float x, y, z;
			InFile >> x >> y >> z;
			Normals.push_back( float3( x, y, z) );
		}
		else if( 0 == strcmp( strCommand, "f" ) )
		{
			// Face
			int iPosition, iTexCoord, iNormal;
			SrVertexP3N3T2 vertex;
			uint16	dwIndex[3];
			for( uint32 iFace = 0; iFace < 3; iFace++ )
			{
				memset( &vertex, 0, sizeof( SrVertexP3N3T2 ) );

				// OBJ format uses 1-based arrays
				InFile >> iPosition;
				iPosition = abs(iPosition);
				vertex.pos = Positions[ iPosition - 1 ];

				if( '/' == InFile.peek() )
				{
					InFile.ignore();

					if( '/' != InFile.peek() )
					{
						// Optional texture coordinate
						InFile >> iTexCoord;
						iTexCoord = abs(iTexCoord);
						vertex.texcoord = TexCoords[ iTexCoord - 1];

					}

					if( '/' == InFile.peek() )
					{
						InFile.ignore();

						// Optional vertex normal
						InFile >> iNormal;
						iNormal = abs(iNormal);
						vertex.normal = Normals[ iNormal - 1 ];
					}
				}

				dwIndex[iFace] = AddVertex( iPosition, &vertex );			
			}
			// gkEngine, Add Indice
			m_Indices.push_back( dwIndex[0] );
			m_Indices.push_back( dwIndex[1] );
			m_Indices.push_back( dwIndex[2] );
		}
		else
		{
			// Unimplemented or unrecognized command
		}

		InFile.ignore( 1000, '\n' );
	}

	if (flushFace)
	{
		flushFace = false;

		// CreateMesh
		CreateMeshInternal(primitives);
		primitives.back().material = gEnv->resourceMgr->LoadMaterial(currMtlName.c_str());				
	}
	
	return true;
}

void SrObjLoader::ClearData()
{
	m_Vertices.clear();     // Filled and copied to the vertex buffer
	m_Indices.clear();       // Filled and copied to the index buffer
	m_Attributes.clear();    // Filled and copied to the attribute buffer
}


void SrObjLoader::CreateMeshInternal(SrPrimitives& primitives )
{
	// Cleanup
	DeleteCache();
	m_bIsLoaded = true;

	// Create Mesh
	SrVertexBuffer* vb = gEnv->resourceMgr->AllocateVertexBuffer( sizeof(SrVertexP3N3T2), (uint32)m_Vertices.size() );
	SrIndexBuffer* ib = gEnv->resourceMgr->AllocateIndexBuffer((uint32)m_Indices.size() );

	memcpy( vb->data, m_Vertices.data(), sizeof(SrVertexP3N3T2) * (uint32)m_Vertices.size() );
	memcpy( ib->data, m_Indices.data(), sizeof(uint32) * (uint32)m_Indices.size() );

	gEnv->renderer->UpdateVertexBuffer(vb);
	gEnv->renderer->UpdateIndexBuffer(ib);

	SrPrimitve primitive;
	primitive.vb = vb;
	primitive.ib = ib;
	primitive.cachedVb = NULL;

	primitives.push_back(primitive);

	ClearData();

}

struct SrMatLoadingParam
{
	float4 Ka;
	float4 Kd;
	float4 Ks;
	float Glossness;
	float FresnelPower;
	float FresnelBia;
	float FresnelScale;
	float AlphaTest;

	std::string Kd_map;
	std::string Kb_map;
	std::string Ks_map;
	std::string Kr_map;
	std::string kspc0_map;
	std::string shading_mode;
	std::string mtlname;

	SrMatLoadingParam()
	{
		Ka = float4(0,0,0,0);
		Kd = float4(1.f,1.f,1.f,1.f);
		Ks = float4(.5f,.5f,.5f,1.f);
		Glossness = 25.f;
		FresnelPower = 5.f;
		FresnelBia = 1.f;
		FresnelScale = 1.f;
		AlphaTest = 0;
	}

	void CreateMat()
	{
		if (mtlname != "")
		{
			// create mtl
			SrMaterial* matptr = gEnv->resourceMgr->CreateMaterial(mtlname.c_str());
			SrMaterial& mat = *matptr;

			mtlname = "";

			mat.m_matDiffuse = Kd;
			mat.m_matSpecular = Ks;
			mat.m_glossness = Glossness;
			mat.m_fresnelPower = FresnelPower;
			mat.m_fresnelBia = FresnelBia;
			mat.m_fresnelScale = FresnelScale;

			mat.m_alphaTest = AlphaTest;

			// texture
			mat.m_textures.clear();

			if ( !Kd_map.empty())
			{
				mat.m_textures.push_back( gEnv->resourceMgr->LoadTexture(Kd_map.c_str()));
			}
			else
			{
				mat.m_textures.push_back( gEnv->resourceMgr->LoadTexture("$default_d"));
			}

			if ( !Kb_map.empty())
			{
				mat.m_textures.push_back( gEnv->resourceMgr->LoadTexture(Kb_map.c_str(), true));
			}
			else
			{
				mat.m_textures.push_back( gEnv->resourceMgr->LoadTexture("$default_n", true));
			}

			if ( !Ks_map.empty())
			{
				mat.m_textures.push_back( gEnv->resourceMgr->LoadTexture(Ks_map.c_str()));
			}
			else
			{
				mat.m_textures.push_back( gEnv->resourceMgr->LoadTexture("$default_d"));
			}

			if ( !Kr_map.empty())
			{
				mat.m_textures.push_back( gEnv->resourceMgr->LoadTexture(Kr_map.c_str()));
			}
			else
			{
				mat.m_textures.push_back( gEnv->resourceMgr->LoadTexture("$default_d"));
			}

			if ( !kspc0_map.empty())
			{
				mat.m_textures.push_back( gEnv->resourceMgr->LoadTexture(kspc0_map.c_str()));
			}

			// clear
			Kr_map.clear();
			Ks_map.clear();
			Kb_map.clear();
			Kd_map.clear();
			kspc0_map.clear();
			AlphaTest = false;
		}
	}
};



bool SrObjLoader::LoadMaterialFromMTL( const char* strFileData )
{
	// Find the file

	// File input
	char strCommand[256] = {0};
	std::stringstream InFile( strFileData );
	if( !InFile )
		return false;

	SrMatLoadingParam param;

	for(; ; )
	{
		InFile >> strCommand;
		if( !InFile )
			break;

		if( 0 == strcmp( strCommand, "#") ) 
		{
			// Comment
		}
		else if( 0 == strcmp( strCommand, "newmtl") ) 
		{
			param.CreateMat();
			InFile >> param.mtlname;
		}
		else if( 0 == strcmp( strCommand, "shading") ) 
		{
			InFile >> param.shading_mode;
		}
		else if( 0 == strcmp( strCommand, "Ka") ) 
		{
			//InFile >> Ka.r >> Ka.g >> Ka.b;
		}
		else if( 0 == strcmp( strCommand, "Kd") ) 
		{
			InFile >> param.Kd.r >> param.Kd.g >> param.Kd.b;
		}
		else if( 0 == strcmp( strCommand, "Ks"))  
		{
			InFile >> param.Ks.r >> param.Ks.g >> param.Ks.b;
		}
		else if( 0 == strcmp( strCommand, "Ns") ) 
		{
			InFile >> param.Glossness;
			param.Glossness = Clamp(param.Glossness, 0.0f, 255.f);
		}
		else if( 0 == strcmp( strCommand, "Nfsp") ) 		// custom attr: fresnel power
		{
			InFile >> param.FresnelPower;
		}
		else if( 0 == strcmp( strCommand, "Nfsb") ) 		// custom attr: fresnel bia
		{
			InFile >> param.FresnelBia;
		}
		else if( 0 == strcmp( strCommand, "Nfss") ) 		// custom attr: fresnel scale
		{
			InFile >> param.FresnelScale;
		}
		else if( 0 == strcmp( strCommand, "map_Kd") ) 
		{		
			InFile >> param.Kd_map;
		}
		else if( 0 == strcmp( strCommand, "map_Kb") ) 
		{
			InFile >> param.Kb_map;
		}
		else if( 0 == strcmp( strCommand, "bump") ) 
		{
			InFile >> param.Kb_map;
		}
		else if( 0 == strcmp( strCommand, "map_Ks") ) 
		{
			InFile >> param.Ks_map;
		}
		else if( 0 == strcmp( strCommand, "map_Kr") ) 
		{
			InFile >> param.Kr_map;
		}
		else if( 0 == strcmp( strCommand, "map_Kspc0") ) 
		{
			InFile >> param.kspc0_map;
		}
		else if( 0 == strcmp( strCommand, "alpha_test") ) 
		{
			InFile >> param.AlphaTest;
		}
		else
		{
			// Unimplemented or unrecognized command
		}

		InFile.ignore( 1000, '\n' );
	}

	m_bIsLoaded = true;

	// create material
	param.CreateMat();

	return true;
}



