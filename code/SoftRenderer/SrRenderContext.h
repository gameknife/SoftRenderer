/**
  @file SrRenderContext.h
  
  @author yikaiming

  ver:1.0
   
 */

#ifndef SrRenderContext_h__
#define SrRenderContext_h__

class SrFragmentBuffer;

SR_ALIGN struct SrRendContext
{
	SrRendContext(int w, int h, int obpp)
	{
			memset(this, 0, sizeof(SrRendContext));
			width = w;
			height = h;
			bpp = obpp / 8;
			viewport = SrViewport(0.f,0.f,(float)width,(float)height,1.f,1000.f);

			//SYSTEM_INFO siSysInfo;
			//GetSystemInfo(&siSysInfo); 

			processorNum = 24;//siSysInfo.dwNumberOfProcessors;
	}

	void OpenFeature( ERenderFeature feature )
	{
		features |= feature;
	}
	void CloseFeature( ERenderFeature feature )
	{
		features &= ~feature;
	}
	bool IsFeatureEnable( ERenderFeature feature )
	{
		return features & feature;
	}
	
	uint32 width;						
	uint32 height;						
	SrViewport viewport;				
	int bpp;							
	uint32 features;					
	int processorNum;					

	SrFragmentBuffer* fBuffer;			///< FragBuffer
};
extern SrRendContext* g_context;

#endif 


