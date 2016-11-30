#include "stdafx.h"
#include "GeomDecalCreateCallback.h"

int GeomDecalCreateCallBack::proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat)
{
	// Simply create at the selected location
	mat.SetTrans(vpt->SnapPoint(m, m, NULL, SNAP_IN_3D));

	return CREATE_STOP;
}