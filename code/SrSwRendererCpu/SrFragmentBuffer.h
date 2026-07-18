/**
  @file SrFragmentBuffer.h
  
  @author yikaiming

  ������־ history
  ver:1.0
   
 */

#ifndef SrFragmentBuffer_h__
#define SrFragmentBuffer_h__
#include "prerequisite.h"
#include <atomic>

struct  SrRendContext;
class SrSoftRenderer;

class SrFragmentBuffer
{
	friend SrRendContext;
public:
	SrFragmentBuffer(int width, int height, SrSoftRenderer* renderer);
	~SrFragmentBuffer(void);

	LPCRITICAL_SECTION GetSyncMark(uint32 address)
	{
	#ifdef RASTERIZER_SYNC
		return &(fragBufferSync[address].cs);
	#else
		return NULL;
	#endif
	}

	inline float3 GetNormal(float2& texcoord) const;
	float3 GetWorldPos(float2& texcoord) const
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

	// Relaxed tile rendering only exposes pixels from tiles whose raster writes
	// have been published. Exact rendering leaves this disabled and preserves
	// the original full-frame lookup behavior.
	void ConfigureRelaxedTileReads(std::atomic<LONG>* tileReady, uint32 tileSize, uint32 tileColumns);
	void DisableRelaxedTileReads();
	bool TryGetWorldPos(float2 texcoord, uint32 currentAddress, float3& worldPos) const
	{
		float u = texcoord.x - floor(texcoord.x);
		float v = texcoord.y - floor(texcoord.y);
		const uint32 x = static_cast<uint32>(u * m_width) % m_width;
		const uint32 y = static_cast<uint32>(v * m_height) % m_height;

		if (m_tileReady)
		{
			const uint32 currentY = currentAddress / m_width;
			const uint32 currentX = currentAddress - currentY * m_width;
			const uint32 currentTile = (currentY / m_tileSize) * m_tileColumns + currentX / m_tileSize;
			const uint32 sampleTile = (y / m_tileSize) * m_tileColumns + x / m_tileSize;
			if (sampleTile != currentTile && m_tileReady[sampleTile].load(std::memory_order_acquire) == 0)
				return false;
		}

		worldPos = fBuffer[y * m_width + x].worldpos_tx.xyz;
		return true;
	}

	SrIndexBuffer* GetPixelIndicesBuffer() {return m_fBufferIndices;}

	void Clear();

public:
	SrFragment*				fBuffer;		///< fragBuffer
	float*					zBuffer;

private:
	SrIndexBuffer*			m_fBufferIndices;	///< fragBufferIndices����
	
	#ifdef RASTERIZER_SYNC
	SrFragmentBufferSync*	fragBufferSync;
	#endif

	uint32					m_width;
	uint32					m_height;
	std::atomic<LONG>*		m_tileReady;
	uint32					m_tileSize;
	uint32					m_tileColumns;

	SrSoftRenderer*			m_renderer;
};
extern SrFragmentBuffer* fBuffer;


#endif // SrFragmentBuffer_h__


