/**
  @file IRenderer.h
  
  @brief renderer interface

  @author yikaiming

  history
  ver:1.0
   
 */

#ifndef IRenderer_h__
#define IRenderer_h__

#include "prerequisite.h"

class SrTexture;
class SrRasterizer;
class SrResourceManager;
class SrShader;

struct SrTextLine
{
	std::string text;
	int2 pos;
	uint32 color;
	uint32 size;
};
typedef std::vector<SrTextLine> SrTextLines;

enum class EHwTimerElement : uint8
{
	eHt_GpuFlushTime = 0,
	eHt_GpuHairTime,
	eHt_GpuPostProcessTime,

	eHt_Count,
};

enum class ERendererType : uint8
{
	eRt_Software,
	eRt_HardwareD3D9,
	eRt_HardwareD3D11,
};

// Renderer Interface
class IRenderer
{
public:
	IRenderer( ERendererType type ):m_rendererType(type)
	{
		m_matrixStack.assign( eMd_Count, float44::CreateIdentity() );
		m_frameCount = 0;
	}
	virtual ~IRenderer(void)
	{

	}

	virtual const char* getName() =0;

	virtual bool Resize(uint32 width, uint32 height)=0;
	virtual uint32 getWidth() =0;
	virtual uint32 getHeight() =0;

	virtual void BeginFrame()=0;
	virtual void EndFrame()=0;

	virtual bool HwClear()=0;

	virtual const uint8* getBuffer() = 0;
	virtual int getBufferLength() = 0;

	virtual bool SetTextureStage( const SrTexture* texture, int stage )=0;
	virtual void ClearTextureStage()=0;

	virtual bool UpdateVertexBuffer(SrVertexBuffer* target) {return true;}
	virtual bool UpdateIndexBuffer(SrIndexBuffer* target) {return true;}

	virtual bool DrawPrimitive( SrPrimitve* primitive )=0;
	virtual bool DrawLine(const float3& from, const float3& to)=0;
	virtual bool DrawScreenText(const char* str, int x,int y, uint32 size, uint32 color = SR_UICOLOR_HIGHLIGHT) =0;

	virtual bool SetShader(const SrShader* shader) =0;
	virtual bool SetShaderConstant( uint32 slot, const float* constantStart, uint32 vec4Count ) =0;

	virtual void SetGpuMarkStart(EHwTimerElement element) {}
	virtual void SetGpuMarkEnd(EHwTimerElement element) {}
	virtual float GetGpuTime(EHwTimerElement element) {return 0;}

	void SetMatrix( EMatrixDefine index, const float44& matrix )
	{
		if (index < eMd_Count)
		{
			m_matrixStack[index] = matrix;
		}	
	}

	float44 GetMatrix( EMatrixDefine index )
	{
		float44 ret = float44::CreateIdentity();
		if (index < eMd_Count)
		{
			ret = m_matrixStack[index];
		}

		return ret;
	}

	// TEX2D for swRenderer
	virtual uint32 Tex2D(const float2& texcoord, const SrTexture* texture ) const =0;


	// FrameCount
	uint32 getFrameCount() {return m_frameCount;}

	ERendererType m_rendererType;

	virtual bool InitRenderer(int width, int height, int bpp) =0;
	virtual bool ShutdownRenderer() =0;

protected:
	SrMatrixArray m_matrixStack;
	
	uint32 m_frameCount;	
};



#endif // IRenderer_h__