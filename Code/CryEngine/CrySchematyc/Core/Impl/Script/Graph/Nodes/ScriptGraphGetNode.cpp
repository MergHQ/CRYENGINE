// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Script/Graph/Nodes/ScriptGraphGetNode.h"

#include <Schematyc/Compiler/CompilerContext.h>
#include <Schematyc/Compiler/IGraphNodeCompiler.h>
#include <Schematyc/Script/Elements/IScriptVariable.h>
#include <Schematyc/Utils/Any.h>
#include <Schematyc/Utils/AnyArray.h>
#include <Schematyc/Utils/IGUIDRemapper.h>
#include <Schematyc/Utils/StackString.h>

#include "Object.h"
#include "Runtime/RuntimeClass.h"
#include "Script/ScriptView.h"
#include "Script/Graph/ScriptGraphNode.h"
#include "Script/Graph/ScriptGraphNodeFactory.h"
#include "SerializationUtils/SerializationContext.h"

namespace Schematyc
{
CScriptGraphGetNode::SRuntimeData::SRuntimeData(uint32 _pos)
	: pos(_pos)
{}

CScriptGraphGetNode::SRuntimeData::SRuntimeData(const SRuntimeData& rhs)
	: pos(rhs.pos)
{}

SGUID CScriptGraphGetNode::SRuntimeData::ReflectSchematycType(CTypeInfo<CScriptGraphGetNode::SRuntimeData>& typeInfo)
{
	return "be778830-e855-42d3-a87f-424161017339"_schematyc_guid;
}

CScriptGraphGetNode::CScriptGraphGetNode() {}

CScriptGraphGetNode::CScriptGraphGetNode(const SGUID& referenceGUID)
	: m_referenceGUID(referenceGUID)
{}

SGUID CScriptGraphGetNode::GetTypeGUID() const
{
	return ms_typeGUID;
}

void CScriptGraphGetNode::CreateLayout(CScriptGraphNodeLayout& layout)
{
	layout.SetColor(EScriptGraphColor::Blue);

	if (!GUID::IsEmpty(m_referenceGUID))
	{
		CScriptView scriptView(CScriptGraphNodeModel::GetNode().GetGraph().GetElement().GetGUID());
		const IScriptElement* pReferenceElement = scriptView.GetScriptElement(m_referenceGUID);
		if (pReferenceElement)
		{
			switch (pReferenceElement->GetElementType())
			{
			case EScriptElementType::Variable:
				{
					const IScriptVariable& variable = DynamicCast<IScriptVariable>(*pReferenceElement);
					CAnyConstPtr pData = variable.GetData();
					if (pData)
					{
						CStackString name = "Get ";
						name.append(variable.GetName());
						layout.SetName(name.c_str());

						if (!variable.IsArray())
						{
							layout.AddOutputWithData("Value", variable.GetTypeId().guid, { EScriptGraphPortFlags::Data, EScriptGraphPortFlags::MultiLink, EScriptGraphPortFlags::Pull }, *pData);
						}
						else
						{
							layout.AddOutputWithData("Value", variable.GetTypeId().guid, { EScriptGraphPortFlags::Data, EScriptGraphPortFlags::MultiLink, EScriptGraphPortFlags::Array, EScriptGraphPortFlags::Pull }, CAnyArrayPtr());
						}
					}
					break;
				}
			}
		}
	}
}

void CScriptGraphGetNode::Compile(SCompilerContext& context, IGraphNodeCompiler& compiler) const
{
	if (!GUID::IsEmpty(m_referenceGUID))
	{
		CScriptView scriptView(CScriptGraphNodeModel::GetNode().GetGraph().GetElement().GetGUID());
		const IScriptElement* pReferenceElement = scriptView.GetScriptElement(m_referenceGUID);
		if (pReferenceElement)
		{
			switch (pReferenceElement->GetElementType())
			{
			case EScriptElementType::Variable:
				{
					const IScriptVariable& variable = DynamicCast<IScriptVariable>(*pReferenceElement);
					const CRuntimeClass* pClass = context.interfaces.Query<const CRuntimeClass>();
					if (pClass)
					{
						const uint32 variablePos = pClass->GetVariablePos(variable.GetGUID());
						if (variablePos != InvalidIdx)
						{
							if (!variable.IsArray())
							{
								compiler.BindCallback(&Execute);
							}
							else
							{
								compiler.BindCallback(&ExecuteArray);
							}
							compiler.BindData(SRuntimeData(variablePos));
						}
					}
					break;
				}
			}
		}
	}
}

void CScriptGraphGetNode::LoadDependencies(Serialization::IArchive& archive, const ISerializationContext& context)
{
	archive(m_referenceGUID, "referenceGUID");
}

void CScriptGraphGetNode::Save(Serialization::IArchive& archive, const ISerializationContext& context)
{
	archive(m_referenceGUID, "referenceGUID");
}

void CScriptGraphGetNode::Validate(Serialization::IArchive& archive, const ISerializationContext& context)
{
	if (!GUID::IsEmpty(m_referenceGUID))
	{
		CScriptView scriptView(CScriptGraphNodeModel::GetNode().GetGraph().GetElement().GetGUID());
		const IScriptElement* pReferenceElement = scriptView.GetScriptElement(m_referenceGUID);
		if (!pReferenceElement)
		{
			archive.error(*this, "Failed to retrieve reference!");
		}
	}
}

void CScriptGraphGetNode::RemapDependencies(IGUIDRemapper& guidRemapper)
{
	m_referenceGUID = guidRemapper.Remap(m_referenceGUID);
}

void CScriptGraphGetNode::Register(CScriptGraphNodeFactory& factory)
{
	class CCreator : public IScriptGraphNodeCreator
	{
	private:

		class CNodeCreationMenuCommand : public IScriptGraphNodeCreationMenuCommand
		{
		public:

			CNodeCreationMenuCommand(const SGUID& referenceGUID)
				: m_referenceGUID(referenceGUID)
			{}

			// IMenuCommand

			IScriptGraphNodePtr Execute(const Vec2& pos)
			{
				return std::make_shared<CScriptGraphNode>(GetSchematycCore().CreateGUID(), stl::make_unique<CScriptGraphGetNode>(m_referenceGUID), pos);
			}

			// ~IMenuCommand

		private:

			SGUID m_referenceGUID;
		};

	public:

		// IScriptGraphNodeCreator

		virtual SGUID GetTypeGUID() const override
		{
			return CScriptGraphGetNode::ms_typeGUID;
		}

		virtual IScriptGraphNodePtr CreateNode(const SGUID& guid) override
		{
			return std::make_shared<CScriptGraphNode>(guid, stl::make_unique<CScriptGraphGetNode>());
		}

		virtual void PopulateNodeCreationMenu(IScriptGraphNodeCreationMenu& nodeCreationMenu, const IScriptView& scriptView, const IScriptGraph& graph) override
		{
			auto visitScriptVariable = [&nodeCreationMenu, &scriptView](const IScriptVariable& variable) -> EVisitStatus
			{
				CStackString label;
				scriptView.QualifyName(variable, EDomainQualifier::Global, label);
				label.append("::Get");
				nodeCreationMenu.AddOption(label.c_str(), "Get value of variable", "", std::make_shared<CNodeCreationMenuCommand>(variable.GetGUID()));
				return EVisitStatus::Continue;
			};
			scriptView.VisitScriptVariables(ScriptVariableConstVisitor::FromLambda(visitScriptVariable), EDomainScope::Derived);
		}

		// ~IScriptGraphNodeCreator
	};

	factory.RegisterCreator(std::make_shared<CCreator>());
}

SRuntimeResult CScriptGraphGetNode::Execute(SRuntimeContext& context, const SRuntimeActivationParams& activationParams)
{
	const SRuntimeData& data = DynamicCast<SRuntimeData>(*context.node.GetData());
	const CAnyPtr pVariable = static_cast<CObject*>(context.pObject)->GetScratchpad().Get(data.pos);
	Any::CopyAssign(*context.node.GetOutputData(EOutputIdx::Value), *pVariable);

	return SRuntimeResult(ERuntimeStatus::Continue);
}

SRuntimeResult CScriptGraphGetNode::ExecuteArray(SRuntimeContext& context, const SRuntimeActivationParams& activationParams)
{
	const SRuntimeData& data = DynamicCast<SRuntimeData>(*context.node.GetData());
	CAnyArray* pArray = DynamicCast<CAnyArray>(static_cast<CObject*>(context.pObject)->GetScratchpad().Get(data.pos));
	Any::CopyAssign(*context.node.GetOutputData(EOutputIdx::Value), CAnyArrayPtr(pArray));

	return SRuntimeResult(ERuntimeStatus::Continue);
}

const SGUID CScriptGraphGetNode::ms_typeGUID = "a15c2533-d004-4266-b121-68404aa5e02f"_schematyc_guid;
} // Schematyc

SCHEMATYC_REGISTER_SCRIPT_GRAPH_NODE(Schematyc::CScriptGraphGetNode::Register)
