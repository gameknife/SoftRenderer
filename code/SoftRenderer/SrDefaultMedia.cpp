/**
  @file SrDefaultMedia.cpp
  
  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#include "stdafx.h"
#include "SrDefaultMedia.h"
#include "SrTexture.h"
#include "SrObjLoader.h"

#include "mmgr.h"


static const char* g_defaultMesh = "\
	v  -5.0000 0.0000 5.0000\n\
	v  -5.0000 0.0000 -5.0000\n\
	v  5.0000 0.0000 -5.0000\n\
	v  5.0000 0.0000 5.0000\n\
	v  -5.0000 10.0000 5.0000\n\
	v  5.0000 10.0000 5.0000\n\
	v  5.0000 10.0000 -5.0000\n\
	v  -5.0000 10.0000 -5.0000\n\
	vn 0.0000 -1.0000 -0.0000\n\
	vn 0.0000 1.0000 -0.0000\n\
	vn 0.0000 0.0000 1.0000\n\
	vn 1.0000 0.0000 -0.0000\n\
	vn 0.0000 0.0000 -1.0000\n\
	vn -1.0000 0.0000 -0.0000\n\
	vt -1.0000 0.0000 0.0000\n\
	vt -1.0000 -1.0000 0.0000\n\
	vt 0.0000 -1.0000 0.0000\n\
	vt 0.0000 0.0000 0.0000\n\
	g Box001\n\
	s 2\n\
	f 1/1/1 2/2/1 3/3/1\n\
	f 3/3/1 4/4/1 1/1/1\n\
	s 4\n\
	f 5/4/2 6/1/2 7/2/2\n\
	f 7/2/2 8/3/2 5/4/2\n\
	s 8\n\
	f 1/4/3 4/1/3 6/2/3\n\
	f 6/2/3 5/3/3 1/4/3\n\
	s 16\n\
	f 4/4/4 3/1/4 7/2/4\n\
	f 7/2/4 6/3/4 4/4/4\n\
	s 32\n\
	f 3/4/5 2/1/5 8/2/5\n\
	f 8/2/5 7/3/5 3/4/5\n\
	s 64\n\
	f 2/4/6 1/1/6 5/2/6\n\
	f 5/2/6 8/3/6 2/4/6\n";

static const char* g_defaultMaterial = "newmtl $srdefualt\n\
	Ns 40.0000\n\
	Ni 1.5000\n\
	Nfsp 2.3\n\
	Nfsb 1.0\n\
	Nfss 0.1\n\
	d 1.0000\n\
	Tr 0.0000\n\
	Tf 1.0000 1.0000 1.0000\n\
	illum 2\n\
	Ka 0.0510 0.0510 0.0510\n\
	Kd 0.5880 0.5880 0.5880\n\
	Ks 1.7616 1.7035 1.5995\n\
	Ke 0.0000 0.0000 0.0000\n\
	map_Kd $default_d\n\
	map_Kb $default_n\n\
	";

SrDefaultMediaPack::SrDefaultMediaPack( void )
{
	// 程序顺序保证这里能够 得到 resourcemanager
	defaultDiffuse = gEnv->resourceMgr->CreateRenderTexture("$default_d", 4, 4, 4);
	defaultFlat = gEnv->resourceMgr->CreateRenderTexture("$default_n", 4, 4, 4);

	// 写render texture
	uint32* buffer = NULL;
	
	// diffuse
	// bitmap access is BGRA
	buffer = (uint32*)(defaultDiffuse->getBuffer());
	*(buffer + 0 ) = 0xff2f2f2f; *(buffer + 1 ) = 0xff2f2f2f; *(buffer + 2 ) = 0xff7f7f7f; *(buffer + 3 ) = 0xff7f7f7f;
	*(buffer + 4 ) = 0xff2f2f2f; *(buffer + 5 ) = 0xff2f2f2f; *(buffer + 6 ) = 0xff7f7f7f; *(buffer + 7 ) = 0xff7f7f7f;
	*(buffer + 8 ) = 0xff7f7f7f; *(buffer + 9 ) = 0xff7f7f7f; *(buffer + 10) = 0xff2f2f2f; *(buffer + 11) = 0xff2f2f2f;
	*(buffer + 12) = 0xff7f7f7f; *(buffer + 13) = 0xff7f7f7f; *(buffer + 14) = 0xff2f2f2f; *(buffer + 15) = 0xff2f2f2f;


	// flat
	buffer = (uint32*)(defaultFlat->getBuffer());

	for (int i=0; i < 16; ++i)
	{
		*(buffer + i) = 0xff7f7fff;
	}

	// create defult mtl
	SrObjLoader loader;
	// load file in memory
	loader.LoadMaterialFromMTL( g_defaultMaterial );
}

SrDefaultMediaPack::~SrDefaultMediaPack( void )
{
	// 资源管理器会自动删除texture
}

const char* SrDefaultMediaPack::getDefaultMesh() const
{
	return g_defaultMesh;
}

const char* SrDefaultMediaPack::getDefaultMtl() const
{
	return g_defaultMaterial;
}
