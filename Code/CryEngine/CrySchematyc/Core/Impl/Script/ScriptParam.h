// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/SerializationUtils/MultiPassSerializer.h>
#include <Schematyc/Script/ScriptDependencyEnumerator.h>
#include <Schematyc/Utils/GUID.h>

#include "Script/ScriptVariableData.h"

namespace Schematyc
{
// Forward declare interfaces.
struct IGUIDRemapper;

struct SScriptParam : public CMultiPassSerializer
{
	// CMultiPassSerializer
	virtual void LoadDependencies(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void Load(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void Save(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void Edit(Serialization::IArchive& archive, const ISerializationContext& context) override;
	// ~CMultiPassSerializer

	SGUID               guid;
	string              name;
	CScriptVariableData data;
};

typedef std::vector<SScriptParam> ScriptParams;

namespace ScriptParam
{
void EnumerateDependencies(const ScriptParams& params, const ScriptDependencyEnumerator& enumerator, EScriptDependencyType type);
void RemapDependencies(ScriptParams& params, IGUIDRemapper& guidRemapper);
} // ScriptParam
} // Schematyc
