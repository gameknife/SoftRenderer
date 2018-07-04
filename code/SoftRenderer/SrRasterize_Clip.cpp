#include "stdafx.h"
#include "SrRasterizer.h"
#include "SrSwShader.h"
#include "SrMaterial.h"


#include "mmgr.h"


/**
*@brief 光珊化三角形裁剪
*@return void 
*@param SrRastTriangle tri 世界空间的三角形 
*@param float zNear 相机近裁剪面
*@param float zFar 相机远裁剪面
*/
void SrRasterizer::RasterizeTriangle_Clip( SrRastTriangle& tri, float zNear, float zFar )
{

	// 裁剪的区域判断码
#define CLIP_CODE_MORE_Z   0x0001    // z > z_max
#define CLIP_CODE_LESS_Z   0x0002    // z < z_min
#define CLIP_CODE_IN_Z   0x0004    // z_min < z < z_max

#define CLIP_CODE_MORE_X   0x0001    // x > x_max
#define CLIP_CODE_LESS_X   0x0002    // x < x_min
#define CLIP_CODE_IN_X   0x0004    // x_min < x < x_max

#define CLIP_CODE_MORE_Y   0x0001    // y > y_max
#define CLIP_CODE_LESS_Y   0x0002    // y < y_min
#define CLIP_CODE_IN_Y   0x0004    // y_min < y < y_max

#define CLIP_CODE_NULL 0x0000

	int vertex_ccodes[3]; // 三角形三个顶点的判断码存储
	int num_verts_in;     // 在视锥内的顶点个数
	int v0, v1, v2;       // 顶点点序



	float z_factor = 1.f;  
	float z_test = z_factor * tri.p[0].pos.w;

	//////////////////////////////////////////////////////////////////////////
	// 裁剪X轴
	// vertex 0
	if (tri.p[0].pos.x > z_test)
		vertex_ccodes[0] = CLIP_CODE_MORE_X;
	else if (tri.p[0].pos.x < -z_test)
		vertex_ccodes[0] = CLIP_CODE_LESS_X;
	else
		vertex_ccodes[0] = CLIP_CODE_IN_X;

	z_test = z_factor * tri.p[1].pos.w;

	// vertex 1
	if (tri.p[1].pos.x > z_test)
		vertex_ccodes[1] = CLIP_CODE_MORE_X;
	else if (tri.p[1].pos.x < -z_test)
		vertex_ccodes[1] = CLIP_CODE_LESS_X;
	else
		vertex_ccodes[1] = CLIP_CODE_IN_X;

	z_test = z_factor * tri.p[2].pos.w;
	// vertex 2  
	if (tri.p[2].pos.x > z_test)
		vertex_ccodes[2] = CLIP_CODE_MORE_X;
	else if (tri.p[2].pos.x < -z_test)
		vertex_ccodes[2] = CLIP_CODE_LESS_X;
	else
		vertex_ccodes[2] = CLIP_CODE_IN_X;


	// 测试：当三角形完全在X轴向上位于视锥外部是，抛弃。
	// 否则不裁剪直接进入光栅化
	if ( ((vertex_ccodes[0] == CLIP_CODE_MORE_X) && 
		(vertex_ccodes[1] == CLIP_CODE_MORE_X) && 
		(vertex_ccodes[2] == CLIP_CODE_MORE_X) ) ||

		((vertex_ccodes[0] == CLIP_CODE_LESS_X) && 
		(vertex_ccodes[1] == CLIP_CODE_LESS_X) && 
		(vertex_ccodes[2] == CLIP_CODE_LESS_X) ) )

	{
		return;
	}
	// 裁剪X轴
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// 裁剪Y轴
	z_factor = 1.f;  
	z_test = z_factor * tri.p[0].pos.w;
	
	// vertex 0
	if (tri.p[0].pos.y > z_test)
		vertex_ccodes[0] = CLIP_CODE_MORE_Y;
	else if (tri.p[0].pos.y < -z_test)
		vertex_ccodes[0] = CLIP_CODE_LESS_Y;
	else
		vertex_ccodes[0] = CLIP_CODE_IN_Y;

	z_test = z_factor * tri.p[1].pos.w;
	// vertex 1
	if (tri.p[1].pos.y > z_test)
		vertex_ccodes[1] = CLIP_CODE_MORE_Y;
	else if (tri.p[1].pos.y < -z_test)
		vertex_ccodes[1] = CLIP_CODE_LESS_Y;
	else
		vertex_ccodes[1] = CLIP_CODE_IN_Y;

	// vertex 2  
	z_test = z_factor * tri.p[2].pos.w;
	if (tri.p[2].pos.y > z_test)
		vertex_ccodes[2] = CLIP_CODE_MORE_Y;
	else if (tri.p[2].pos.y < -z_test)
		vertex_ccodes[2] = CLIP_CODE_LESS_Y;
	else
		vertex_ccodes[2] = CLIP_CODE_IN_Y;

	// 测试：当三角形完全在X轴向上位于视锥外部是，抛弃.
	// 否则不裁剪直接进入光栅化
	if ( ((vertex_ccodes[0] == CLIP_CODE_MORE_Y) && 
		(vertex_ccodes[1] == CLIP_CODE_MORE_Y) && 
		(vertex_ccodes[2] == CLIP_CODE_MORE_Y) ) ||

		((vertex_ccodes[0] == CLIP_CODE_LESS_Y) && 
		(vertex_ccodes[1] == CLIP_CODE_LESS_Y) && 
		(vertex_ccodes[2] == CLIP_CODE_LESS_Y) ) )

	{
		return;
	}
	// 裁剪Y轴
	//////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////
	// 裁剪Z轴
	{
		// 首先判断三个顶点与ZNEAR平面的状态

		// 在视锥体内部的顶点数量置0
		num_verts_in = 0;

		// vertex 0
		if (tri.p[0].pos.w > zFar)
		{
			vertex_ccodes[0] = CLIP_CODE_MORE_Z;
		} 
		else if (tri.p[0].pos.w < zNear)
		{
			vertex_ccodes[0] = CLIP_CODE_LESS_Z;
		}
		else
		{
			vertex_ccodes[0] = CLIP_CODE_IN_Z;
			num_verts_in++;
		} 

		// vertex 1
		if (tri.p[1].pos.w > zFar)
		{
			vertex_ccodes[1] = CLIP_CODE_MORE_Z;
		} 
		else if (tri.p[1].pos.w < zNear)
		{
			vertex_ccodes[1] = CLIP_CODE_LESS_Z;
		}
		else
		{
			vertex_ccodes[1] = CLIP_CODE_IN_Z;
			num_verts_in++;
		}     

		// vertex 2
		if (tri.p[2].pos.w > zFar)
		{
			vertex_ccodes[2] = CLIP_CODE_MORE_Z;
		} 
		else if (tri.p[2].pos.w < zNear)
		{
			vertex_ccodes[2] = CLIP_CODE_LESS_Z;
		}
		else
		{
			vertex_ccodes[2] = CLIP_CODE_IN_Z;
			num_verts_in++;
		} 

		// 测试：当三角形完全在Z轴向上位于视锥外部时，抛弃。
		if ( ((vertex_ccodes[0] == CLIP_CODE_MORE_Z) && 
			(vertex_ccodes[1] == CLIP_CODE_MORE_Z) && 
			(vertex_ccodes[2] == CLIP_CODE_MORE_Z) ) ||

			((vertex_ccodes[0] == CLIP_CODE_LESS_Z) && 
			(vertex_ccodes[1] == CLIP_CODE_LESS_Z) && 
			(vertex_ccodes[2] == CLIP_CODE_LESS_Z) ) )

		{
			// 所用顶点均在不位于视锥内部，直接剔除
			return;
		}

		//////////////////////////////////////////////////////////////////////////
		// 当所有顶点，均位于Z范围内，直接提交
		if ( (vertex_ccodes[0] == CLIP_CODE_IN_Z) && 
			(vertex_ccodes[1] == CLIP_CODE_IN_Z) && 
			(vertex_ccodes[2] == CLIP_CODE_IN_Z) )
		{
			RasterizeTriangle(tri);
		}

		// 这时，判断，是否有点位于近平面以内？
		if ( ( (vertex_ccodes[0] | vertex_ccodes[1] | vertex_ccodes[2]) & CLIP_CODE_LESS_Z) )
		{
			// 有
			// 这时，这个三角形一定与近平面交错





			if (num_verts_in == 1)
			{
				// 情况A:
				// 当一个顶点在视锥内
				// 最简单，直接截取
				//     /\
				//    /  \
				//   /____\
				//  /      \

				// 1.先简单排序，把在视锥里面的顶点排到第一个
				if ( vertex_ccodes[0] == CLIP_CODE_IN_Z)
				{ v0 = 0; v1 = 1; v2 = 2; }
				else if (vertex_ccodes[1] == CLIP_CODE_IN_Z)
				{ v0 = 1; v1 = 2; v2 = 0; }
				else
				{ v0 = 2; v1 = 0; v2 = 1; }

				// 2. 裁剪，
				// 构造三个新顶点
				SrRendVertex p0 = tri.p[v0];
				SrRendVertex p1 = tri.p[v1];
				SrRendVertex p2 = tri.p[v2];

				// FIXME DIVIDE 0
				assert( abs(p0.pos.w - p1.pos.w) > SR_EQUAL_PRECISION );
				assert( abs(p0.pos.w - p2.pos.w) > SR_EQUAL_PRECISION );

				float ratio1 = (p0.pos.w - zNear) / (p0.pos.w - p1.pos.w);
				float ratio2 = (p0.pos.w - zNear) / (p0.pos.w - p2.pos.w);

				// 这里，由于三角形还在3D空间，因此直接插值，不需要透视矫正
#ifdef FIXED_FUNCTION_RASTERIZOR
				FixedRasterize(&p1, &tri.p[v0], &tri.p[v1], NULL, ratio1, &(tri.primitive->shaderConstants));
				FixedRasterize(&p2, &tri.p[v0], &tri.p[v2], NULL, ratio2, &(tri.primitive->shaderConstants));
#else
				tri.primitive->shader->ProcessRasterize(&p1, &tri.p[v0], &tri.p[v1], NULL, ratio1, &(tri.primitive->shaderConstants));
				tri.primitive->shader->ProcessRasterize(&p2, &tri.p[v0], &tri.p[v2], NULL, ratio2, &(tri.primitive->shaderConstants));
#endif



				tri.p[v1] = p1;
				tri.p[v2] = p2;

				// 裁剪完毕，提交
				RasterizeTriangle(tri);					

			}
			else if (num_verts_in == 2)
			{ 
				// num_verts = 2
				
				// 情况B：
				// 有两个顶点在视锥内
				// 稍微复杂，裁剪完后是一个四边形，需要分割

				//1          2
				//3__________4
				// \        /
				//  \/_____/
				//   \    /


				//     p0

				// 1. 找出不在视锥体内的顶点
				if ( vertex_ccodes[0] == CLIP_CODE_LESS_Z)
				{ v0 = 0; v1 = 1; v2 = 2; }
				else if (vertex_ccodes[1] == CLIP_CODE_LESS_Z)
				{ v0 = 1; v1 = 2; v2 = 0; }
				else
				{ v0 = 2; v1 = 0; v2 = 1; }

				// step 2: 裁剪
				SrRendVertex p0 = tri.p[v0];
				SrRendVertex p1 = tri.p[v1];
				SrRendVertex p2 = tri.p[v2];

				SrRendVertex p3 = p1;
				SrRendVertex p4 = p2;

				// FIXME DIVIDE 0
				assert( abs(p0.pos.w - p1.pos.w) > SR_EQUAL_PRECISION );
				assert( abs(p0.pos.w - p2.pos.w) > SR_EQUAL_PRECISION );

				float ratio1 = (zNear - p0.pos.w) / (p1.pos.w - p0.pos.w);
				float ratio2 = (zNear - p0.pos.w) / (p2.pos.w - p0.pos.w);

#ifdef FIXED_FUNCTION_RASTERIZOR
				FixedRasterize(&p3, &tri.p[v0], &tri.p[v1], NULL, ratio1, &(tri.primitive->shaderConstants));
				FixedRasterize(&p4, &tri.p[v0], &tri.p[v2], NULL, ratio2, &(tri.primitive->shaderConstants));
#else
				tri.primitive->shader->ProcessRasterize(&p3, &tri.p[v0], &tri.p[v1], NULL, ratio1, &(tri.primitive->shaderConstants));
				tri.primitive->shader->ProcessRasterize(&p4, &tri.p[v0], &tri.p[v2], NULL, ratio2, &(tri.primitive->shaderConstants));
#endif

				// step 3: 构造
				SrRastTriangle tri1 = tri;
				SrRastTriangle tri2 = tri;
				tri1.p[v0] = p3;
				// not like this...

				tri2.p[v0] = p4;
				tri2.p[v1] = p3;

				// 提交分割后的三角形

				RasterizeTriangle(tri1);
				RasterizeTriangle(tri2);


			} 

		}

	}

}



///////////////////////////////////////////////////////////

int SrRasterizer::Draw_Clip_Line(int x0,int y0, int x1, int y1, int color, 
	uint32 *dest_buffer, int lpitch)
{
	// this function draws a wireframe triangle

	int cxs, cys,
		cxe, cye;

	// clip and draw each line
	cxs = x0;
	cys = y0;
	cxe = x1;
	cye = y1;

	// clip the line
	if (Clip_Line(cxs,cys,cxe,cye))
		Draw_Line(cxs, cys, cxe,cye,color,dest_buffer,lpitch);

	// return success
	return(1);

} // end Draw_Clip_Line

///////////////////////////////////////////////////////////

int SrRasterizer::Clip_Line(int &x1,int &y1,int &x2, int &y2)
{
	// this function clips the sent line using the globally defined clipping
	// region

	// internal clipping codes
#define CLIP_CODE_C  0x0000
#define CLIP_CODE_N  0x0008
#define CLIP_CODE_S  0x0004
#define CLIP_CODE_E  0x0002
#define CLIP_CODE_W  0x0001

#define CLIP_CODE_NE 0x000a
#define CLIP_CODE_SE 0x0006
#define CLIP_CODE_NW 0x0009 
#define CLIP_CODE_SW 0x0005

	int xc1=x1, 
		yc1=y1, 
		xc2=x2, 
		yc2=y2;

	int p1_code=0, 
		p2_code=0;

	float min_clip_y = g_context->viewport.y;
	float max_clip_y = g_context->viewport.y + g_context->viewport.h - 1;
	float min_clip_x = g_context->viewport.x;
	float max_clip_x = g_context->viewport.x + g_context->viewport.w;
	
	// determine codes for p1 and p2
	if (y1 < min_clip_y)
		p1_code|=CLIP_CODE_N;
	else
		if (y1 > max_clip_y)
			p1_code|=CLIP_CODE_S;

	if (x1 < min_clip_x)
		p1_code|=CLIP_CODE_W;
	else
		if (x1 > max_clip_x)
			p1_code|=CLIP_CODE_E;

	if (y2 < min_clip_y)
		p2_code|=CLIP_CODE_N;
	else
		if (y2 > max_clip_y)
			p2_code|=CLIP_CODE_S;

	if (x2 < min_clip_x)
		p2_code|=CLIP_CODE_W;
	else
		if (x2 > max_clip_x)
			p2_code|=CLIP_CODE_E;

	// try and trivially reject
	if ((p1_code & p2_code)) 
		return(0);

	// test for totally visible, if so leave points untouched
	if (p1_code==0 && p2_code==0)
		return(1);

	// determine end clip point for p1
	switch(p1_code)
	{
	case CLIP_CODE_C: break;

	case CLIP_CODE_N:
		{
			yc1 = min_clip_y;
			xc1 = x1 + 0.5+(min_clip_y-y1)*(x2-x1)/(y2-y1);
		} break;
	case CLIP_CODE_S:
		{
			yc1 = max_clip_y;
			xc1 = x1 + 0.5+(max_clip_y-y1)*(x2-x1)/(y2-y1);
		} break;

	case CLIP_CODE_W:
		{
			xc1 = min_clip_x;
			yc1 = y1 + 0.5+(min_clip_x-x1)*(y2-y1)/(x2-x1);
		} break;

	case CLIP_CODE_E:
		{
			xc1 = max_clip_x;
			yc1 = y1 + 0.5+(max_clip_x-x1)*(y2-y1)/(x2-x1);
		} break;

		// these cases are more complex, must compute 2 intersections
	case CLIP_CODE_NE:
		{
			// north hline intersection
			yc1 = min_clip_y;
			xc1 = x1 + 0.5+(min_clip_y-y1)*(x2-x1)/(y2-y1);

			// test if intersection is valid, of so then done, else compute next
			if (xc1 < min_clip_x || xc1 > max_clip_x)
			{
				// east vline intersection
				xc1 = max_clip_x;
				yc1 = y1 + 0.5+(max_clip_x-x1)*(y2-y1)/(x2-x1);
			} // end if

		} break;

	case CLIP_CODE_SE:
		{
			// south hline intersection
			yc1 = max_clip_y;
			xc1 = x1 + 0.5+(max_clip_y-y1)*(x2-x1)/(y2-y1);	

			// test if intersection is valid, of so then done, else compute next
			if (xc1 < min_clip_x || xc1 > max_clip_x)
			{
				// east vline intersection
				xc1 = max_clip_x;
				yc1 = y1 + 0.5+(max_clip_x-x1)*(y2-y1)/(x2-x1);
			} // end if

		} break;

	case CLIP_CODE_NW: 
		{
			// north hline intersection
			yc1 = min_clip_y;
			xc1 = x1 + 0.5+(min_clip_y-y1)*(x2-x1)/(y2-y1);

			// test if intersection is valid, of so then done, else compute next
			if (xc1 < min_clip_x || xc1 > max_clip_x)
			{
				xc1 = min_clip_x;
				yc1 = y1 + 0.5+(min_clip_x-x1)*(y2-y1)/(x2-x1);	
			} // end if

		} break;

	case CLIP_CODE_SW:
		{
			// south hline intersection
			yc1 = max_clip_y;
			xc1 = x1 + 0.5+(max_clip_y-y1)*(x2-x1)/(y2-y1);	

			// test if intersection is valid, of so then done, else compute next
			if (xc1 < min_clip_x || xc1 > max_clip_x)
			{
				xc1 = min_clip_x;
				yc1 = y1 + 0.5+(min_clip_x-x1)*(y2-y1)/(x2-x1);	
			} // end if

		} break;

	default:break;

	} // end switch

	// determine clip point for p2
	switch(p2_code)
	{
	case CLIP_CODE_C: break;

	case CLIP_CODE_N:
		{
			yc2 = min_clip_y;
			xc2 = x2 + (min_clip_y-y2)*(x1-x2)/(y1-y2);
		} break;

	case CLIP_CODE_S:
		{
			yc2 = max_clip_y;
			xc2 = x2 + (max_clip_y-y2)*(x1-x2)/(y1-y2);
		} break;

	case CLIP_CODE_W:
		{
			xc2 = min_clip_x;
			yc2 = y2 + (min_clip_x-x2)*(y1-y2)/(x1-x2);
		} break;

	case CLIP_CODE_E:
		{
			xc2 = max_clip_x;
			yc2 = y2 + (max_clip_x-x2)*(y1-y2)/(x1-x2);
		} break;

		// these cases are more complex, must compute 2 intersections
	case CLIP_CODE_NE:
		{
			// north hline intersection
			yc2 = min_clip_y;
			xc2 = x2 + 0.5+(min_clip_y-y2)*(x1-x2)/(y1-y2);

			// test if intersection is valid, of so then done, else compute next
			if (xc2 < min_clip_x || xc2 > max_clip_x)
			{
				// east vline intersection
				xc2 = max_clip_x;
				yc2 = y2 + 0.5+(max_clip_x-x2)*(y1-y2)/(x1-x2);
			} // end if

		} break;

	case CLIP_CODE_SE:
		{
			// south hline intersection
			yc2 = max_clip_y;
			xc2 = x2 + 0.5+(max_clip_y-y2)*(x1-x2)/(y1-y2);	

			// test if intersection is valid, of so then done, else compute next
			if (xc2 < min_clip_x || xc2 > max_clip_x)
			{
				// east vline intersection
				xc2 = max_clip_x;
				yc2 = y2 + 0.5+(max_clip_x-x2)*(y1-y2)/(x1-x2);
			} // end if

		} break;

	case CLIP_CODE_NW: 
		{
			// north hline intersection
			yc2 = min_clip_y;
			xc2 = x2 + 0.5+(min_clip_y-y2)*(x1-x2)/(y1-y2);

			// test if intersection is valid, of so then done, else compute next
			if (xc2 < min_clip_x || xc2 > max_clip_x)
			{
				xc2 = min_clip_x;
				yc2 = y2 + 0.5+(min_clip_x-x2)*(y1-y2)/(x1-x2);	
			} // end if

		} break;

	case CLIP_CODE_SW:
		{
			// south hline intersection
			yc2 = max_clip_y;
			xc2 = x2 + 0.5+(max_clip_y-y2)*(x1-x2)/(y1-y2);	

			// test if intersection is valid, of so then done, else compute next
			if (xc2 < min_clip_x || xc2 > max_clip_x)
			{
				xc2 = min_clip_x;
				yc2 = y2 + 0.5+(min_clip_x-x2)*(y1-y2)/(x1-x2);	
			} // end if

		} break;

	default:break;

	} // end switch

	// do bounds check
	if ((xc1 < min_clip_x) || (xc1 > max_clip_x) ||
		(yc1 < min_clip_y) || (yc1 > max_clip_y) ||
		(xc2 < min_clip_x) || (xc2 > max_clip_x) ||
		(yc2 < min_clip_y) || (yc2 > max_clip_y) )
	{
		return(0);
	} // end if

	// store vars back
	x1 = xc1;
	y1 = yc1;
	x2 = xc2;
	y2 = yc2;

	return(1);

} // end Clip_Line

void DrawPixel_Alpha(uint32* target, uint32 color, float alpha)
{
	*target = SrColorMulFloat(*target, 1.f - alpha) + SrColorMulFloat(color, alpha);
}

///////////////////////////////////////////////////////////

int SrRasterizer::Draw_Line(int x0, int y0, // starting position 
	int x1, int y1, // ending position
	int color,     // color index
	uint32 *vb_start, int lpitch) // video buffer and memory pitch
{
	// this function draws a line from xo,yo to x1,y1 using differential error
	// terms (based on Bresenahams work)

	int dx,             // difference in x's
		dy,             // difference in y's
		dx2,            // dx,dy * 2
		dy2, 
		x_inc,          // amount in pixel space to move during drawing
		y_inc,          // amount in pixel space to move during drawing
		error,          // the discriminant i.e. error i.e. decision variable
		index;          // used for looping

	// pre-compute first pixel address in video buffer
	vb_start = vb_start + x0 + y0*lpitch;

	// compute horizontal and vertical deltas
	dx = x1-x0;
	dy = y1-y0;

	// test which direction the line is going in i.e. slope angle
	if (dx>=0)
	{
		x_inc = 1;

	} // end if line is moving right
	else
	{
		x_inc = -1;
		dx    = -dx;  // need absolute value

	} // end else moving left

	// test y component of slope

	if (dy>=0)
	{
		y_inc = lpitch;
	} // end if line is moving down
	else
	{
		y_inc = -lpitch;
		dy    = -dy;  // need absolute value

	} // end else moving up

	// compute (dx,dy) * 2
	dx2 = dx << 1;
	dy2 = dy << 1;

	// now based on which delta is greater we can draw the line
	if (dx > dy)
	{
		// initialize error term
		error = dy2 - dx; 

		// draw the line
		for (index=0; index <= dx; index++)
		{
			// set the pixel
			DrawPixel_Alpha(vb_start, color, 0.3f);
			//*vb_start = color;

			// test if error has overflowed
			if (error >= 0) 
			{
				error-=dx2;

				// move to next line
				vb_start+=y_inc;

			} // end if error overflowed

			// adjust the error term
			error+=dy2;

			// move to the next pixel
			vb_start+=x_inc;

		} // end for

	} // end if |slope| <= 1
	else
	{
		// initialize error term
		error = dx2 - dy; 

		// draw the line
		for (index=0; index <= dy; index++)
		{
			// set the pixel
			DrawPixel_Alpha(vb_start, color, 0.3f);
			//*vb_start = color;

			// test if error overflowed
			if (error >= 0)
			{
				error-=dy2;

				// move to next line
				vb_start+=x_inc;

			} // end if error overflowed

			// adjust the error term
			error+=dx2;

			// move to the next pixel
			vb_start+=y_inc;

		} // end for

	} // end else |slope| > 1

	// return success
	return(1);

} // end Draw_Line
