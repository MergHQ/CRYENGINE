/********************************************************/
/* AABB-triangle overlap test code                      */
/* by Tomas Akenine-Möller                              */
/* Function: int triBoxOverlap(float boxcenter[3],      */
/*          float boxhalfsize[3],float triverts[3][3]); */
/* History:                                             */
/*   2001-03-05: released the code in its first version */
/*   2001-06-18: changed the order of the tests, faster */
/*                                                      */
/* Acknowledgement: Many thanks to Pierre Terdiman for  */
/* suggestions and discussions on how to optimize code. */
/* Thanks to David Hunt for finding a ">="-bug!         */
/********************************************************/
#include "stdafx.h"
#include <math.h>
#include <stdio.h>
#define iX 0
#define iY 1
#define iZ 2
#define CROSS(dest,v1,v2) \
dest[0] = v1[1] * v2[2] - v1[2] * v2[1]; \
dest[1] = v1[2] * v2[0] - v1[0] * v2[2]; \
dest[2] = v1[0] * v2[1] - v1[1] * v2[0];
#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])
#define SUB(dest,v1,v2) \
dest[0] = v1[0] - v2[0]; \
dest[1] = v1[1] - v2[1]; \
dest[2] = v1[2] - v2[2];

#define FINDMINMAX(x0,x1,x2,min,max) \min = max = x0;   \
if (x1 < min) min = x1; \
	if (x1 > max) max = x1; \
		if (x2 < min) min = x2; \
			if (x2 > max) max = x2;
int planeBoxOverlap(float normal[3], float vert[3], float maxbox[3])	// -NJMP-
{
	int q;
	float vmin[3], vmax[3], v;
	for (q = iX; q <= iZ; q++)
	{
		v = vert[q];					// -NJMP-
		if (normal[q] > 0.0f)
		{
			vmin[q] = -maxbox[q] - v;	// -NJMP-
			vmax[q] = maxbox[q] - v;	// -NJMP-
		}
		else
		{			vmin[q] = maxbox[q] - v;	// -NJMP-
			vmax[q] = -maxbox[q] - v;	// -NJMP-
		}
	}	if (DOT(normal, vmin) > 0.0f) return 0;	// -NJMP-
	if (DOT(normal, vmax) >= 0.0f) return 1;	// -NJMP-
	return 0;
}
/*======================== iX-tests ========================*/#define AXISTEST_X01(a, b, fa, fb)			   \
p0 = a*v0[iY] - b*v0[iZ];			       	   \
p2 = a*v2[iY] - b*v2[iZ];			       	   \
if (p0 < p2) { min = p0; max = p2; } else { min = p2; max = p0; } \
rad = fa * boxhalfsize[iY] + fb * boxhalfsize[iZ];   \
if (min > rad || max < -rad) \return 0;
#define AXISTEST_X2(a, b, fa, fb)			   \
p0 = a*v0[iY] - b*v0[iZ];			           \
p1 = a*v1[iY] - b*v1[iZ];			       	   \
if (p0 < p1) { min = p0; max = p1; } else { min = p1; max = p0; } \
rad = fa * boxhalfsize[iY] + fb * boxhalfsize[iZ];   \
if (min > rad || max < -rad) \ return 0;
/*======================== iY-tests ========================*/
#define AXISTEST_Y02(a, b, fa, fb)			   \
p0 = -a*v0[iX] + b*v0[iZ];		      	   \
p2 = -a*v2[iX] + b*v2[iZ];	       	       	   \
if (p0 < p2) { min = p0; max = p2; } else { min = p2; max = p0; } \
rad = fa * boxhalfsize[iX] + fb * boxhalfsize[iZ];   \
if (min > rad || max < -rad) \return 0;
#define AXISTEST_Y1(a, b, fa, fb)			   \
p0 = -a*v0[iX] + b*v0[iZ];		      	   \
p1 = -a*v1[iX] + b*v1[iZ];	     	       	   \
if (p0 < p1) { min = p0; max = p1; } else { min = p1; max = p0; } \
rad = fa * boxhalfsize[iX] + fb * boxhalfsize[iZ];   \
if (min > rad || max < -rad) \
return 0;

/*======================== iZ-tests ========================*/
#define AXISTEST_Z12(a, b, fa, fb)			   \
p1 = a*v1[iX] - b*v1[iY];			           \
p2 = a*v2[iX] - b*v2[iY];			       	   \
if (p2 < p1) { min = p2; max = p1; } else { min = p1; max = p2; } \
rad = fa * boxhalfsize[iX] + fb * boxhalfsize[iY];   \
if (min > rad || max < -rad) \return 0;
#define AXISTEST_Z0(a, b, fa, fb)			   \
p0 = a*v0[iX] - b*v0[iY];				   \
p1 = a*v1[iX] - b*v1[iY];			           \
if (p0 < p1) { min = p0; max = p1; } else { min = p1; max = p0; } \
rad = fa * boxhalfsize[iX] + fb * boxhalfsize[iY];   \
if (min > rad || max < -rad) \
return 0;

int triBoxOverlap(float boxcenter[3], float boxhalfsize[3], float triverts[3][3])
{
	/*    use separating axis theorem to test overlap between triangle and box */
	/*    need to test for overlap in these directions: */
	/*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */
	/*       we do not even need to test these) */
	/*    2) normal of the triangle */
	/*    3) crossproduct(edge from tri, {x,y,z}-directin) */
	/*       this gives 3x3=9 more tests */
	float v0[3], v1[3], v2[3];
	//   float axis[3];
	float min, max, p0, p1, p2, rad, fex, fey, fez;		// -NJMP- "d" local variable removed
	float normal[3], e0[3], e1[3], e2[3];
	/* This is the fastest branch on Sun */
	/* move everything so that the boxcenter is in (0,0,0) */
	SUB(v0, triverts[0], boxcenter);
	SUB(v1, triverts[1], boxcenter);
	SUB(v2, triverts[2], boxcenter);
	/* compute triangle edges */
	SUB(e0, v1, v0);      /* tri edge 0 */
	SUB(e1, v2, v1);      /* tri edge 1 */
	SUB(e2, v0, v2);      /* tri edge 2 */
						  /* Bullet 3:  */
						  /*  test the 9 tests first (this was faster) */
	fex = fabsf(e0[iX]);
	fey = fabsf(e0[iY]);
	fez = fabsf(e0[iZ]);
	AXISTEST_X01(e0[iZ], e0[iY], fez, fey);
	AXISTEST_Y02(e0[iZ], e0[iX], fez, fex);
	AXISTEST_Z12(e0[iY], e0[iX], fey, fex);
	fex = fabsf(e1[iX]);
	fey = fabsf(e1[iY]);
	fez = fabsf(e1[iZ]);
	AXISTEST_X01(e1[iZ], e1[iY], fez, fey);
	AXISTEST_Y02(e1[iZ], e1[iX], fez, fex);
	AXISTEST_Z0(e1[iY], e1[iX], fey, fex);
	fex = fabsf(e2[iX]);
	fey = fabsf(e2[iY]);
	fez = fabsf(e2[iZ]);
	AXISTEST_X2(e2[iZ], e2[iY], fez, fey);
	AXISTEST_Y1(e2[iZ], e2[iX], fez, fex);
	AXISTEST_Z12(e2[iY], e2[iX], fey, fex);
	/* Bullet 1: */
	/*  first test overlap in the {x,y,z}-directions */
	/*  find min, max of the triangle each direction, and test for overlap in */
	/*  that direction -- this is equivalent to testing a minimal AABB around */
	/*  the triangle against the AABB */

	/* test in iX-direction */
	FINDMINMAX(v0[iX], v1[iX], v2[iX], min, max);
	if (min > boxhalfsize[iX] || max < -boxhalfsize[iX])		return 0;
	/* test in iY-direction */
	FINDMINMAX(v0[iY], v1[iY], v2[iY], min, max);
	if (min > boxhalfsize[iY] || max < -boxhalfsize[iY])		return 0;
	/* test in iZ-direction */
	FINDMINMAX(v0[iZ], v1[iZ], v2[iZ], min, max);
	if (min > boxhalfsize[iZ] || max < -boxhalfsize[iZ])		return 0;
	/* Bullet 2: */
	/*  test if the box intersects the plane of the triangle */
	/*  compute plane equation of triangle: normal*x+d=0 */
	CROSS(normal, e0, e1);
	// -NJMP- (line removed here)
	if (!planeBoxOverlap(normal, v0, boxhalfsize))		return 0;	// -NJMP-
	return 1;   /* box and triangle overlaps */
}