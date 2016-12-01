#pragma once

int triBoxOverlap(float boxcenter[3], float boxhalfsize[3], float triverts[3][3]);

bool triBoxIntersect(Box3 b, Point3* t)
{
	float centre[3] = { b.Center().x, b.Center().y, b.Center().z };
	float halfSize[3] = { b.Width().x / 2, b.Width().y / 2, b.Width().z / 2 };
	float verts[3][3];
	verts[0][0] = t[0][0];
	verts[0][1] = t[0][1];
	verts[0][2] = t[0][2];
	verts[1][0] = t[1][0];
	verts[1][1] = t[1][1];
	verts[1][2] = t[1][2];
	verts[2][0] = t[2][0];
	verts[2][1] = t[2][1];
	verts[2][2] = t[2][2];
	return triBoxOverlap(centre, halfSize, verts);
}