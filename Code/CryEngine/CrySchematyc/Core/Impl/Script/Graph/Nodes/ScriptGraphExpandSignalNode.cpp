// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Script/Graph/Nodes/ScriptGraphExpandSignalNode.h"

#include <CrySerialization/Decorators/ActionButton.h>
#include <Schematyc/IObject.h>
#include <Schematyc/Compiler/CompilerContext.h>
#include <Schematyc/Compiler/IGraphNodeCompiler.h>
#include <Schematyc/Env/IEnvRegistry.h>
#include <Schematyc/Env/Elements/IEnvSignal.h>
#include <Schematyc/Script/IScriptRegistry.h>
#include <Schematyc/Utils/Any.h>
#include <Schematyc/Utils/IGUIDRemapper.h>
#include <Schematyc/Utils/StackString.h>

#include "Runtime/RuntimeClass.h"
#include "Script/ScriptView.h"
#include "Script/Graph/ScriptGraphNode.h"
#include "Script/Graph/ScriptGraphNodeFactory.h"
#include "SerializationUtils/SerializationContext.h"

namespace Schematyc
{
CScriptGraphExpandSignalNode::CScriptGraphExpandSignalNode() {}

CScriptGraphExpandSignalNode::CScriptGraphExpandSignalNode(const SElementId& typeId)
	: m_typeId(typeId)
{}

SGUID CScriptGraphExpandSignalNode::GetTypeGUID() const
{
	return ms_typeGUID;
}

void CScriptGraphExpandSignalNode::CreateLayout(CScriptGraphNodeLayout& layout)
{
	layout.SetColor(EScriptGraphColor::Blue);

	if (!GUID::IsEmpty(m_typeId.guid))
	{
		switch (m_typeId.domain)
		{
		case EDomain::Env:
			{
				const IEnvSignal* pEnvSignal = GetSchematycCore().GetEnvRegistry().GetSignal(m_typeId.guid);
				if (pEnvSignal)
				{
					const IScriptElement& element = CScriptGraphNodeModel::GetNode().GetGraph().GetElement();
					CScriptView scriptView(element.GetGUID());

					CStackString name;
					scriptView.QualifyName(*pEnvSignal, name);
					name.insert(0, "Expand ");
					layout.SetName(name.c_str());

					layout.AddInput("In", m_typeId.guid, EScriptGraphPortFlags::Signal);
					layout.AddOutput("Out", SGUID(), { EScriptGraphPortFlags::Flow, EScriptGraphPortFlags::Begin });

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
		}
	}
}

void CScriptGraphExpandSignalNode::Compile(SCompilerContext& context, IGraphNodeCompiler& compiler) const
{
	switch (m_typeId.domain)
	{
	case EDomain::Env:
		{
			const IEnvSignal* pEnvSignal = GetSchematycCore().GetEnvRegistry().GetSignal(m_typeId.guid);
			if (pEnvSignal)
			{
				compiler.BindCallback(&Execute);
			}
			break;
		}
	}
}

void CScriptGraphExpandSignalNode::LoadDependencies(Serialization::IArchive& archive, const ISerializationContext& context)
{
	archive(m_typeId, "typeId");
}

void CScriptGraphExpandSignalNode::Save(Serialization::IArchive& archive, const ISerializationContext& context)
{
	archive(m_typeId, "typeId");
}

void CScriptGraphExpandSignalNode::Edit(Serialization::IArchive& archive, const ISerializationContext& context)
{
	archive(Serialization::ActionButton(functor(*this, &CScriptGraphExpandSignalNode::GoToType)), "goToType", "^Go To Type");

	Validate(archive, context);
}

void CScriptGraphExpandSignalNode::Validate(Serialization::IArchive& archive, const ISerializationContext& context)
{
	if (!GUID::IsEmpty(m_typeId.guid))
	{
		switch (m_typeId.domain)
		{
		case EDomain::Env:
			{
				const IEnvElement* pEnvElement = GetSchematycCore().GetEnvRegistry().GetElement(m_typeId.guid);
				if (!pEnvElement)
				{
					archive.error(*this, "Unable to retrieve environment type!");
				}
				break;
			}
		}
	}
}

void CScriptGraphExpandSignalNode::RemapDependencies(IGUIDRemapper& guidRemapper)
{
	if (m_typeId.domain == EDomain::Script)
	{
		m_typeId.guid = guidRemapper.Remap(m_typeId.guid);
	}
}

void CScriptGraphExpandSignalNode::Register(CScriptGraphNodeFactory& factory)
{
	class CCreator : public IScriptGraphNodeCreator
	{
	private:

		class CNodeCreationMenuCommand : public IScriptGraphNodeCreationMenuCommand
		{
		public:

			CNodeCreationMenuCommand(const SElementId& typeId)
				: m_typeId(typeId)
			{}

			// IMenuCommand

			IScriptGraphNodePtr Execute(const Vec2& pos)
			{
				return std::make_shared<CScriptGraphNode>(GetSchematycCore().CreateGUID(), stl::make_unique<CScriptGraphExpandSignalNode>(m_typeId), pos);
			}

			// ~IMenuCommand

		private:

			SElementId m_typeId;
		};

	public:

		// IScriptGraphNodeCreator

		virtual SGUID GetTypeGUID() const override
		{
			return CScriptGraphExpandSignalNode::ms_typeGUID;
		}

		virtual IScriptGraphNodePtr CreateNode(const SGUID& guid) override
		{
			return std::make_shared<CScriptGraphNode>(guid, stl::make_unique<CScriptGraphExpandSignalNode>());
		}

		virtual void PopulateNodeCreationMenu(IScriptGraphNodeCreationMenu& nodeCreationMenu, const IScriptView& scriptView, const IScriptGraph& graph) override
		{
			switch (graph.GetType())
			{
			case EScriptGraphType::Transition:
				{
					auto visitEnvSignal = [&nodeCreationMenu, &scriptView](const IEnvSignal& envSignal) -> EVisitStatus
					{
						CStackString label;
						scriptView.QualifyName(envSignal, label);
						label.append("::ExpandSignal");
						nodeCreationMenu.AddOption(label.c_str(), envSignal.GetDescription(), nullptr, std::make_shared<CNodeCreationMenuCommand>(SElementId(EDomain::Env, envSignal.GetGUID())));
						return EVisitStatus::Continue;
					};
					scriptView.VisitEnvSignals(EnvSignalConstVisitor::FromLambda(visitEnvSignal));

					break;
				}
			}
		}

		// ~IScriptGraphNodeCreator
	};

	factory.RegisterCreator(std::make_shared<CCreator>());
}

void CScriptGraphExpandSignalNode::GoToType()
{
	CryLinkUtils::ExecuteCommand(CryLinkUtils::ECommand::Show, m_typeId.guid);
}

SRuntimeResult CScriptGraphExpandSignalNode::Execute(SRuntimeContext& context, const SRuntimeActivationParams& activationParams)
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

const SGUID CScriptGraphExpandSignalNode::ms_typeGUID = "f4b52ef8-18ec-4f82-bf61-42429b85ebf6"_schematyc_guid;
} // Schematyc

SCHEMATYC_REGISTER_SCRIPT_GRAPH_NODE(Schematyc::CScriptGraphExpandSignalNode::Register)
