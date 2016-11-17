// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Script/Graph/Nodes/ScriptGraphReceiveSignalNode.h"

#include <CrySerialization/Decorators/ActionButton.h>
#include <Schematyc/IObject.h>
#include <Schematyc/Compiler/CompilerContext.h>
#include <Schematyc/Compiler/IGraphNodeCompiler.h>
#include <Schematyc/Env/IEnvRegistry.h>
#include <Schematyc/Env/Elements/IEnvSignal.h>
#include <Schematyc/Script/IScriptRegistry.h>
#include <Schematyc/Script/Elements/IScriptSignal.h>
#include <Schematyc/Script/Elements/IScriptTimer.h>
#include <Schematyc/Utils/IGUIDRemapper.h>
#include <Schematyc/Utils/StackString.h>

#include "Runtime/RuntimeClass.h"
#include "Script/ScriptView.h"
#include "Script/Graph/ScriptGraphNode.h"
#include "Script/Graph/ScriptGraphNodeFactory.h"
#include "SerializationUtils/SerializationContext.h"

namespace Schematyc
{
CScriptGraphReceiveSignalNode::CScriptGraphReceiveSignalNode() {}

CScriptGraphReceiveSignalNode::CScriptGraphReceiveSignalNode(const SElementId& signalId)
	: m_signalId(signalId)
{}

SGUID CScriptGraphReceiveSignalNode::GetTypeGUID() const
{
	return ms_typeGUID;
}

void CScriptGraphReceiveSignalNode::CreateLayout(CScriptGraphNodeLayout& layout)
{
	layout.SetColor(EScriptGraphColor::Green);

	if (!GUID::IsEmpty(m_signalId.guid))
	{
		switch (m_signalId.domain)
		{
		case EDomain::Env:
			{
				const IEnvSignal* pEnvSignal = GetSchematycCore().GetEnvRegistry().GetSignal(m_signalId.guid);
				if (pEnvSignal)
				{
					const IScriptElement& element = CScriptGraphNodeModel::GetNode().GetGraph().GetElement();
					CScriptView scriptView(element.GetGUID());

					CStackString name;
					scriptView.QualifyName(*pEnvSignal, name);
					layout.SetName(name.c_str());

					layout.AddOutput("Out", m_signalId.guid, EScriptGraphPortFlags::Signal);

					for (uint32 signalInputIdx = 0, signalInputCount = pEnvSignal->GetInputCount(); signalInputIdx < signalInputCount; ++signalInputIdx)
					{
						CAnyConstPtr pData = pEnvSignal->GetInputData(signalInputIdx);
						SCHEMATYC_CORE_ASSERT(pData);
						if (pData)
						{
							layout.AddOutputWithData(CGraphPortId::FromUniqueId(pEnvSignal->GetInputId(signalInputIdx)), pEnvSignal->GetInputName(signalInputIdx), pData->GetTypeInfo().GetGUID(), { EScriptGraphPortFlags::Data, EScriptGraphPortFlags::MultiLink }, *pData);
						}
					}
				}
				break;
			}
		case EDomain::Script:
			{
				const IScriptElement* pScriptElement = GetSchematycCore().GetScriptRegistry().GetElement(m_signalId.guid);
				if (pScriptElement)
				{
					switch (pScriptElement->GetElementType())
					{
					case EScriptElementType::Signal:
						{
							const IScriptSignal& scriptSignal = DynamicCast<IScriptSignal>(*pScriptElement);

							const IScriptElement& element = CScriptGraphNodeModel::GetNode().GetGraph().GetElement();
							CScriptView scriptView(element.GetGUID());

							CStackString name;
							scriptView.QualifyName(scriptSignal, EDomainQualifier::Local, name);
							layout.SetName(name.c_str());

							layout.AddOutput("Out", m_signalId.guid, EScriptGraphPortFlags::Signal);
							break;
						}
					case EScriptElementType::Timer:
						{
							const IScriptTimer& scriptTimer = DynamicCast<IScriptTimer>(*pScriptElement);

							const IScriptElement& element = CScriptGraphNodeModel::GetNode().GetGraph().GetElement();
							CScriptView scriptView(element.GetGUID());

							CStackString name;
							scriptView.QualifyName(scriptTimer, EDomainQualifier::Local, name);
							layout.SetName(name.c_str());

							layout.AddOutput("Out", m_signalId.guid, EScriptGraphPortFlags::Signal);
							break;
						}
					}
				}
				break;
			}
		}
	}
}

void CScriptGraphReceiveSignalNode::Compile(SCompilerContext& context, IGraphNodeCompiler& compiler) const
{
	CRuntimeClass* pClass = context.interfaces.Query<CRuntimeClass>();
	if (pClass)
	{
		const IScriptElement* pOwner = CScriptGraphNodeModel::GetNode().GetGraph().GetElement().GetParent();
		for (; pOwner; pOwner = pOwner->GetParent())
		{
			if (pOwner->GetElementType() != EScriptElementType::Filter)
			{
				break;
			}
		}
		switch (pOwner->GetElementType())
		{
		case EScriptElementType::Class:
			{
				pClass->AddSignalReceiver(m_signalId.guid, compiler.GetGraphIdx(), SRuntimeActivationParams(compiler.GetGraphNodeIdx(), EOutputIdx::Out, EActivationMode::Output));
				compiler.BindCallback(&Execute);
				break;
			}
		case EScriptElementType::State:
			{
				const uint32 stateIdx = pClass->FindState(pOwner->GetGUID());
				if (stateIdx != InvalidIdx)
				{
					pClass->AddStateSignalReceiver(stateIdx, m_signalId.guid, compiler.GetGraphIdx(), SRuntimeActivationParams(compiler.GetGraphNodeIdx(), EOutputIdx::Out, EActivationMode::Output));
					compiler.BindCallback(&Execute);
				}
				else
				{
					SCHEMATYC_COMPILER_ERROR("Failed to retrieve class state!");
				}
				break;
			}
		}
	}
}

void CScriptGraphReceiveSignalNode::LoadDependencies(Serialization::IArchive& archive, const ISerializationContext& context)
{
	archive(m_signalId, "signalId");
}

void CScriptGraphReceiveSignalNode::Save(Serialization::IArchive& archive, const ISerializationContext& context)
{
	archive(m_signalId, "signalId");
}

void CScriptGraphReceiveSignalNode::Edit(Serialization::IArchive& archive, const ISerializationContext& context)
{
	archive(Serialization::ActionButton(functor(*this, &CScriptGraphReceiveSignalNode::GoToSignal)), "goToSignal", "^Go To Signal");

	Validate(archive, context);
}

void CScriptGraphReceiveSignalNode::Validate(Serialization::IArchive& archive, const ISerializationContext& context)
{
	if (!GUID::IsEmpty(m_signalId.guid))
	{
		switch (m_signalId.domain)
		{
		case EDomain::Env:
			{
				const IEnvSignal* pEnvSignal = GetSchematycCore().GetEnvRegistry().GetSignal(m_signalId.guid);
				if (!pEnvSignal)
				{
					archive.error(*this, "Unable to retrieve environment signal!");
				}
				break;
			}
		case EDomain::Script:
			{
				const IScriptElement* pScriptElement = GetSchematycCore().GetScriptRegistry().GetElement(m_signalId.guid);
				if (!pScriptElement)
				{
					archive.error(*this, "Unable to retrieve script signal!");
				}
				break;
			}
		}
	}
}

void CScriptGraphReceiveSignalNode::RemapDependencies(IGUIDRemapper& guidRemapper)
{
	if (m_signalId.domain == EDomain::Script)
	{
		m_signalId.guid = guidRemapper.Remap(m_signalId.guid);
	}
}

void CScriptGraphReceiveSignalNode::Register(CScriptGraphNodeFactory& factory)
{
	class CCreator : public IScriptGraphNodeCreator
	{
	private:

		class CNodeCreationMenuCommand : public IScriptGraphNodeCreationMenuCommand
		{
		public:

			CNodeCreationMenuCommand(const SElementId& signalId)
				: m_signalId(signalId)
			{}

			// IMenuCommand

			IScriptGraphNodePtr Execute(const Vec2& pos)
			{
				return std::make_shared<CScriptGraphNode>(GetSchematycCore().CreateGUID(), stl::make_unique<CScriptGraphReceiveSignalNode>(m_signalId), pos);
			}

			// ~IMenuCommand

		private:

			SElementId m_signalId;
		};

	public:

		// IScriptGraphNodeCreator

		virtual SGUID GetTypeGUID() const override
		{
			return CScriptGraphReceiveSignalNode::ms_typeGUID;
		}

		virtual IScriptGraphNodePtr CreateNode(const SGUID& guid) override
		{
			return std::make_shared<CScriptGraphNode>(guid, stl::make_unique<CScriptGraphReceiveSignalNode>());
		}

		virtual void PopulateNodeCreationMenu(IScriptGraphNodeCreationMenu& nodeCreationMenu, const IScriptView& scriptView, const IScriptGraph& graph) override
		{
			switch (graph.GetType())
			{
			case EScriptGraphType::Signal:
				{
					auto visitEnvSignal = [&nodeCreationMenu, &scriptView](const IEnvSignal& envSignal) -> EVisitStatus
					{
						CStackString label;
						scriptView.QualifyName(envSignal, label);
						label.append("::ReceiveSignal");
						nodeCreationMenu.AddOption(label.c_str(), envSignal.GetDescription(), nullptr, std::make_shared<CNodeCreationMenuCommand>(SElementId(EDomain::Env, envSignal.GetGUID())));
						return EVisitStatus::Continue;
					};
					scriptView.VisitEnvSignals(EnvSignalConstVisitor::FromLambda(visitEnvSignal));

					auto visitScriptSignal = [&nodeCreationMenu, &scriptView](const IScriptSignal& scriptSignal) -> EVisitStatus
					{
						CStackString label;
						scriptView.QualifyName(scriptSignal, EDomainQualifier::Global, label);
						label.append("::ReceiveSignal");
						nodeCreationMenu.AddOption(label.c_str(), scriptSignal.GetDescription(), nullptr, std::make_shared<CNodeCreationMenuCommand>(SElementId(EDomain::Script, scriptSignal.GetGUID())));
						return EVisitStatus::Continue;
					};
					scriptView.VisitScriptSignals(ScriptSignalConstVisitor::FromLambda(visitScriptSignal), EDomainScope::Local);

					auto visitScriptTimer = [&nodeCreationMenu, &scriptView](const IScriptTimer& scriptTimer) -> EVisitStatus
					{
						CStackString label;
						scriptView.QualifyName(scriptTimer, EDomainQualifier::Global, label);
						label.append("::ReceiveSignal");
						nodeCreationMenu.AddOption(label.c_str(), scriptTimer.GetDescription(), nullptr, std::make_shared<CNodeCreationMenuCommand>(SElementId(EDomain::Script, scriptTimer.GetGUID())));
						return EVisitStatus::Continue;
					};
					scriptView.VisitScriptTimers(ScriptTimerConstVisitor::FromLambda(visitScriptTimer), EDomainScope::Local);

					break;
				}
			}
		}

		// ~IScriptGraphNodeCreator
	};

	factory.RegisterCreator(std::make_shared<CCreator>());
}

void CScriptGraphReceiveSignalNode::GoToSignal()
{
	CryLinkUtils::ExecuteCommand(CryLinkUtils::ECommand::Show, m_signalId.guid);
}

SRuntimeResult CScriptGraphReceiveSignalNode::Execute(SRuntimeContext& context, const SRuntimeActivationParams& activationParams)
{
	for (uint8 outputIdx = EOutputIdx::FirstParam, outputCount = context.node.GetOutputCount(); outputIdx < outputCount; ++outputIdx)
	{
		if (context.node.IsDataOutput(outputIdx))
		{
			CAnyConstPtr pSrcData = context.params.GetInput(context.node.GetOutputId(outputIdx).AsUniqueId());
			if (pSrcData)
			{
				Any::CopyAssign(*context.node.GetOutputData(outputIdx), *pSrcData);
			}
			else
			{
				return SRuntimeResult(ERuntimeStatus::Error);
			}
		}
	}

	return SRuntimeResult(ERuntimeStatus::Continue, EOutputIdx::Out);
}

const SGUID CScriptGraphReceiveSignalNode::ms_typeGUID = "ad2aee64-0a60-4469-8ec7-38b4b720d30c"_schematyc_guid;
} // Schematyc

SCHEMATYC_REGISTER_SCRIPT_GRAPH_NODE(Schematyc::CScriptGraphReceiveSignalNode::Register)
