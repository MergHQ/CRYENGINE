#pragma once

#include "GeomDecal.h"

class GeomDecalCreateCallBack : public CreateMouseCallBack 
{
	IPoint2 sp0;
	GeomDecal *ob;
	Point3 p0;
public:
	int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat) override;
	void SetObj(GeomDecal *obj) { ob = obj; }
};