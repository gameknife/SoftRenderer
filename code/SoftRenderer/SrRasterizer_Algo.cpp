/**
  @file SrRasterizer_Algo.cpp
  
  @author yikaiming

  ver:1.0
   
 */

#include "stdafx.h"
#include "SrRasterizer.h"
#include "SrProfiler.h"
#include "SrSwShader.h"
#include "SrFragmentBuffer.h"


void SrRasterizer::RasterizeTriangle( SrRastTriangle& calTri, bool subtri )
{
	if(!subtri)
	{
		if (calTri.primitive->shaderConstants.culling)
		{
			assert( abs(calTri.p[0].pos.w) > SR_EQUAL_PRECISION );
			assert( abs(calTri.p[1].pos.w) > SR_EQUAL_PRECISION );
			assert( abs(calTri.p[2].pos.w) > SR_EQUAL_PRECISION );

			float3 v0v1 = calTri.p[1].pos.xyz / calTri.p[1].pos.w - calTri.p[0].pos.xyz / calTri.p[0].pos.w;
			float3 v1v2 = calTri.p[2].pos.xyz / calTri.p[2].pos.w - calTri.p[1].pos.xyz / calTri.p[1].pos.w;

			v0v1.z = 0;
			v1v2.z = 0;

			float3 normal = v0v1 % v1v2;
			if (normal.z > 0)
			{
				// ccw
				return;
			}
		}


		gEnv->profiler->setIncrement(ePe_TriangleCount);

		// Patch Shader
		SrRendVertex* p[3] = {&(calTri.p[0]), &(calTri.p[1]), &(calTri.p[2])};
		SrRendVertex tmp[3] = {calTri.p[0], calTri.p[1], calTri.p[2]};
		calTri.primitive->shader->ProcessPatch( p[0], p[1], p[2], &(tmp[0]), &(tmp[1]), &(tmp[2]), &(calTri.primitive->shaderConstants) );

		for(int i=0; i < 3; ++i)
		{
			SrRendVertex* p1 = p[i];

			assert( !Equal(p1->pos.w, 0) );
			p1->pos.w = 1.f / p1->pos.w;
			p1->pos.x *= p1->pos.w;
			p1->pos.y *= p1->pos.w;
			p1->pos.z *= p1->pos.w;

			p1->channel1 *= p1->pos.w;
			p1->channel2 *= p1->pos.w;
			p1->channel3 *= p1->pos.w;

			p1->pos.x = ((p1->pos.x * .5f + 0.5f) * g_context->width);
			p1->pos.y = ((-p1->pos.y * .5f + 0.5f) * g_context->height);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// 
	//  |
	// \|/
	//  y5
	// 	     0
	// 	    /|
	// 	   / |
	// 1  /  |
	// 	  \  |
	// 	   \ |
	// 	    \|
	// 		 2
	if (calTri.p[2].pos.y < calTri.p[1].pos.y)
	{
		SWAP(calTri.p[2], calTri.p[1]);
	}
	if (calTri.p[1].pos.y < calTri.p[0].pos.y)
	{
		SWAP(calTri.p[1], calTri.p[0]);
	}
	if (calTri.p[2].pos.y < calTri.p[1].pos.y)
	{
		SWAP(calTri.p[2], calTri.p[1]);
	}

	//////////////////////////////////////////////////////////////////////////

	if (Equal(calTri.p[0].pos.y, calTri.p[1].pos.y))
	{

		// 1  ____0   0___1
		// 	  \  |    |  /
		// 	   \ |    | /
		// 	    \|    |/
		// 		 2    2

		if (calTri.p[0].pos.x < calTri.p[1].pos.x)
		{
			//     0___1
			//     |  /
			//     | /
			// 	   |/
			// 	   2

		}
		else
		{
			SWAP(calTri.p[0], calTri.p[1]);
		}

		Rasterize_Top_Tri_F( calTri );

	}
	else if (Equal(calTri.p[2].pos.y, calTri.p[1].pos.y))
	{

		// 	     2   2
		// 	    /|   |\
		// 	   / |   | \
		// 0  /  |1  |0 \1
		//	 ----    ----
		SWAP(calTri.p[0], calTri.p[2]);



		if (calTri.p[0].pos.x < calTri.p[1].pos.x)
		{
			// 	     2  
			// 	    /|  
			// 	   / |  
			// 0  /  |1  
			//	 ----    
		}
		else
		{
			SWAP(calTri.p[0], calTri.p[1]);
		}

		Rasterize_Bottom_Tri_F( calTri );
	}
	else
	{
		// 	     0
		// 	    /|
		// 	   / |
		// 1  /__|3
		// 	  \  |
		// 	   \ |
		// 	    \|
		// 		 2

		SrRastTriangle topTri = calTri;
		SrRastTriangle bottomTri = calTri;

		// Create A new Point
		SrRendVertex newVertINST = calTri.p[0];

		float ratio = (calTri.p[1].pos.y - calTri.p[0].pos.y) / (calTri.p[2].pos.y - calTri.p[0].pos.y);

		if ( calTri.primitive->shader )
		{
#ifdef FIXED_FUNCTION_RASTERIZOR
			FixedRasterize( &newVertINST, &(calTri.p[0]), &(calTri.p[2]), NULL, ratio, false );
#else
			calTri.primitive->shader->ProcessRasterize( &newVertINST, &(calTri.p[0]), &(calTri.p[2]), NULL, ratio, NULL, false );
#endif
		}

		topTri.p[2] = newVertINST;
		bottomTri.p[0] = newVertINST;

		if (subtri)
		{
			return;
		}

		RasterizeTriangle(topTri, true);
		RasterizeTriangle(bottomTri, true);
	}
}

void SrRasterizer::WriteLine(const void* vertA, const void* vertB, SrRendPrimitve* primitive, uint32 count, float ratio_step, float ratio_start, uint32 address_start)
{
	__m128 pos[2];
	float4 channel1[2];
	float4 channel2[2];
	//__m256 channel12[2];

	pos[0] = ((SrRendVertexSSE*)vertA)->c0;
	pos[1] = ((SrRendVertexSSE*)vertB)->c0;

	channel1[0] = ((SrRendVertex*)vertA)->channel1;
	channel1[1] = ((SrRendVertex*)vertB)->channel1;

	channel2[0] = ((SrRendVertex*)vertA)->channel2;
	channel2[1] = ((SrRendVertex*)vertB)->channel2;

	// channel12[0] = ((SrRendVertexSSE*)vertA)->c1_2;
	// channel12[1] = ((SrRendVertexSSE*)vertB)->c1_2;
	
	uint32 address = address_start;
	float ratio = ratio_start;
	float4 outHolder;
	for (uint32 i = 0; i < count; ++i, ++address, ratio += ratio_step)
	{
		SrFragment* thisBuffer = fBuffer->fBuffer + address;
		outHolder.m128 = FastLerp128(pos[0], pos[1], ratio);

		if (outHolder.z - 1.f > fBuffer->zBuffer[address])
		{
			continue;
		}
		fBuffer->zBuffer[address] = outHolder.z - 1.f;

		thisBuffer->c0 = outHolder.m128;
		thisBuffer->primitive = primitive;

		thisBuffer->c1 = FastLerp128W(channel1[0].m128, channel1[1].m128, ratio, outHolder.w);
		thisBuffer->c2 = FastLerp128W(channel2[0].m128, channel2[1].m128, ratio, outHolder.w);
		//thisBuffer->c12 = FastLerp256W(channel12[0], channel12[1], ratio, outHolder.w);
	}
}

void SrRasterizer::Rasterize_ScanLine( uint32 line, float fstart, float fend, const void* vertA, const void* vertB, SrRendPrimitve* primitive, ERasterizeMode rMode /*= eRm_WireFrame*/, bool toptri /*= true */ )
{
	int start = (int)fstart;
	int end = (int)fend + 1;

	if (fstart > fend)
	{
		return;
	}

	if (g_context->IsFeatureEnable(eRFeature_InterlaceRendering))
	{
		if (line % 2 == gEnv->renderer->getFrameCount() % 2)
		{
			return;
		}
	}

	assert(start >= 0 && end > 0 && end - start > 0 );

	uint32 count = end - start;

	float ratio_step = 1.f / (fend - fstart + 1);
	float ratio_start = (start - fstart) * ratio_step;

	uint32 address_start = line * g_context->width + start;



	// old array of struct method
	// for ( uint32 i = 0; i < count; ++i )
	// {
	// 	uint32 address = address_start + i;
	// 	float ratio = ratio_start + ratio_step * i;
	// 	SrFragment* thisBuffer = fBuffer->fBuffer + address;
	// 	Rasterize_WritePixel(vertA, vertB, ratio, thisBuffer, primitive, address);
	// }

	// new struct of array method
	WriteLine(vertA, vertB, primitive, count, ratio_step, ratio_start, address_start);
}

void SrRasterizer::Rasterize_ScanLine_Clipped( uint32 line, float fstart, float fend, float fclipStart, float fclipEnd, const void* vertA, const void* vertB, SrRendPrimitve* primitive, ERasterizeMode rMode /*= eRm_Solid */ )
{
	int start = (int)fstart;
	int end = (int)fend + 1;

	if (fstart > fend)
	{
		return;
	}

	if (fclipStart > fclipEnd)
	{
		return;
	}

	if (g_context->IsFeatureEnable(eRFeature_InterlaceRendering))
	{
		if (line % 2 == gEnv->renderer->getFrameCount() % 2)
		{
			return;
		}
	}

	int clipStart = (int)fclipStart;
	int clipEnd = (int)fclipEnd + 1;

	assert(clipStart >= 0 && clipEnd >= 0 && end - start > 0 && clipEnd - clipStart > 0);

	uint32 clipCount = clipEnd - clipStart - 1;
	//uint32 count = end - start;

	float ratio_step = 1.f / (fend - fstart + 1);
	float ratio_start = (clipStart - fstart) * ratio_step;

	uint32 address_start = line * g_context->width + clipStart;

	// for ( uint32 i = 0; i < clipCount; ++i )
	// {
	// 	uint32 address = address_start + i;
	// 	// dotRendering
	// 	if (g_context->IsFeatureEnable(eRFeature_DotCoverageRendering))
	// 	{
	// 		bool omitThisFrame = (address - ((address / g_context->width) % 2)) % 2 == gEnv->renderer->getFrameCount() % 2;
	// 		if (omitThisFrame)
	// 		{
	// 			// TODO
	// 			// set back to black
	// 			continue;
	// 		}
	// 	}
	//
	// 	float ratio = ratio_start + ratio_step * i;
	//
	// 	// rasterize this
	// 	assert( address >=0 && address < g_context->width * g_context->height );
	// 	SrFragment* thisBuffer = fBuffer->fBuffer + address;
	// 	{
	// 		Rasterize_WritePixel(vertA, vertB, ratio, thisBuffer, primitive, address);
	//
	// 		
	// 	}
	// }

	// new struct of array method
	WriteLine(vertA, vertB, primitive, clipCount, ratio_step, ratio_start, address_start);
}

void SrRasterizer::Rasterize_Top_Tri_F( SrRastTriangle& tri )
{
	float dx_right,    
		dx_left,     // the dx/dy ratio of the left edge of line
		xs,xe,       // the starting and ending points of the edges
		height,      // the height of the triangle
		right,       // used by clipping
		left;

	int iy1,iy3,loop_y; // integers for y loops

	float x1 = tri.p[0].pos.x;
	float y1 = tri.p[0].pos.y;
	float x2 = tri.p[1].pos.x;
	//float y2 = tri.p[1].pos.y;
	float x3 = tri.p[2].pos.x;
	float y3 = tri.p[2].pos.y;

	float min_clip_y = g_context->viewport.y;
	float max_clip_y = g_context->viewport.y + g_context->viewport.h;
	float min_clip_x = g_context->viewport.x;
	float max_clip_x = g_context->viewport.x + g_context->viewport.w;

	height = y3-y1;
	float sy1 = y1;

	dx_left  = (x3-x1)/height;
	dx_right = (x3-x2)/height;

	xs = x1;
	xe = x2;

	float offsetY = 0;

	if (y1 < min_clip_y)
	{
		// compute new xs and ys
		xs = xs+dx_left*(-y1+min_clip_y);
		xe = xe+dx_right*(-y1+min_clip_y);

		// reset y1
		offsetY = ceil(y1) - y1;
		y1 = min_clip_y;


		iy1 = (int)y1;
	} // end if top is off screen
	else
	{
		// top-left
		iy1 = (int)(ceil(y1));
		// 
		offsetY = iy1 - y1;

		// bump xs and xe appropriately
		xs = xs+dx_left*(iy1-y1);
		xe = xe+dx_right*(iy1-y1);
	} // end else



	if (y3 > max_clip_y)
	{
		// clip y
		y3 = max_clip_y;

		// make sure top left fill convention is observed
		iy3 = (int)y3-1;
	} // end if
	else
	{
		// make sure top left fill convention is observed
		iy3 = (int)(ceil(y3))-1;
	} // end else

	SrRendVertex StackLVert;
	SrRendVertex StackRVert;

	void* leftVert = &StackLVert;
	void* rightVert = &StackRVert;

	// test if x clipping is needed
	if (x1>=min_clip_x && x1<=max_clip_x &&
		x2>=min_clip_x && x2<=max_clip_x &&
		x3>=min_clip_x && x3<=max_clip_x)
	{
		// draw the triangle
		for (loop_y=iy1; loop_y<=iy3; loop_y++)
		{
			// calc ratio
			float dx = (float)height / float(x2 - x1);
			float ratio = 1.f;

			float ratio_step = 1.f / (float)height;
			float ratio_start =  -(float)sy1 * ratio_step;
			if (dx > 1.0f)
			{
				ratio = ratio_step * loop_y + ratio_start;
			}
			else
			{
				if(abs(x2 - x3) > abs(x3 - x1))
				{
					ratio = 1.f - abs(xe - x3) / abs(x2 - x3);
				}
				else
				{
					ratio = 1.f - abs(x3 - xs) / abs(x3 - x1);
				}

				ratio = Clamp(ratio, 0.0f, 1.0f);
			}

			if ( tri.primitive->shader ) 
			{
#ifdef FIXED_FUNCTION_RASTERIZOR
				FixedRasterize(  leftVert, &(tri.p[0]), &(tri.p[2]), NULL, ratio, false );
				FixedRasterize(  rightVert, &(tri.p[1]), &(tri.p[2]), NULL, ratio, false );
#else
				tri.primitive->shader->ProcessRasterize( leftVert, &(tri.p[0]), &(tri.p[2]), NULL, ratio, NULL, false );
				tri.primitive->shader->ProcessRasterize( rightVert, &(tri.p[1]), &(tri.p[2]), NULL, ratio, NULL, false );
#endif
			}

			Rasterize_ScanLine(loop_y, xs, xe, leftVert, rightVert, tri.primitive, eRm_Solid );
			
			xs+=dx_left;
			xe+=dx_right;
		}

	}
	else
	{
		for (loop_y=iy1; loop_y<=iy3; loop_y++, xs+=dx_left, xe+=dx_right)
		{
			left  = xs;
			right = xe;

			if (left < min_clip_x)
			{
				left = min_clip_x;

				if (right < min_clip_x)
					continue;
			}

			if (right > max_clip_x)
			{
				right = max_clip_x;

				if (left > max_clip_x)
					continue;
			}

			// calc ratio
			float dx = (float)height / float(x2 - x1);
			float ratio = 1.f;

			float ratio_step = 1.f / (float)height;
			float ratio_start =  -(float)sy1 * ratio_step;
			if (dx > 1.0f)
			{
				ratio = ratio_step * loop_y + ratio_start;
			}
			else
			{
				if(abs(x2 - x3) > abs(x3 - x1))
				{
					ratio = 1.f - abs(xe - x3) / abs(x2 - x3);
				}
				else
				{
					ratio = 1.f - abs(x3 - xs) / abs(x3 - x1);
				}

				ratio = Clamp(ratio, 0.0f, 1.0f);
			}

			if ( tri.primitive->shader )
			{
#ifdef FIXED_FUNCTION_RASTERIZOR
				FixedRasterize( leftVert, &(tri.p[0]), &(tri.p[2]), NULL, ratio, false );
				FixedRasterize( rightVert, &(tri.p[1]), &(tri.p[2]), NULL, ratio, false );
#else
				tri.primitive->shader->ProcessRasterize( leftVert, &(tri.p[0]), &(tri.p[2]), NULL, ratio, NULL, false );
				tri.primitive->shader->ProcessRasterize( rightVert, &(tri.p[1]), &(tri.p[2]), NULL, ratio, NULL, false );
#endif

			}

			Rasterize_ScanLine_Clipped(loop_y, xs, xe, left, right, leftVert, rightVert, tri.primitive, eRm_Solid );
		} 

	} 
}

void SrRasterizer::Rasterize_Bottom_Tri_F( SrRastTriangle& tri )
{
	float dx_right,		
		dx_left,		
		xs,xe,			
		height,			
		right,			
		left;

	int iy1,iy3,loop_y;

	float x1 = tri.p[2].pos.x;
	float y1 = tri.p[2].pos.y;
	float x2 = tri.p[0].pos.x;
	//float y2 = tri.p[0].pos.y;
	float x3 = tri.p[1].pos.x;
	float y3 = tri.p[1].pos.y;

	float min_clip_y = g_context->viewport.y;
	float max_clip_y = g_context->viewport.y + g_context->viewport.h;
	float min_clip_x = g_context->viewport.x;
	float max_clip_x = g_context->viewport.x + g_context->viewport.w;

	height = y3-y1;
	float sy1 = y1;

	dx_left  = (x2-x1)/height;
	dx_right = (x3-x1)/height;

	xs = x1;
	xe = x1; 

	float offsetY = 0;

	if (y1 < min_clip_y)
	{
		xs = xs+dx_left*(-y1+min_clip_y);
		xe = xe+dx_right*(-y1+min_clip_y);

		offsetY = ceil(y1) - y1;
		y1 = min_clip_y;

		iy1 = (int)y1;
	} 
	else
	{
		iy1 = (int)(ceil(y1));
		offsetY = iy1 - y1;

		xs = xs+dx_left*(iy1-y1);
		xe = xe+dx_right*(iy1-y1);
	} 

	if (y3 > max_clip_y)
	{
		y3 = max_clip_y;
		iy3 = (int)y3-1;
	} 
	else
	{
		iy3 = (int)(ceil(y3))-1;
	}

	SrRendVertex StackLVert;
	SrRendVertex StackRVert;

	void* leftVert = &StackLVert;
	void* rightVert = &StackRVert;


	if (x1 >= min_clip_x && x1 <= max_clip_x &&
		x2 >= min_clip_x && x2 <= max_clip_x &&
		x3 >= min_clip_x && x3 <= max_clip_x)
	{

		for (loop_y = iy1; loop_y <= iy3; loop_y++)
		{
			// calc ratio
			float dx = (float)height / float(x3 - x2);
			float ratio = 1.f;

			float ratio_step = 1.f / (float)height;
			float ratio_start =  -(float)sy1 * ratio_step;
			if (dx > 1.0f)
			{
				ratio = ratio_step * loop_y + ratio_start;
			}
			else
			{
				if(abs(x3 - x1) > abs(x2 - x1))
				{
					ratio = abs(xe - x1) / abs(x3 - x1);
				}
				else
				{
					ratio = abs(x1 - xs) / abs(x1 - x2);
				}

				ratio = Clamp(ratio, 0.0f, 1.0f);
			}

			if ( tri.primitive->shader )
			{
#ifdef FIXED_FUNCTION_RASTERIZOR
				FixedRasterize( leftVert, &(tri.p[2]), &(tri.p[0]), NULL, ratio, false );
				FixedRasterize( rightVert, &(tri.p[2]), &(tri.p[1]), NULL, ratio, false );
#else
				tri.primitive->shader->ProcessRasterize( leftVert, &(tri.p[2]), &(tri.p[0]), NULL, ratio, NULL, false );
				tri.primitive->shader->ProcessRasterize( rightVert, &(tri.p[2]), &(tri.p[1]), NULL, ratio, NULL, false );
#endif
			}

			// draw the line
			Rasterize_ScanLine(loop_y, xs, xe, leftVert, rightVert, tri.primitive, eRm_Solid );

			xs+=dx_left;
			xe+=dx_right;
		} 

	} 
	else
	{


		for (loop_y = iy1; loop_y <= iy3; loop_y++, xs+=dx_left, xe+=dx_right)
		{
			left  = xs;
			right = xe;

			if (left < min_clip_x)
			{
				left = min_clip_x;

				if (right < min_clip_x)
					continue;
			}

			if (right > max_clip_x)
			{
				right = max_clip_x;

				if (left > max_clip_x)
					continue;
			}

			// calc ratio
			float dx = (float)height / float(x3 - x2);
			float ratio = 1.f;

			float ratio_step = 1.f / (float)height;
			float ratio_start =  -(float)sy1 * ratio_step;
			if (dx > 1.0f)
			{
				ratio = ratio_step * loop_y + ratio_start;
			}
			else
			{
				if(abs(x3 - x1) > abs(x2 - x1))
				{
					ratio = abs(xe - x1) / abs(x3 - x1);
				}
				else
				{
					ratio = abs(x1 - xs) / abs(x1 - x2);
				}

				ratio = Clamp(ratio, 0.0f, 1.0f);
			}

			if ( tri.primitive->shader )
			{
#ifdef FIXED_FUNCTION_RASTERIZOR
				FixedRasterize( leftVert, &(tri.p[2]), &(tri.p[0]), NULL, ratio, false );
				FixedRasterize( rightVert, &(tri.p[2]), &(tri.p[1]), NULL, ratio, false );
#else
				tri.primitive->shader->ProcessRasterize( leftVert, &(tri.p[2]), &(tri.p[0]), NULL, ratio, NULL, false );
				tri.primitive->shader->ProcessRasterize( rightVert, &(tri.p[2]), &(tri.p[1]), NULL, ratio, NULL, false );
#endif
			}

			Rasterize_ScanLine_Clipped(loop_y, xs, xe, left, right, leftVert, rightVert,  tri.primitive, eRm_Solid );
		}

	}
}

void SrRasterizer::Rasterize_WritePixel( const void* vertA, const void* vertB, float ratio, SrFragment* thisBuffer, SrRendPrimitve* primitive, uint32 address )
{
	SrRendVertex* posA = (SrRendVertex*)vertA;
	SrRendVertex* posB = (SrRendVertex*)vertB;


	// lerp z
	float z = posA->pos.z * (1.f - ratio) + posB->pos.z * ratio;
	z = (z - 1.f);

	// cs
	//assert(address < g_context->width * g_context->height && address >=0 );

#ifdef RASTERIZER_SYNC
	//LPCRITICAL_SECTION csptr = fBuffer->GetSyncMark(address);
	//EnterCriticalSection( csptr );

	if ( z > fBuffer->zBuffer[address] )
	{
		//LeaveCriticalSection(csptr); 
		return;
	}
	fBuffer->zBuffer[address] = z;
	//LeaveCriticalSection(csptr); 
#else
	if ( z > fBuffer->zBuffer[address] )
	{
		return;
	}
	fBuffer->zBuffer[address] = z;
#endif


	// here, rasterize
	assert( primitive && primitive->shader);

#ifdef FIXED_FUNCTION_RASTERIZOR
	FixedRasterize( thisBuffer, vertA, vertB, NULL, ratio, &(primitive->shaderConstants), true  );
#else
	primitive->shader->ProcessRasterize( thisBuffer, vertA, vertB, NULL, ratio, &(primitive->shaderConstants), true );
#endif
	
	thisBuffer->primitive = primitive;
}

