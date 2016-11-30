#include "stdafx.h"

#include "FlowPaintClassDesc.h"

FlowPaintTestClassDesc* FlowPaintTestClassDesc::GetInstance()
{
	static FlowPaintTestClassDesc instance;
	return &instance;
}
