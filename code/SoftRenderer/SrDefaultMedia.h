/**
  @file SrDefaultMedia.h
  
  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef SrDefaultMedia_h__
#define SrDefaultMedia_h__

#include "prerequisite.h"

SR_ALIGN struct SrDefaultMediaPack
{
	SrDefaultMediaPack(void);
	~SrDefaultMediaPack(void);

	const char* getDefaultMesh() const;
	const char* getDefaultMtl() const;
	SrTexture* getDefaultTex() {return defaultDiffuse;}
	SrTexture* getDefaultFlatTex() {return defaultFlat;}

	SrTexture* defaultDiffuse;
	SrTexture* defaultFlat;
};

#endif // SrDefaultMedia_h__