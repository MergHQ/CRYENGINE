// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/Forward.h>

namespace Schematyc
{

struct SEntityClassProperties
{
	SEntityClassProperties();

	void        Serialize(Serialization::IArchive& archive);

	static void ReflectType(CTypeDesc<SEntityClassProperties>& desc);

	string      icon;
	bool        bHideInEditor;
	bool        bTriggerAreas;
};

} // Schematyc
