// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Script/Elements/ScriptInterfaceTask.h"

#include <CrySerialization/BlackBox.h>
#include <CrySerialization/IArchiveHost.h>
#include <CrySerialization/STL.h>
#include <Schematyc/Env/IEnvRegistry.h>
#include <Schematyc/Env/Elements/IEnvInterface.h>
#include <Schematyc/SerializationUtils/ISerializationContext.h>
#include <Schematyc/SerializationUtils/SerializationUtils.h>
#include <Schematyc/Utils/IGUIDRemapper.h>

namespace Schematyc
{
CScriptInterfaceTask::CScriptInterfaceTask() {}

CScriptInterfaceTask::CScriptInterfaceTask(const SGUID& guid, const char* szName)
	: CScriptElementBase(guid, szName)
{}

EScriptElementAccessor CScriptInterfaceTask::GetAccessor() const
{
	return EScriptElementAccessor::Private;
}

void CScriptInterfaceTask::EnumerateDependencies(const ScriptDependencyEnumerator& enumerator, EScriptDependencyType type) const {}

void CScriptInterfaceTask::RemapDependencies(IGUIDRemapper& guidRemapper)                                                        {}

void CScriptInterfaceTask::ProcessEvent(const SScriptEvent& event)
{
	CScriptElementBase::ProcessEvent(event);

	switch (event.id)
	{
	case EScriptEventId::EditorAdd:
	case EScriptEventId::EditorPaste:
		{
			m_userDocumentation.SetCurrentUserAsAuthor();
			break;
		}
	}
}

void CScriptInterfaceTask::Serialize(Serialization::IArchive& archive)
{
	// #SchematycTODO : Shouldn't this be handled by CScriptElementBase itself?
	CScriptElementBase::Serialize(archive);
	CMultiPassSerializer::Serialize(archive);
	CScriptElementBase::SerializeExtensions(archive);
}

const char* CScriptInterfaceTask::GetAuthor() const
{
	return m_userDocumentation.author.c_str();
}

const char* CScriptInterfaceTask::GetDescription() const
{
	return m_userDocumentation.description.c_str();
}

void CScriptInterfaceTask::Load(Serialization::IArchive& archive, const ISerializationContext& context)
{
	archive(m_userDocumentation, "userDocumentation");
}

void CScriptInterfaceTask::Save(Serialization::IArchive& archive, const ISerializationContext& context)
{
	archive(m_userDocumentation, "userDocumentation");
}

void CScriptInterfaceTask::Edit(Serialization::IArchive& archive, const ISerializationContext& context)
{
	archive(m_userDocumentation, "userDocumentation", "Documentation");
}
} // Schematyc
