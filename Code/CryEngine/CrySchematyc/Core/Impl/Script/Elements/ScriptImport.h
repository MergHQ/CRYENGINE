// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Script/Elements/IScriptImport.h>

#include "Script/ScriptElementBase.h"

namespace Schematyc
{
class CScriptImport : public CScriptElementBase<IScriptImport>
{
public:

	CScriptImport();
	CScriptImport(const SGUID& guid, const SGUID& moduleGUID);

	// IScriptElement
	virtual EScriptElementAccessor GetAccessor() const override;
	virtual void                   EnumerateDependencies(const ScriptDependencyEnumerator& enumerator, EScriptDependencyType type) const override;
	virtual void                   RemapDependencies(IGUIDRemapper& guidRemapper) override;
	virtual void                   ProcessEvent(const SScriptEvent& event) override;
	virtual void                   Serialize(Serialization::IArchive& archive) override;
	// ~IScriptElement

	// IScriptImport
	virtual SGUID GetModuleGUID() const override;
	// ~IScriptImport

private:

	SGUID m_moduleGUID;
};
} // Schematyc
