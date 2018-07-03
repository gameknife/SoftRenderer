/**
  @file SrFragmentBuffer.h
  
  @author yikaiming

  ������־ history
  ver:1.0
   
 */

#ifndef SrFragmentBuffer_h__
#define SrFragmentBuffer_h__
#include "prerequisite.h"

struct  SrRendContext;
class SrSoftRenderer;

class SrFragmentBuffer
{
	friend SrRendContext;
public:
	SrFragmentBuffer(int width, int height, SrSoftRenderer* renderer);
	~SrFragmentBuffer(void);

	// LPCRITICAL_SECTION GetSyncMark(uint32 address)
	// {
	// 	return &(fragBufferSync[address].cs);
	// }

	inline float3 GetNormal(const float2& texcoord) const;
	float3 GetWorldPos(const float2& texcoord) const
	{
		// ��������warp
		float u = texcoord.x - floor(texcoord.x);
		float v = texcoord.y - floor(texcoord.y);

		// �ٽ������
		u *= (m_width);
		v *= (m_height);

		// get int
		int x = (int)( u );
		int y = (int)( v );
		x = x % m_width;
		y = y % m_height;

		return fBuffer[y * m_width + x].worldpos_tx.xyz;
	}

	SrIndexBuffer* GetPixelIndicesBuffer() {return m_fBufferIndices;}

	void Clear();

public:
	SrFragment*				fBuffer;		///< fragBuffer
	float*					zBuffer;

private:
	SrIndexBuffer*			m_fBufferIndices;	///< fragBufferIndices����
	
	SrFragmentBufferSync*	fragBufferSync;

	uint32					m_width;
	uint32					m_height;

	SrSoftRenderer*			m_renderer;
};
extern SrFragmentBuffer* fBuffer;


#endif // SrFragmentBuffer_h__


