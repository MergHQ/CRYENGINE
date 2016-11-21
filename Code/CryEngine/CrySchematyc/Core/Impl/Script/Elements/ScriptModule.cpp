// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Script/Elements/ScriptModule.h"

#include <Schematyc/SerializationUtils/ISerializationContext.h>
#include <Schematyc/Utils/IGUIDRemapper.h>

namespace Schematyc
{
CScriptModule::CScriptModule()
	: CScriptElementBase(EScriptElementFlags::MustOwnScript)
{}

CScriptModule::CScriptModule(const SGUID& guid, const char* szName)
	: CScriptElementBase(guid, szName, EScriptElementFlags::MustOwnScript)
{}

void CScriptModule::EnumerateDependencies(const ScriptDependencyEnumerator& enumerator, EScriptDependencyType type) const {}

void CScriptModule::RemapDependencies(IGUIDRemapper& guidRemapper)                                                        {}

void CScriptModule::ProcessEvent(const SScriptEvent& event)                                                               {}

void CScriptModule::Serialize(Serialization::IArchive& archive)
{
	// #SchematycTODO : Shouldn't this be handled by CScriptElementBase itself?
	CScriptElementBase::Serialize(archive);
	CMultiPassSerializer::Serialize(archive);
	CScriptElementBase::SerializeExtensions(archive);
}
} // Schematyc
