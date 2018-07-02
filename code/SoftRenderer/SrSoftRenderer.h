/**
  @file SrRenderer.h
  
  @author yikaiming

  ������־ history
  ver:1.0
   
 */

#ifndef SrRenderer_h__
#define SrRenderer_h__
#include "prerequisite.h"
#include "RendererBase.h"

struct SrFragmentBuffer;

/**
 *@brief ��Ⱦ��
 */
class SrSoftRenderer : public IRenderer
{
	typedef std::vector<SrSwShader*> SrSwShaders;
	friend SrRasterizer;

public:
	SrSoftRenderer(void);
	virtual ~SrSoftRenderer(void);

	virtual const char* getName();

	// ����رպ���
	bool InitRenderer(int width, int height, int bpp);
	bool ShutdownRenderer();
	bool Resize(uint32 width, uint32 height);
	virtual uint32 getWidth();
	virtual uint32 getHeight();

	// ֡���ƺ���
	void BeginFrame();
	void EndFrame();

	// Ӳ��Clear
	bool HwClear();

	// ��ȡScreenBuffer����
	virtual const uint8* getBuffer();
	virtual int getBufferLength();

	// ����ͨ������
	bool SetTextureStage( const SrTexture* texture, int stage );
	void ClearTextureStage();
	
	// ��Ⱦ����
	bool DrawPrimitive( SrPrimitve* primitive );
	bool DrawLine(const float3& from, const float3& to);
	bool DrawScreenText(const char* str, int x,int y, uint32 size, uint32 color = SR_UICOLOR_HIGHLIGHT);

	// Shader����
	virtual bool SetShader( const SrShader* shader );
	virtual bool SetShaderConstant( uint32 slot, const float* constantStart, uint32 vec4Count );

	virtual uint32 Tex2D( const float2& texcoord, const SrTexture* texture ) const;

private:
	// ����Ӳ��֡����
	bool Swap();
	// ����Ӳ��buffer����
	bool CreateHwBuffer();
	// ������������
	void FlushText();

	virtual SrVertexBuffer* AllocateNormalizedVertexBuffer( uint32 count, bool fastmode = false );

	virtual bool InnerInitShaders();

	// ��դ������
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

	SrTextLines m_textLines;
};

#endif // SrRenderer_h__


