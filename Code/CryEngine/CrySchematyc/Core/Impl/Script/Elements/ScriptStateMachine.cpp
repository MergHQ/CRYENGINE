// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Script/Elements/ScriptStateMachine.h"

#include <CrySerialization/IArchiveHost.h>
#include <Schematyc/Script/IScriptGraph.h>
#include <Schematyc/SerializationUtils/ISerializationContext.h>
#include <Schematyc/Utils/Assert.h>
#include <Schematyc/Utils/IGUIDRemapper.h>

#include "Script/ScriptView.h"
#include "Script/Graph/ScriptGraph.h"
#include "Script/Graph/ScriptGraphNode.h"
#include "Script/Graph/Nodes/ScriptGraphBeginNode.h"

SERIALIZATION_ENUM_BEGIN_NESTED(Schematyc, EScriptStateMachineLifetime, "Schematyc Script State Machine Lifetime")
SERIALIZATION_ENUM(Schematyc::EScriptStateMachineLifetime::Persistent, "persistent", "Persistent")
SERIALIZATION_ENUM(Schematyc::EScriptStateMachineLifetime::Task, "task", "Task")
SERIALIZATION_ENUM_END()

namespace Schematyc
{
CScriptStateMachine::CScriptStateMachine()
	: CScriptElementBase(EScriptElementFlags::CanOwnScript)
	, m_lifetime(EScriptStateMachineLifetime::Persistent)
{
	CreateTransitionGraph();
}

CScriptStateMachine::CScriptStateMachine(const SGUID& guid, const char* szName, EScriptStateMachineLifetime lifetime, const SGUID& contextGUID, const SGUID& partnerGUID)
	: CScriptElementBase(guid, szName, EScriptElementFlags::CanOwnScript)
	, m_lifetime(lifetime)
	, m_contextGUID(contextGUID)
	, m_partnerGUID(partnerGUID)
{
	CreateTransitionGraph();
}

void CScriptStateMachine::EnumerateDependencies(const ScriptDependencyEnumerator& enumerator, EScriptDependencyType type) const {}

void CScriptStateMachine::RemapDependencies(IGUIDRemapper& guidRemapper)
{
	m_partnerGUID = guidRemapper.Remap(m_partnerGUID);
	m_contextGUID = guidRemapper.Remap(m_contextGUID);
	m_transitionGraphGUID = guidRemapper.Remap(m_transitionGraphGUID);
}

void CScriptStateMachine::ProcessEvent(const SScriptEvent& event)
{
	switch (event.id)
	{
	case EScriptEventId::EditorFixUp:
	case EScriptEventId::EditorPaste:
		{
			RefreshTransitionGraph();
			break;
		}
	}

	CScriptElementBase::ProcessEvent(event);
}

void CScriptStateMachine::Serialize(Serialization::IArchive& archive)
{
	// #SchematycTODO : Shouldn't this be handled by CScriptElementBase itself?
	CScriptElementBase::Serialize(archive);
	CMultiPassSerializer::Serialize(archive);
	CScriptElementBase::SerializeExtensions(archive);
}

EScriptStateMachineLifetime CScriptStateMachine::GetLifetime() const
{
	return m_lifetime;
}

SGUID CScriptStateMachine::GetContextGUID() const
{
	return m_contextGUID;
}

SGUID CScriptStateMachine::GetPartnerGUID() const
{
	return m_partnerGUID;
}

void CScriptStateMachine::Load(Serialization::IArchive& archive, const ISerializationContext& context)
{
	archive(m_lifetime, "lifetime", "!Lifetime");
	archive(m_contextGUID, "contextGUID");
	archive(m_transitionGraphGUID, "transition_graph_guid");
	archive(m_partnerGUID, "partnerGUID");
}

void CScriptStateMachine::Save(Serialization::IArchive& archive, const ISerializationContext& context)
{
	archive(m_lifetime, "lifetime", "!Lifetime");
	archive(m_contextGUID, "contextGUID");
	archive(m_transitionGraphGUID, "transition_graph_guid");

	archive(m_partnerGUID, "partnerGUID");
}

void CScriptStateMachine::Edit(Serialization::IArchive& archive, const ISerializationContext& context)
{
	/*
	typedef std::vector<SGUID> GUIDs;

	GUIDs partnerGUIDs;
	Serialization::StringList partnerNames;
	partnerGUIDs.reserve(16);
	partnerNames.reserve(16);
	partnerGUIDs.push_back(SGUID());
	partnerNames.push_back("None");

	CScriptView scriptView(GetParent()->GetGUID());

	auto visitStateMachine = [this, &partnerGUIDs, &partnerNames](const IScriptStateMachine& stateMachine) -> EVisitStatus
	{
		if (stateMachine.GetLifetime() == EScriptStateMachineLifetime::Persistent)
		{
			const SGUID stateMachineGUID = stateMachine.GetGUID();
			if (stateMachineGUID != CScriptElementBase::GetGUID())
			{
				partnerGUIDs.push_back(stateMachineGUID);
				partnerNames.push_back(stateMachine.GetName());
			}
		}
		return EVisitStatus::Continue;
	};
	scriptView.VisitScriptStateMachines(ScriptStateMachineConstVisitor::FromLambda(visitStateMachine), EDomainScope::Local);

	GUIDs::const_iterator itPartnerGUID = std::find(partnerGUIDs.begin(), partnerGUIDs.end(), m_partnerGUID);
	const int partnerIdx = itPartnerGUID != partnerGUIDs.end() ? static_cast<int>(itPartnerGUID - partnerGUIDs.begin()) : 0;
	Serialization::StringListValue partnerName(partnerNames, partnerIdx);
	archive(partnerName, "partnerName", "Partner");
	if (archive.isInput())
	{
		m_partnerGUID = partnerGUIDs[partnerName.index()];
	}
	*/
}

void CScriptStateMachine::CreateTransitionGraph()
{
	CScriptGraphPtr pScriptGraph = std::make_shared<CScriptGraph>(*this, EScriptGraphType::Transition);
	pScriptGraph->AddNode(std::make_shared<CScriptGraphNode>(gEnv->pSchematyc->CreateGUID(), stl::make_unique<CScriptGraphBeginNode>()));   // #SchematycTODO : Shouldn't we be using CScriptGraphNodeFactory::CreateNode() instead of instantiating the node directly?!?
	CScriptElementBase::AddExtension(pScriptGraph);
}

void CScriptStateMachine::RefreshTransitionGraph()
{
	if (gEnv->IsEditor())
	{
		/*IScriptFile& file = CScriptElementBase::GetFile();
		   if(!file.IsDummyFile() && (!m_transitionGraphGUID || !file.GetGraph(m_transitionGraphGUID)))
		   {
		   IDocGraph* pTransitionGraph = file.AddGraph(SScriptGraphParams(CScriptElementBase::GetGUID(), "Transitions", EScriptGraphType::Transition_DEPRECATED, SGUID()));
		   SCHEMATYC_CORE_ASSERT(pTransitionGraph);
		   if(pTransitionGraph)
		   {
		    IScriptGraphNode* pBeginNode = pTransitionGraph->AddNode(EScriptGraphNodeType::BeginState, SGUID(), SGUID(), Vec2(0.0f, 0.0f));
		    if(m_lifetime == EScriptStateMachineLifetime::Task)
		    {
		      IScriptGraphNode* pEndNode = pTransitionGraph->AddNode(EScriptGraphNodeType::EndState, SGUID(), SGUID(), Vec2(200.0f, 0.0f));
		      pTransitionGraph->AddLink(pBeginNode->GetGUID(), pBeginNode->GetOutputName(0), pEndNode->GetGUID(), pEndNode->GetInputName(0));
		    }
		    m_transitionGraphGUID = pTransitionGraph->GetGUID();
		   }
		   }*/
	}
}
} // Schematyc
