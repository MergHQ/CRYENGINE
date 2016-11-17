// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Script/Elements/IScriptRoot.h>
#include <Schematyc/SerializationUtils/MultiPassSerializer.h>

#include "Script/ScriptElementBase.h"

namespace Schematyc
{
class CScriptRoot : public CScriptElementBase<IScriptRoot>, public CMultiPassSerializer
{
public:

	CScriptRoot();

	// IScriptElement
	virtual EScriptElementAccessor GetAccessor() const override;
	virtual void                   EnumerateDependencies(const ScriptDependencyEnumerator& enumerator, EScriptDependencyType type) const override;
	virtual void                   RemapDependencies(IGUIDRemapper& guidRemapper) override;
	virtual void                   ProcessEvent(const SScriptEvent& event) override;
	virtual void                   Serialize(Serialization::IArchive& archive) override;
	// ~IScriptElement
};
} // Schematyc
