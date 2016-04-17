// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "PrimitiveRenderPass.h"

class CVolumeRenderPass : public CPrimitiveRenderPass
{
public:
	enum EVolumeType
	{
		eVolume_Box
	};

	CVolumeRenderPass()
		: CPrimitiveRenderPass()
	{
		m_primitiveType = ePrim_Box;
	}

	void SeVolumeType(EVolumeType volumeType)
	{
		EPrimitiveType primitiveType = ePrim_Box;
		if (volumeType == eVolume_Box)
			primitiveType = ePrim_Box;

		SetupPrimitiveType(primitiveType);
	}
};
