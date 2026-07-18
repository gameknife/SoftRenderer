/**
  @file SrRenderer.h
  
  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef SrRenderer_h__
#define SrRenderer_h__
#include "prerequisite.h"
#include "RendererBase.h"

class SrFragmentBuffer;

/**
 *@brief 渲染器
 */
class SrSoftRenderer : public IRenderer
{
	typedef std::vector<SrSwShader*> SrSwShaders;
	friend SrRasterizer;

public:
	SrSoftRenderer(void);
	virtual ~SrSoftRenderer(void);

	virtual const char* getName();

	// 启动，关闭函数
	bool InitRenderer(HWND hWnd, int width, int height, int bpp);
	bool ShutdownRenderer();
	bool Resize(uint32 width, uint32 height);
	virtual uint32 getWidth();
	virtual uint32 getHeight();

	// 帧控制函数
	void BeginFrame();
	void EndFrame();

	// 硬件Clear
	bool HwClear();

	// 获取ScreenBuffer函数
	void* getBuffer();
	
	// 纹理通道设置
	bool SetTextureStage( const SrTexture* texture, int stage );
	void ClearTextureStage();
	
	// 渲染调用
	bool DrawPrimitive( SrPrimitve* primitive );
	bool DrawLine(const float3& from, const float3& to);

	// Shader设置
	virtual bool SetShader( const SrShader* shader );
	virtual bool SetShaderConstant( uint32 slot, const float* constantStart, uint32 vec4Count );

	virtual uint32 Tex2D( float2& texcoord, const SrTexture* texture ) const;

private:
	// 交换硬件帧缓冲
	bool Swap();
	// 创建硬件buffer函数
	bool CreateHwBuffer();

	virtual SrVertexBuffer* AllocateNormalizedVertexBuffer( uint32 count, bool fastmode = false );

	virtual bool InnerInitShaders();

	

	HWND m_window;
	std::vector<uint32> m_frameBuffer;
	
	// 光栅处理器
	SrRasterizer* m_rasterizer;

	uint32 m_renderState;

	void* m_cachedBuffer;
	int m_bufferPitch;

	SrBitmapArray m_textureStages;

	SrRendVertex* m_normalizeVertexBuffer;
	uint32 m_normalizeVBAllocSize;
	SrVertexBufferArray m_normlizedVBs;

	const SrSwShader* m_currShader;
	SrSwShaders m_swShaders;
	float4* m_shaderConstants;
	SrHandleList m_swHandles;

};

#endif // SrRenderer_h__

