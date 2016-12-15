// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/Forward.h>

namespace Schematyc
{
struct SEntityClassProperties
{
	SEntityClassProperties();

	void                Serialize(Serialization::IArchive& archive);

	static inline SGUID ReflectSchematycType(CTypeInfo<SEntityClassProperties>& typeInfo);

	string              icon;
	bool                bHideInEditor;
	bool                bTriggerAreas;
};

SGUID SEntityClassProperties::ReflectSchematycType(CTypeInfo<SEntityClassProperties>& typeInfo)
{
	return "cb7311ea-9a07-490a-a351-25c298a91550"_schematyc_guid;
}
} // Schematyc
