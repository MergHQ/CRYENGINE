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
	layout.SetStyleId("Core::ReceiveSignal");
	layout.SetColor(EScriptGraphColor::Green);

	const char* szSubject = nullptr;
	if (!GUID::IsEmpty(m_signalId.guid))
	{
		switch (m_signalId.domain)
		{
		case EDomain::Env:
			{
				const IEnvSignal* pEnvSignal = gEnv->pSchematyc->GetEnvRegistry().GetSignal(m_signalId.guid);
				if (pEnvSignal)
				{
					szSubject = pEnvSignal->GetName();

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
				const IScriptElement* pScriptElement = gEnv->pSchematyc->GetScriptRegistry().GetElement(m_signalId.guid);
				if (pScriptElement)
				{
					switch (pScriptElement->GetElementType())
					{
					case EScriptElementType::Signal:
						{
							szSubject = pScriptElement->GetName();

							layout.AddOutput("Out", m_signalId.guid, EScriptGraphPortFlags::Signal);
							break;
						}
					case EScriptElementType::Timer:
						{
							szSubject = pScriptElement->GetName();

							layout.AddOutput("Out", m_signalId.guid, EScriptGraphPortFlags::Signal);
							break;
						}
					}
				}
				break;
			}
		}
	}
	layout.SetName("Receive Signal", szSubject);
}

void CScriptGraphReceiveSignalNode::Compile(SCompilerContext& context, IGraphNodeCompiler& compiler) const
{
	CRuntimeClass* pClass = context.interfaces.Query<CRuntimeClass>();
	if (pClass)
	{
		const IScriptElement* pOwner = CScriptGraphNodeModel::GetNode().GetGraph().GetElement().GetParent();
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
				const IEnvSignal* pEnvSignal = gEnv->pSchematyc->GetEnvRegistry().GetSignal(m_signalId.guid);
				if (!pEnvSignal)
				{
					archive.error(*this, "Unable to retrieve environment signal!");
				}
				break;
			}
		case EDomain::Script:
			{
				const IScriptElement* pScriptElement = gEnv->pSchematyc->GetScriptRegistry().GetElement(m_signalId.guid);
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

		class CCreationCommand : public IScriptGraphNodeCreationCommand
		{
		public:

			CCreationCommand(const char* szSubject, const SElementId& signalId)
				: m_subject(szSubject)
				, m_signalId(signalId)
			{}

			// IScriptGraphNodeCreationCommand

			virtual const char* GetBehavior() const override
			{
				return "Signal::Receive";
			}

			virtual const char* GetSubject() const override
			{
				return m_subject.c_str();
			}

			virtual const char* GetDescription() const override
			{
				return "Receive signal";
			}

			virtual const char* GetStyleId() const override
			{
				return "Core::ReceiveSignal";
			}

			virtual IScriptGraphNodePtr Execute(const Vec2& pos) override
			{
				return std::make_shared<CScriptGraphNode>(gEnv->pSchematyc->CreateGUID(), stl::make_unique<CScriptGraphReceiveSignalNode>(m_signalId), pos);
			}

			// ~IScriptGraphNodeCreationCommand

		private:

			string     m_subject;
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
						CStackString subject;
						scriptView.QualifyName(envSignal, subject);
						nodeCreationMenu.AddCommand(std::make_shared<CCreationCommand>(subject.c_str(), SElementId(EDomain::Env, envSignal.GetGUID())));
						return EVisitStatus::Continue;
					};
					scriptView.VisitEnvSignals(EnvSignalConstVisitor::FromLambda(visitEnvSignal));

					auto visitScriptSignal = [&nodeCreationMenu, &scriptView](const IScriptSignal& scriptSignal)
					{
						CStackString subject;
						scriptView.QualifyName(scriptSignal, EDomainQualifier::Global, subject);
						nodeCreationMenu.AddCommand(std::make_shared<CCreationCommand>(subject.c_str(), SElementId(EDomain::Script, scriptSignal.GetGUID())));
					};
					scriptView.VisitAccesibleSignals(ScriptSignalConstVisitor::FromLambda(visitScriptSignal));

					auto visitScriptTimer = [&nodeCreationMenu, &scriptView](const IScriptTimer& scriptTimer)
					{
						CStackString subject;
						scriptView.QualifyName(scriptTimer, EDomainQualifier::Global, subject);
						nodeCreationMenu.AddCommand(std::make_shared<CCreationCommand>(subject.c_str(), SElementId(EDomain::Script, scriptTimer.GetGUID())));
					};
					scriptView.VisitAccesibleTimers(ScriptTimerConstVisitor::FromLambda(visitScriptTimer));

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
