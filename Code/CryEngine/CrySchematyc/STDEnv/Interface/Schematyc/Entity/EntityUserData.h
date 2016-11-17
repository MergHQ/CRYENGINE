// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace Schematyc
{
struct SEntityUserData
{
	inline SEntityUserData(bool _bIsPreview)
		: bIsPreview(_bIsPreview)
	{}

	bool bIsPreview;
};
} // Schematyc
