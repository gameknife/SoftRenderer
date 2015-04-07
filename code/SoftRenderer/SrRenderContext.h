/**
  @file SrRenderContext.h
  
  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef SrRenderContext_h__
#define SrRenderContext_h__

class SrFragmentBuffer;

struct SrRendContext
{
	SrRendContext(int w, int h, int obpp)
	{
			memset(this, 0, sizeof(SrRendContext));
			width = w;
			height = h;
			bpp = obpp / 8;
			viewport = SrViewport(0.f,0.f,(float)width,(float)height,1.f,1000.f);

			SYSTEM_INFO siSysInfo;
			GetSystemInfo(&siSysInfo); 

			processorNum = siSysInfo.dwNumberOfProcessors;
	}

	// 渲染特性相关
	void OpenFeature( ERenderFeature feature )
	{
		features |= feature;
	}
	void CloseFeature( ERenderFeature feature )
	{
		features &= ~feature;
	}
	BOOL IsFeatureEnable( ERenderFeature feature )
	{
		return features & feature;
	}
	
	// 详细参数
	int width;							///< 渲染器宽度
	int height;							///< 渲染器高度
	SrViewport viewport;				///< 3维视口
	int bpp;							///< 像素位宽，单位byte
	uint32 features;					///< 渲染特性
	int processorNum;					///< 处理器个数

	SrFragmentBuffer* fBuffer;			///< FragBuffer
};
extern SrRendContext* g_context;

#endif // SrRenderContext_h__


