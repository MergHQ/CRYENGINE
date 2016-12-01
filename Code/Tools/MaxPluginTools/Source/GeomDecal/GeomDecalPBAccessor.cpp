#include "stdafx.h"
#include "GeomDecalPBAccessor.h"

#include "GeomDecalParamBlock.h"
#include "GeomDecal.h"

void GeomDecalPBAccessor::Set(PB2Value & v, ReferenceMaker * owner, ParamID id, int tabIndex, TimeValue t)
{
	GeomDecal* geomDecal = (GeomDecal*)owner;
	IParamBlock2* pblock2 = geomDecal->pblock2;


	switch (id)
	{
	default:
		break;
	}
}
