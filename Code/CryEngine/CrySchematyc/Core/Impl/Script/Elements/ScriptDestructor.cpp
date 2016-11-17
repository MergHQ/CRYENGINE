// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Script/Elements/ScriptDestructor.h"

#include <Schematyc/SerializationUtils/ISerializationContext.h>
#include <Schematyc/Utils/IGUIDRemapper.h>

#include "Script/Graph/ScriptGraph.h"
#include "Script/Graph/ScriptGraphNode.h"
#include "Script/Graph/Nodes/ScriptGraphBeginNode.h"

namespace Schematyc
{
CScriptDestructor::CScriptDestructor()
	: CScriptElementBase({ EScriptElementFlags::CanOwnScript, EScriptElementFlags::FixedName })
{
	CreateGraph();
}

CScriptDestructor::CScriptDestructor(const SGUID& guid, const char* szName)
	: CScriptElementBase(guid, szName, { EScriptElementFlags::CanOwnScript, EScriptElementFlags::FixedName })
{
	CreateGraph();
}

EScriptElementAccessor CScriptDestructor::GetAccessor() const
{
	return EScriptElementAccessor::Private;
}

void CScriptDestructor::EnumerateDependencies(const ScriptDependencyEnumerator& enumerator, EScriptDependencyType type) const {}

void CScriptDestructor::RemapDependencies(IGUIDRemapper& guidRemapper) {}

void CScriptDestructor::ProcessEvent(const SScriptEvent& event)
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

void CScriptDestructor::Serialize(Serialization::IArchive& archive)
{
	LOADING_TIME_PROFILE_SECTION;

	CScriptElementBase::Serialize(archive);

	switch (SerializationContext::GetPass(archive))
	{
	case ESerializationPass::LoadDependencies:
	case ESerializationPass::Save:
		{
			archive(m_userDocumentation, "userDocumentation");
			break;
		}
	case ESerializationPass::Edit:
		{
			archive(m_userDocumentation, "userDocumentation", "Documentation");
			break;
		}
	}

	CScriptElementBase::SerializeExtensions(archive);
}

void CScriptDestructor::CreateGraph() {}
} // Schematyc
