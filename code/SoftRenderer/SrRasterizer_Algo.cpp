/**
  @file SrRasterizer_Algo.cpp
  
  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#include "stdafx.h"
#include "SrRasterizer.h"
#include "SrProfiler.h"
#include "SrMesh.h"
#include "SrMaterial.h"
#include "SrSwShader.h"
#include "SrFragmentBuffer.h"

#include "mmgr.h"

void SrRasterizer::RasterizeTriangle( SrRastTriangle& calTri, bool subtri )
{
	// 如果是提交三角形，先进行透视变换
	if(!subtri)
	{
		// ccw,cw 背向面裁剪
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

		// 此时的三角形还在裁剪空间，需要投影到屏幕空间！
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
// 			p1->channel4 *= p1->pos.w;
// 			p1->channel5 *= p1->pos.w;

			// 变换到屏幕空间
			p1->pos.x = ((p1->pos.x * .5f + 0.5f) * g_context->width);
			p1->pos.y = ((-p1->pos.y * .5f + 0.5f) * g_context->height);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// 排序三角形，变为
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
	// 排序完成
	//////////////////////////////////////////////////////////////////////////

	if (Equal(calTri.p[0].pos.y, calTri.p[1].pos.y))
	{

		// 平顶三角

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
		// 平底三角

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
		// 需要分割
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
			calTri.primitive->shader->ProcessRasterize( &newVertINST, &(calTri.p[0]), &(calTri.p[2]), NULL, ratio, false );
#endif
		}

		topTri.p[2] = newVertINST;
		bottomTri.p[0] = newVertINST;

		//assert(!subtri);

		if (subtri)
		{
			return;
		}

		RasterizeTriangle(topTri, true);
		RasterizeTriangle(bottomTri, true);
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
	
	for ( uint32 i = 0; i < count; ++i )
	{
		uint32 address = address_start + i;
		// 如果开启了dotRendering
		if (g_context->IsFeatureEnable(eRFeature_DotCoverageRendering))
		{
			bool omitThisFrame = (address - ((address / g_context->width) % 2)) % 2 == gEnv->renderer->getFrameCount() % 2;
			if (omitThisFrame)
			{
				// TODO
				// set back to black
				continue;
			}
		}
		float ratio = ratio_start + ratio_step * i;

		// rasterize this
		assert( address >=0 && address < g_context->width * g_context->height );
		SrFragment* thisBuffer = fBuffer->fBuffer + address;
		{
			// ztest 可以再光栅化之前作，这时可以预判z值，提前kill掉，加速
			Rasterize_WritePixel(vertA, vertB, ratio, thisBuffer, primitive, address);
		}
	}
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
	uint32 count = end - start;

	float ratio_step = 1.f / (fend - fstart + 1);
	float ratio_start = (clipStart - fstart) * ratio_step;

	uint32 address_start = line * g_context->width + clipStart;

	for ( uint32 i = 0; i < clipCount; ++i )
	{
		uint32 address = address_start + i;
		// 如果开启了dotRendering
		if (g_context->IsFeatureEnable(eRFeature_DotCoverageRendering))
		{
			bool omitThisFrame = (address - ((address / g_context->width) % 2)) % 2 == gEnv->renderer->getFrameCount() % 2;
			if (omitThisFrame)
			{
				// TODO
				// set back to black
				continue;
			}
		}

		float ratio = ratio_start + ratio_step * i;

		// rasterize this
		assert( address >=0 && address < g_context->width * g_context->height );
		SrFragment* thisBuffer = fBuffer->fBuffer + address;
		{
			// ztest 可以再光栅化之前作，这时可以预判z值，提前kill掉，加速
			Rasterize_WritePixel(vertA, vertB, ratio, thisBuffer, primitive, address);
		}
	}
}

void SrRasterizer::Rasterize_Top_Tri_F( SrRastTriangle& tri )
{
	// 浮点数 光栅化函数

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
	float y2 = tri.p[1].pos.y;
	float x3 = tri.p[2].pos.x;
	float y3 = tri.p[2].pos.y;

	float min_clip_y = g_context->viewport.y;
	float max_clip_y = g_context->viewport.y + g_context->viewport.h;
	float min_clip_x = g_context->viewport.x;
	float max_clip_x = g_context->viewport.x + g_context->viewport.w;

	// 计算扫描行数
	height = y3-y1;
	float sy1 = y1;

	dx_left  = (x3-x1)/height;
	dx_right = (x3-x2)/height;

	// 设置起始扫描点
	xs = x1;
	xe = x2;

	// 记录每根扫描线计算ratio时的偏移值
	float offsetY = 0;

	// y轴裁剪
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
		// 确保 top-left 填充法则
		iy1 = (int)(ceil(y1));
		// 计算偏移值
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
			// 取得此扫描线首尾的 插值后 顶点
			// 如果斜率大于1，则使用横向坐标来计算ratio
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
				// 此 ratio全为 绝对浮点值，不需要作offset处理
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
				tri.primitive->shader->ProcessRasterize( leftVert, &(tri.p[0]), &(tri.p[2]), NULL, ratio, false );
				tri.primitive->shader->ProcessRasterize( rightVert, &(tri.p[1]), &(tri.p[2]), NULL, ratio, false );
#endif
			}

			// 扫描线
			Rasterize_ScanLine(loop_y, xs, xe, leftVert, rightVert, tri.primitive, eRm_Solid );
			
			// 调整始末点到下一根
			xs+=dx_left;
			xe+=dx_right;
		}

	}
	else
	{
		// X轴需要裁剪
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
			// 取得此扫描线首尾的 插值后 顶点
			// 如果斜率大于1，则使用横向坐标来计算ratio
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
				tri.primitive->shader->ProcessRasterize( leftVert, &(tri.p[0]), &(tri.p[2]), NULL, ratio, false );
				tri.primitive->shader->ProcessRasterize( rightVert, &(tri.p[1]), &(tri.p[2]), NULL, ratio, false );
#endif

			}

			// 扫描线
			Rasterize_ScanLine_Clipped(loop_y, xs, xe, left, right, leftVert, rightVert, tri.primitive, eRm_Solid );
		} 

	} 
}

void SrRasterizer::Rasterize_Bottom_Tri_F( SrRastTriangle& tri )
{
	// 浮点数 光栅化函数

	float dx_right,		// 右边的斜率
		dx_left,		// 左边的斜率
		xs,xe,			// 开始，和结束的坐标
		height,			// 存储高度
		right,			// 存储裁剪的左右坐标
		left;

	int iy1,iy3,loop_y; // 扫描线行序

	float x1 = tri.p[2].pos.x;
	float y1 = tri.p[2].pos.y;
	float x2 = tri.p[0].pos.x;
	float y2 = tri.p[0].pos.y;
	float x3 = tri.p[1].pos.x;
	float y3 = tri.p[1].pos.y;

	float min_clip_y = g_context->viewport.y;
	float max_clip_y = g_context->viewport.y + g_context->viewport.h;
	float min_clip_x = g_context->viewport.x;
	float max_clip_x = g_context->viewport.x + g_context->viewport.w;

	// 计算高度，存储到sy1
	height = y3-y1;
	float sy1 = y1;

	// 计算斜率
	dx_left  = (x2-x1)/height;
	dx_right = (x3-x1)/height;

	// 设置起始点
	xs = x1;
	xe = x1; 

	// 记录每根扫描线计算ratio时的偏移值
	float offsetY = 0;

	// y 裁剪
	if (y1 < min_clip_y)
	{
		// 重新计算起始点
		xs = xs+dx_left*(-y1+min_clip_y);
		xe = xe+dx_right*(-y1+min_clip_y);

		// 重置
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
			// 取得此扫描线首尾的 插值后 顶点

			// 如果斜率大于1，则使用横向坐标来计算ratio
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
				tri.primitive->shader->ProcessRasterize( leftVert, &(tri.p[2]), &(tri.p[0]), NULL, ratio, false );
				tri.primitive->shader->ProcessRasterize( rightVert, &(tri.p[2]), &(tri.p[1]), NULL, ratio, false );
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
			// 取得此扫描线首尾的 插值后 顶点

			// 如果斜率大于1，则使用横向坐标来计算ratio
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
				tri.primitive->shader->ProcessRasterize( leftVert, &(tri.p[2]), &(tri.p[0]), NULL, ratio, false );
				tri.primitive->shader->ProcessRasterize( rightVert, &(tri.p[2]), &(tri.p[1]), NULL, ratio, false );
#endif
			}

			Rasterize_ScanLine_Clipped(loop_y, xs, xe, left, right, leftVert, rightVert,  tri.primitive, eRm_Solid );
		}

	}
}

void SrRasterizer::Rasterize_WritePixel( const void* vertA, const void* vertB, float ratio, SrFragment* thisBuffer, SrRendPrimitve* primitive, uint32 address )
{
	float4* posA = (float4*)vertA;
	float4* posB = (float4*)vertB;

	if ( !primitive->shaderConstants.alphaTest )
	{


		// lerp z 作预判
		float z = posA->z * (1.f - ratio) + posB->z * ratio;
		z = (z - 1.f);

		// cs
		assert(address < g_context->width * g_context->height && address >=0 );

#ifdef RASTERIZER_SYNC
		LPCRITICAL_SECTION csptr = fBuffer->GetSyncMark(address);
		EnterCriticalSection( csptr );

		if ( z > fBuffer->zBuffer[address] )
		{
			LeaveCriticalSection(csptr); 
			return;
		}
		fBuffer->zBuffer[address] = z;
		LeaveCriticalSection(csptr); 
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
	else
	{
		SrFragment tmpBuffer = *thisBuffer;

#ifdef FIXED_FUNCTION_RASTERIZOR
		FixedRasterize(  &tmpBuffer, vertA, vertB, NULL, ratio, &(primitive->shaderConstants), true );
#else
		primitive->shader->ProcessRasterize( &tmpBuffer, vertA, vertB, NULL, ratio, &(primitive->shaderConstants), true );
#endif
		
		tmpBuffer.primitive = primitive;
		
// 		uint32 color = primitive->shaderConstants.Tex2D( float2(tmpBuffer.worldpos_tx.w, tmpBuffer.normal_ty.w), 0 );
// 		if ( ( color & 0xFF000000 ) >> 24 < 1 )
// 		{
// 			return;
// 		}

		// lerp z 作预判
		float z = posA->z * (1.f - ratio) + posB->z * ratio;
		z = (z - 1.f);

		// cs
		assert(address < g_context->width * g_context->height && address >=0 );

#ifdef RASTERIZER_SYNC
		LPCRITICAL_SECTION csptr = fBuffer->GetSyncMark(address);
		EnterCriticalSection( csptr );

		if ( z > fBuffer->zBuffer[address] )
		{
			LeaveCriticalSection(csptr); 
			return;
		}
		fBuffer->zBuffer[address] = z;
		LeaveCriticalSection(csptr); 
#else
		if ( z > fBuffer->zBuffer[address] )
		{
			return;
		}
		fBuffer->zBuffer[address] = z;
#endif
		*thisBuffer = tmpBuffer;
	}

}

