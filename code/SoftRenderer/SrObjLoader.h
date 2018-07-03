/**
  @file SrObjLoader.h
  
  @author Kaiming

  ������־ history
  ver:1.0
   
 */

#ifndef SrObjLoader_h__
#define SrObjLoader_h__



class SrObjLoader
{

	SR_ALIGN struct CacheEntry
	{
		uint32 index;
		CacheEntry* pNext;
	};
public:
	SrObjLoader(void);
	~SrObjLoader(void);

public:
	uint32 GetSubsetCount() {return m_uSubsetCount < 2 ? 1 : m_uSubsetCount;}
	uint32 m_uSubsetCount;
	bool LoadGeometryFromOBJ( const char* strMeshData, SrPrimitives& primitives );
	bool LoadMaterialFromMTL( const char* strFileData);
private:
	uint16 AddVertex( uint32 hash, SrVertexP3N3T2* pVertex );
	bool IsMatIDExist( uint32 matID );
	void DeleteCache();
	void ClearData();
	void CreateMeshInternal(SrPrimitives& primitives );

	bool	m_bIsLoaded;

	std::vector<float4> Positions;
	std::vector<float2> TexCoords;
	std::vector<float3> Normals;

	std::vector<CacheEntry*> m_VertexCache;   // Hashtable cache for locating duplicate vertices
	std::vector<SrVertexP3N3T2> m_Vertices;     // Filled and copied to the vertex buffer
	std::vector<uint32> m_Indices;       // Filled and copied to the index buffer
	std::vector<uint32> m_Attributes;    // Filled and copied to the attribute buffer

	CacheEntry* nullCache;
};

#endif


