// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Script/Elements/ScriptSignalReceiver.h"

#include <CrySerialization/Decorators/ActionButton.h>
#include <Schematyc/Env/IEnvRegistry.h>
#include <Schematyc/Env/Elements/IEnvSignal.h>
#include <Schematyc/Script/IScriptRegistry.h>
#include <Schematyc/Script/Elements/IScriptSignal.h>
#include <Schematyc/Script/Elements/IScriptTimer.h>
#include <Schematyc/SerializationUtils/ISerializationContext.h>
#include <Schematyc/Utils/IGUIDRemapper.h>
#include <Schematyc/Utils/StackString.h>

#include "Script/Graph/ScriptGraph.h"
#include "Script/Graph/ScriptGraphNode.h"
#include "Script/Graph/Nodes/ScriptGraphBeginNode.h"

namespace Schematyc
{
CScriptSignalReceiver::CScriptSignalReceiver()
	: CScriptElementBase(EScriptElementFlags::CanOwnScript)
	, m_type(EScriptSignalReceiverType::Unknown)
{
	CreateGraph();
}

CScriptSignalReceiver::CScriptSignalReceiver(const SGUID& guid, const char* szName, EScriptSignalReceiverType type, const SGUID& signalGUID)
	: CScriptElementBase(guid, szName, EScriptElementFlags::CanOwnScript)
	, m_type(type)
	, m_signalGUID(signalGUID)
{
	if (m_type != EScriptSignalReceiverType::Universal)
	{
		ScriptElementFlags elementFlags = CScriptElementBase::GetElementFlags();
		elementFlags.Add(EScriptElementFlags::FixedName);
		CScriptElementBase::SetElementFlags(elementFlags);
	}

	CreateGraph();
}

EScriptElementAccessor CScriptSignalReceiver::GetAccessor() const
{
	return EScriptElementAccessor::Private;
}

void CScriptSignalReceiver::EnumerateDependencies(const ScriptDependencyEnumerator& enumerator, EScriptDependencyType type) const
{
	SCHEMATYC_CORE_ASSERT(!enumerator.IsEmpty());
	if (!enumerator.IsEmpty())
	{
		enumerator(m_signalGUID);

		CScriptElementBase::EnumerateDependencies(enumerator, type);
	}
}

void CScriptSignalReceiver::RemapDependencies(IGUIDRemapper& guidRemapper)
{
	CScriptElementBase::RemapDependencies(guidRemapper);
}

void CScriptSignalReceiver::ProcessEvent(const SScriptEvent& event)
{
	CScriptElementBase::ProcessEvent(event);

	switch (m_type)
	{
	case EScriptSignalReceiverType::EnvSignal:
		{
			const IEnvSignal* pEnvSignal = GetSchematycCore().GetEnvRegistry().GetSignal(m_signalGUID);
			if (pEnvSignal)
			{
				CStackString name;
				name = "On";
				name.append(pEnvSignal->GetName());
				CScriptElementBase::SetName(name);
			}
			break;
		}
	case EScriptSignalReceiverType::ScriptSignal:
		{
			const IScriptSignal* pScriptSignal = DynamicCast<IScriptSignal>(GetSchematycCore().GetScriptRegistry().GetElement(m_signalGUID));
			if (pScriptSignal)
			{
				CStackString name;
				name = "On";
				name.append(pScriptSignal->GetName());
				CScriptElementBase::SetName(name);
			}
			break;
		}
	case EScriptSignalReceiverType::ScriptTimer:
		{
			const IScriptTimer* pScriptTimer = DynamicCast<IScriptTimer>(GetSchematycCore().GetScriptRegistry().GetElement(m_signalGUID));
			if (pScriptTimer)
			{
				CStackString name;
				name = "On";
				name.append(pScriptTimer->GetName());
				CScriptElementBase::SetName(name);
			}
			break;
		}
	}

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

void CScriptSignalReceiver::Serialize(Serialization::IArchive& archive)
{
	// #SchematycTODO : Shouldn't this be handled by CScriptElementBase itself?
	CScriptElementBase::Serialize(archive);
	CMultiPassSerializer::Serialize(archive);
	CScriptElementBase::SerializeExtensions(archive);
}

EScriptSignalReceiverType CScriptSignalReceiver::GetType() const
{
	return m_type;
}

SGUID CScriptSignalReceiver::GetSignalGUID() const
{
	return m_signalGUID;
}

void CScriptSignalReceiver::LoadDependencies(Serialization::IArchive& archive, const ISerializationContext& context)
{
	archive(m_signalGUID, "signalGUID");
}

void CScriptSignalReceiver::Load(Serialization::IArchive& archive, const ISerializationContext& context)
{
	archive(m_type, "type");
	archive(m_userDocumentation, "userDocumentation");

	if (m_type != EScriptSignalReceiverType::Universal)
	{
		ScriptElementFlags elementFlags = CScriptElementBase::GetElementFlags();
		elementFlags.Add(EScriptElementFlags::FixedName);
		CScriptElementBase::SetElementFlags(elementFlags);
	}
}

void CScriptSignalReceiver::Save(Serialization::IArchive& archive, const ISerializationContext& context)
{
	archive(m_type, "type");
	archive(m_signalGUID, "signalGUID");
	archive(m_userDocumentation, "userDocumentation");
}

void CScriptSignalReceiver::Edit(Serialization::IArchive& archive, const ISerializationContext& context)
{
	archive(m_userDocumentation, "userDocumentation", "Documentation");

	switch (m_type)
	{
	case EScriptSignalReceiverType::ScriptSignal:
	case EScriptSignalReceiverType::ScriptTimer:
		{
			archive(Serialization::ActionButton(functor(*this, &CScriptSignalReceiver::GoToSignal)), "goToSignal", "^Go To Signal");
			break;
		}
	}

	if (archive.isValidation())
	{
		Validate(archive, context);
	}
}

void CScriptSignalReceiver::Validate(Serialization::IArchive& archive, const ISerializationContext& context)
{
	// #SchematycTODO : Use script view to ensure signal is scoped correctly?

	switch (m_type)
	{
	case EScriptSignalReceiverType::EnvSignal:
		{
			const IEnvSignal* pEnvSignal = GetSchematycCore().GetEnvRegistry().GetSignal(m_signalGUID);
			if (!pEnvSignal)
			{
				archive.error(*this, "Failed to retrieve environment signal!");
			}
			break;
		}
	case EScriptSignalReceiverType::ScriptSignal:
		{
			const IScriptSignal* pScriptSignal = DynamicCast<IScriptSignal>(GetSchematycCore().GetScriptRegistry().GetElement(m_signalGUID));
			if (!pScriptSignal)
			{
				archive.error(*this, "Failed to retrieve script signal!");
			}
			break;
		}
	case EScriptSignalReceiverType::ScriptTimer:
		{
			const IScriptTimer* pScriptTimer = DynamicCast<IScriptTimer>(GetSchematycCore().GetScriptRegistry().GetElement(m_signalGUID));
			if (!pScriptTimer)
			{
				archive.error(*this, "Failed to retrieve script timer!");
			}
			break;
		}
	}
}

void CScriptSignalReceiver::CreateGraph()
{
	CScriptGraphPtr pScriptGraph = std::make_shared<CScriptGraph>(*this, EScriptGraphType::Signal);

	if (m_type != EScriptSignalReceiverType::Universal)
	{
		pScriptGraph->AddNode(std::make_shared<CScriptGraphNode>(GetSchematycCore().CreateGUID(), stl::make_unique<CScriptGraphBeginNode>())); // #SchematycTODO : Shouldn't we be using CScriptGraphNodeFactory::CreateNode() instead of instantiating the node directly?!?
	}

	CScriptElementBase::AddExtension(pScriptGraph);
}

void CScriptSignalReceiver::GoToSignal()
{
	CryLinkUtils::ExecuteCommand(CryLinkUtils::ECommand::Show, m_signalGUID);
}
} // Schematyc
