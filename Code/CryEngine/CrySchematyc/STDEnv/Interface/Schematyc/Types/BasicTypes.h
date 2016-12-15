// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/Forward.h>

namespace Schematyc
{
enum class ExplicitEntityId : EntityId
{
	Invalid = 0
};

inline bool Serialize(Serialization::IArchive& archive, ExplicitEntityId& value, const char* szName, const char* szLabel)
{
	if (!archive.isEdit())
	{
		return archive(static_cast<EntityId>(value), szName, szLabel);
	}
	return true;
}

inline void ToString(IString& output, const ExplicitEntityId& input)
{
	output.Format("%d", static_cast<EntityId>(input));
}

inline SGUID ReflectSchematycType(CTypeInfo<ExplicitEntityId>& typeInfo)
{
	typeInfo.SetToStringMethod<&ToString>();
	typeInfo.DeclareSerializeable();
	return "00782e22-3188-4538-b4f2-8749b8a9dc48"_schematyc_guid;
}
} // Schematyc