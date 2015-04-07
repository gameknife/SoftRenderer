/**
  @file SrRenderTexture.h
  
  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef SrRenderTexture_h__
#define SrRenderTexture_h__

#include "prerequisite.h"
#include "SrTexture.h"

class SrRenderTexture : public SrTexture
{
public:
	SrRenderTexture(const char* name, int width, int height, int bpp);
	~SrRenderTexture(void);

private:
	SrRenderTexture(void);
};


#endif // SrRenderTexture_h__

