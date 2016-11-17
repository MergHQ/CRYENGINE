// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Script/Graph/Nodes/ScriptGraphArrayAddNode.h"

#include <CrySerialization/Enum.h>
#include <Schematyc/Compiler/IGraphNodeCompiler.h>
#include <Schematyc/Env/Elements/IEnvDataType.h>
#include <Schematyc/Script/Elements/IScriptVariable.h>
#include <SerializationUtils/SerializationContext.h>
#include <Schematyc/Utils/Any.h>
#include <Schematyc/Utils/AnyArray.h>
#include <Schematyc/Utils/StackString.h>

#include "Script/ScriptView.h"
#include "Script/Graph/ScriptGraphNode.h"
#include "Script/Graph/ScriptGraphNodeFactory.h"
#include "SerializationUtils/SerializationContext.h"

SERIALIZATION_ENUM_BEGIN_NESTED2(Schematyc, CScriptGraphArrayAddNode, EReferenceMode, "Schematyc Script Graph Array Add Node Reference Mode")
SERIALIZATION_ENUM(Schematyc::CScriptGraphArrayAddNode::EReferenceMode::Input, "Input", "Input")
SERIALIZATION_ENUM(Schematyc::CScriptGraphArrayAddNode::EReferenceMode::Inline, "Inline", "Inline")
SERIALIZATION_ENUM_END()

namespace Schematyc
{
CScriptGraphArrayAddNode::CScriptGraphArrayAddNode(EReferenceMode referenceMode, const SElementId& reference)
	: m_referenceMode(referenceMode)
	, m_defaultValue(referenceMode == EReferenceMode::Input ? reference : SElementId())
{}

SGUID CScriptGraphArrayAddNode::GetTypeGUID() const
{
	return ms_typeGUID;
}

void CScriptGraphArrayAddNode::CreateLayout(CScriptGraphNodeLayout& layout)
{
	layout.SetName("Array::Add");
	layout.SetColor(EScriptGraphColor::Purple);

	layout.AddInput("In", SGUID(), { EScriptGraphPortFlags::Flow, EScriptGraphPortFlags::MultiLink });
	layout.AddOutput("Default", SGUID(), { EScriptGraphPortFlags::Flow, EScriptGraphPortFlags::SpacerBelow });

	if (!m_defaultValue.IsEmpty())
	{
		const SGUID typeGUID = m_defaultValue.GetTypeId().guid;
		if (m_referenceMode == EReferenceMode::Input)
		{
			layout.AddInput("Array", typeGUID, { EScriptGraphPortFlags::Data, EScriptGraphPortFlags::Array });
		}
		layout.AddInputWithData(m_defaultValue.GetTypeName(), typeGUID, { EScriptGraphPortFlags::Data, EScriptGraphPortFlags::Persistent, EScriptGraphPortFlags::Editable }, *m_defaultValue.GetValue());
	}
}

void CScriptGraphArrayAddNode::Compile(SCompilerContext& context, IGraphNodeCompiler& compiler) const
{
	compiler.BindCallback(&Execute);
}

void CScriptGraphArrayAddNode::LoadDependencies(Serialization::IArchive& archive, const ISerializationContext& context)
{
	m_defaultValue.SerializeTypeId(archive);
}

void CScriptGraphArrayAddNode::Load(Serialization::IArchive& archive, const ISerializationContext& context)
{
	//archive(m_referenceMode, "referenceMode");
}

void CScriptGraphArrayAddNode::Save(Serialization::IArchive& archive, const ISerializationContext& context)
{
	//archive(m_referenceMode, "referenceMode");
	m_defaultValue.SerializeTypeId(archive);
}

void CScriptGraphArrayAddNode::Edit(Serialization::IArchive& archive, const ISerializationContext& context)
{
	//archive(m_referenceMode, "referenceMode", "Reference Mode");
	switch (m_referenceMode)
	{
	case EReferenceMode::Input:
		{
			ScriptVariableData::CScopedSerializationConfig serializationConfig(archive);

			const SGUID guid = CScriptGraphNodeModel::GetNode().GetGraph().GetElement().GetGUID();
			serializationConfig.DeclareEnvDataTypes(guid);
			serializationConfig.DeclareScriptEnums(guid);
			serializationConfig.DeclareScriptStructs(guid);

			m_defaultValue.SerializeTypeId(archive);
			break;
		}
		/*case EReferenceMode::Inline:
		   {
		   SerializationUtils::CScopedQuickSearchConfig<SElementId> quickSearchConfig(archive, "Variable");

		   CScriptView scriptView(CScriptGraphNodeModel::GetNode().GetGraph().GetElement().GetGUID());

		   auto visitScriptVariable = [&quickSearchConfig](const IScriptVariable& scriptVariable) -> EVisitStatus
		   {
		   quickSearchConfig.AddOption(scriptVariable.GetName(), SElementId(EDomain::Script, scriptVariable.GetGUID()), scriptVariable.GetName(), nullptr);
		   return EVisitStatus::Continue;
		   };
		   scriptView.VisitScriptVariables(ScriptVariableConstVisitor::FromLambda(visitScriptVariable), EDomainScope::Local);

		   SElementId variableId;
		   archive(SerializationUtils::QuickSearch(variableId), "variableId", "Variable");
		   break;
		   }*/
	}
}

void CScriptGraphArrayAddNode::RemapDependencies(IGUIDRemapper& guidRemapper)
{
	m_defaultValue.RemapDependencies(guidRemapper);
}

void CScriptGraphArrayAddNode::Register(CScriptGraphNodeFactory& factory)
{
	class CCreator : public IScriptGraphNodeCreator
	{
	private:

		class CNodeCreationMenuCommand : public IScriptGraphNodeCreationMenuCommand
		{
		public:

			inline CNodeCreationMenuCommand(EReferenceMode referenceMode = EReferenceMode::Input, const SElementId& reference = SElementId())
				: m_referenceMode(referenceMode)
				, m_reference(reference)
			{}

			// IMenuCommand

			IScriptGraphNodePtr Execute(const Vec2& pos)
			{
				return std::make_shared<CScriptGraphNode>(GetSchematycCore().CreateGUID(), stl::make_unique<CScriptGraphArrayAddNode>(m_referenceMode, m_reference), pos);
			}

			// ~IMenuCommand

		private:

			EReferenceMode m_referenceMode;
			SElementId     m_reference;
		};

	public:

		// IScriptGraphNodeCreator

		virtual SGUID GetTypeGUID() const override
		{
			return CScriptGraphArrayAddNode::ms_typeGUID;
		}

		virtual IScriptGraphNodePtr CreateNode(const SGUID& guid) override
		{
			return std::make_shared<CScriptGraphNode>(guid, stl::make_unique<CScriptGraphArrayAddNode>());
		}

		virtual void PopulateNodeCreationMenu(IScriptGraphNodeCreationMenu& nodeCreationMenu, const IScriptView& scriptView, const IScriptGraph& graph) override
		{
			const char* szLabel = "Array::Add";
			const char* szDescription = "Add element to end of array";

			nodeCreationMenu.AddOption(szLabel, szDescription, nullptr, std::make_shared<CNodeCreationMenuCommand>());

			// #SchematycTODO : This code is duplicated in all array nodes, find a way to reduce the duplication.

			auto visitEnvDataType = [&nodeCreationMenu, &scriptView, szLabel, szDescription](const IEnvDataType& envDataType) -> EVisitStatus
			{
				CStackString label;
				scriptView.QualifyName(envDataType, label);
				label.append("::");
				label.append(szLabel);
				nodeCreationMenu.AddOption(label.c_str(), szDescription, nullptr, std::make_shared<CNodeCreationMenuCommand>(EReferenceMode::Input, SElementId(EDomain::Env, envDataType.GetGUID())));
				return EVisitStatus::Continue;
			};
			scriptView.VisitEnvDataTypes(EnvDataTypeConstVisitor::FromLambda(visitEnvDataType));

			/*auto visitScriptVariable = [&nodeCreationMenu, &scriptView, szLabel, szDescription](const IScriptVariable& scriptVariable) -> EVisitStatus
			   {
			   if (scriptVariable.IsArray())
			   {
			    CStackString label;
			    scriptView.QualifyName(scriptVariable, EDomainQualifier::Global, label);
			    label.append("::");
			    label.append(szLabel);
			    nodeCreationMenu.AddOption(label.c_str(), szDescription, nullptr, std::make_shared<CNodeCreationMenuCommand>(?));
			   }
			   return EVisitStatus::Continue;
			   };
			   scriptView.VisitScriptVariables(ScriptVariableConstVisitor::FromLambda(visitScriptVariable), EDomainScope::Local);*/
		}

		// ~IScriptGraphNodeCreator
	};

	factory.RegisterCreator(std::make_shared<CCreator>());
}

SRuntimeResult CScriptGraphArrayAddNode::Execute(SRuntimeContext& context, const SRuntimeActivationParams& activationParams)
{
	CAnyArrayPtr pArray = DynamicCast<CAnyArrayPtr>(*context.node.GetInputData(EInputIdx::Array));
	CAnyConstRef value = *context.node.GetInputData(EInputIdx::Value);

	pArray->PushBack(value);

	return SRuntimeResult(ERuntimeStatus::Continue, EOutputIdx::Out);
}

const SGUID CScriptGraphArrayAddNode::ms_typeGUID = "02368e7b-7939-495e-bf65-f044c440f4f3"_schematyc_guid;
} // Schematyc

SCHEMATYC_REGISTER_SCRIPT_GRAPH_NODE(Schematyc::CScriptGraphArrayAddNode::Register)
