#pragma once

enum EGeomForm { GeomForm_Vertices, GeomForm_Edges, GeomForm_Surface, GeomForm_Volume };
struct PosNorm {
	Vec3 vPos,vNorm;
	PosNorm() {}
	PosNorm(type_zero) { zero(); }
	void zero() { vPos.zero(); vNorm.zero(); }
	void operator<<=(const QuatTS&) {}
};

struct AABB {
	Vec3 bounds[2];
};
