// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Script/Elements/ScriptImport.h"

#include <Schematyc/Script/IScriptRegistry.h>
#include <Schematyc/Script/Elements/IScriptModule.h>
#include <Schematyc/SerializationUtils/ISerializationContext.h>
#include <Schematyc/Utils/IGUIDRemapper.h>

namespace Schematyc
{
CScriptImport::CScriptImport()
	: CScriptElementBase(EScriptElementFlags::FixedName)
{}

CScriptImport::CScriptImport(const SGUID& guid, const SGUID& moduleGUID)
	: CScriptElementBase(guid, nullptr, EScriptElementFlags::FixedName)
	, m_moduleGUID(moduleGUID)
{}

EScriptElementAccessor CScriptImport::GetAccessor() const
{
	return EScriptElementAccessor::Public;
}

void CScriptImport::EnumerateDependencies(const ScriptDependencyEnumerator& enumerator, EScriptDependencyType type) const
{
	enumerator(m_moduleGUID);
}

void CScriptImport::RemapDependencies(IGUIDRemapper& guidRemapper)
{
	m_moduleGUID = guidRemapper.Remap(m_moduleGUID);
}

void CScriptImport::ProcessEvent(const SScriptEvent& event)
{
	const IScriptModule* pScriptModule = DynamicCast<IScriptModule>(GetSchematycCore().GetScriptRegistry().GetElement(m_moduleGUID));
	if (pScriptModule)
	{
		CScriptElementBase::SetName(pScriptModule->GetName());
	}
	else
	{
		CScriptElementBase::SetName(nullptr);
	}
}

void CScriptImport::Serialize(Serialization::IArchive& archive)
{
	LOADING_TIME_PROFILE_SECTION;

	CScriptElementBase::Serialize(archive);

	switch (SerializationContext::GetPass(archive))
	{
	case ESerializationPass::LoadDependencies:
	case ESerializationPass::Save:
		{
			archive(m_moduleGUID, "moduleGUID");
			break;
		}
	}

	CScriptElementBase::SerializeExtensions(archive);
}

SGUID CScriptImport::GetModuleGUID() const
{
	return m_moduleGUID;
}
} // Schematyc
