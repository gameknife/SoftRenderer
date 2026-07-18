/**
  @file SrRenderContext.h
  
  @author yikaiming

  ������־ history
  ver:1.0
   
 */

#ifndef SrRenderContext_h__
#define SrRenderContext_h__

class SrFragmentBuffer;

struct SrRendContext
{
	void* operator new(size_t size) {return _mm_malloc_custom(size, 16);}
	void operator delete(void* memoryToBeDeallocated) {_mm_free_custom(memoryToBeDeallocated);}

	SrRendContext(int w, int h, int obpp)
	{
			memset(this, 0, sizeof(SrRendContext));
			width = w;
			height = h;
			bpp = obpp / 8;
			viewport = SrViewport(0.f,0.f,(float)width,(float)height,1.f,1000.f);

			const int hardwareThreads = (std::max)(1u, std::thread::hardware_concurrency());
			processorNum = hardwareThreads;
			const char* configuredWorkers = getenv("SR_WORKER_COUNT");
			if (configuredWorkers)
			{
				const int requested = atoi(configuredWorkers);
				if (requested > 0)
					processorNum = (std::min<int>)(requested, hardwareThreads);
			}
	}

	// ��Ⱦ�������
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
	
	// ��ϸ����
	int width;							///< ��Ⱦ�����
	int height;							///< ��Ⱦ���߶�
	SrViewport viewport;				///< 3ά�ӿ�
	int bpp;							///< ����λ�����λbyte
	uint32 features;					///< ��Ⱦ����
	int processorNum;					///< ����������

	SrFragmentBuffer* fBuffer;			///< FragBuffer
};
extern SrRendContext* g_context;

#endif // SrRenderContext_h__

