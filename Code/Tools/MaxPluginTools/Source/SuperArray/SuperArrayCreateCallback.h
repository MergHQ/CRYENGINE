#pragma once

#include "SuperArray.h"

class SuperArrayCreateCallBack : public CreateMouseCallBack 
{
	IPoint2 sp0;
	SuperArray *ob;
	Point3 p0;
public:
	int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat) override;
	void SetObj(SuperArray *obj) { ob = obj; }
};