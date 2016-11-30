#include "stdafx.h"
#include "SuperArrayPBAccessor.h"

#include "SuperArrayParamBlock.h"
#include "SuperArray.h"

void SuperArrayPBAccessor::Set(PB2Value & v, ReferenceMaker * owner, ParamID id, int tabIndex, TimeValue t)
{
	SuperArray* superArray = (SuperArray*)owner;
	IParamBlock2* pblock2 = superArray->pblock2;

	switch (id)
	{
	case eSuperArrayParam::rand_rot_max:
	case eSuperArrayParam::rand_rot_min:
	{
		// If min > max or max < min, adjust the values to fix it.
		Point3 randomRotationMin;
		pblock2->GetValue(eSuperArrayParam::rand_rot_min, t, randomRotationMin, superArray->ivalid);
		Point3 randomRotationMax;
		pblock2->GetValue(eSuperArrayParam::rand_rot_max, t, randomRotationMax, superArray->ivalid);

		if (id == eSuperArrayParam::rand_rot_min)
		{
			Point3 max = ComponentwiseMax(randomRotationMin, randomRotationMax);
			if (max != randomRotationMax)
				pblock2->SetValue(eSuperArrayParam::rand_rot_max, t, max);
		}
		else if (id == eSuperArrayParam::rand_rot_max)
		{
			Point3 min = ComponentwiseMin(randomRotationMin, randomRotationMax);
			if (min != randomRotationMin)
				pblock2->SetValue(eSuperArrayParam::rand_rot_min, t, min);
		}
	}
	case eSuperArrayParam::rand_offset_max:
	case eSuperArrayParam::rand_offset_min:
	{
		// If min > max or max < min, adjust the values to fix it.
		Point3 randomOffsetMin;
		pblock2->GetValue(eSuperArrayParam::rand_offset_min, t, randomOffsetMin, superArray->ivalid);
		Point3 randomOffsetMax;
		pblock2->GetValue(eSuperArrayParam::rand_offset_max, t, randomOffsetMax, superArray->ivalid);

		if (id == eSuperArrayParam::rand_offset_min)
		{
			Point3 max = ComponentwiseMax(randomOffsetMin, randomOffsetMax);
			if (max != randomOffsetMax)
				pblock2->SetValue(eSuperArrayParam::rand_offset_max, t, max);
		}
		else if (id == eSuperArrayParam::rand_offset_max)
		{
			Point3 min = ComponentwiseMin(randomOffsetMin, randomOffsetMax);
			if (min != randomOffsetMin)
				pblock2->SetValue(eSuperArrayParam::rand_offset_min, t, min);
		}
	}
	case eSuperArrayParam::rand_scale_max:
	case eSuperArrayParam::rand_scale_min:
	{
		// If min > max or max < min, adjust the values to fix it.
		Point3 randomScaleMin;
		pblock2->GetValue(eSuperArrayParam::rand_scale_min, t, randomScaleMin, superArray->ivalid);
		Point3 randomScaleMax;
		pblock2->GetValue(eSuperArrayParam::rand_scale_max, t, randomScaleMax, superArray->ivalid);

		if (id == eSuperArrayParam::rand_scale_min)
		{
			Point3 max = ComponentwiseMax(randomScaleMin, randomScaleMax);
			if (max != randomScaleMax)
				pblock2->SetValue(eSuperArrayParam::rand_scale_max, t, max);
		}
		else if (id == eSuperArrayParam::rand_scale_max)
		{
			Point3 min = ComponentwiseMin(randomScaleMin, randomScaleMax);
			if (min != randomScaleMin)
				pblock2->SetValue(eSuperArrayParam::rand_scale_min, t, min);
		}
	}
	default:
		break;
	}
}
