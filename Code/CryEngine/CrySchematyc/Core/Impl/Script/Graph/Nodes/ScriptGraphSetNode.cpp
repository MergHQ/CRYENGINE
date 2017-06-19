// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Script/Graph/Nodes/ScriptGraphSetNode.h"

#include <CrySchematyc/Compiler/CompilerContext.h>
#include <CrySchematyc/Compiler/IGraphNodeCompiler.h>
#include <CrySchematyc/Script/Elements/IScriptVariable.h>
#include <CrySchematyc/Script/Elements/IScriptComponentInstance.h>
#include <CrySchematyc/Utils/Any.h>
#include <CrySchematyc/Utils/IGUIDRemapper.h>
#include <CrySchematyc/Utils/StackString.h>

#include "Object.h"
#include "Runtime/RuntimeClass.h"
#include "Script/ScriptView.h"
#include "Script/Graph/ScriptGraphNode.h"
#include "Script/Graph/ScriptGraphNodeFactory.h"
#include "SerializationUtils/SerializationContext.h"

namespace Schematyc
{
CScriptGraphSetNode::SRuntimeData::SRuntimeData(uint32 _pos)
	: pos(_pos)
{}

CScriptGraphSetNode::SRuntimeData::SRuntimeData(const SRuntimeData& rhs)
	: pos(rhs.pos)
{}

void CScriptGraphSetNode::SRuntimeData::ReflectType(CTypeDesc<CScriptGraphSetNode::SRuntimeData>& desc)
{
	desc.SetGUID("1a4b1431-c8fe-46f5-aecd-fc8c11500e99"_cry_guid);
}

CScriptGraphSetNode::CScriptGraphSetNode() {}

CScriptGraphSetNode::CScriptGraphSetNode(const CryGUID& referenceGUID,uint32 componentMemberId)
	: m_referenceGUID(referenceGUID)
	, m_componentMemberId(componentMemberId)
{}

CryGUID CScriptGraphSetNode::GetTypeGUID() const
{
	return ms_typeGUID;
}

void CScriptGraphSetNode::CreateLayout(CScriptGraphNodeLayout& layout)
{
	layout.SetStyleId("Core::Data");

	CStackString subject;
	if (!GUID::IsEmpty(m_referenceGUID))
	{
		layout.AddInput("In", CryGUID(), { EScriptGraphPortFlags::Flow, EScriptGraphPortFlags::MultiLink });
		layout.AddOutput("Out", CryGUID(), EScriptGraphPortFlags::Flow);

		CScriptView scriptView(CScriptGraphNodeModel::GetNode().GetGraph().GetElement().GetGUID());
		const IScriptElement* pReferenceElement = scriptView.GetScriptElement(m_referenceGUID);
		if (pReferenceElement)
		{
			switch (pReferenceElement->GetType())
			{
			case EScriptElementType::Variable:
				{
					const IScriptVariable& variable = DynamicCast<IScriptVariable>(*pReferenceElement);
					CAnyConstPtr pData = variable.GetData();
					if (pData)
					{
						subject = variable.GetName();

						layout.AddInputWithData("Value", variable.GetTypeId().guid, { EScriptGraphPortFlags::Data, EScriptGraphPortFlags::MultiLink, EScriptGraphPortFlags::Persistent, EScriptGraphPortFlags::Editable }, *pData);
					}
					break;
				}
			case EScriptElementType::ComponentInstance:
				{
					const IScriptComponentInstance& scriptComponentInstance = DynamicCast<IScriptComponentInstance>(*pReferenceElement);
					if (scriptComponentInstance.GetProperties().GetTypeDesc())
					{
						const CClassMemberDesc* pMember = scriptComponentInstance.GetProperties().GetTypeDesc()->FindMemberById(m_componentMemberId);
						if (pMember)
						{
							CAnyConstPtr pData(pMember->GetTypeDesc(), pMember->GetDefaultValue());
							if (pData)
							{
								subject = scriptComponentInstance.GetName();
								subject += ".";
								subject += pMember->GetLabel();

								layout.AddInputWithData("Value", pData->GetTypeDesc().GetGUID(), { EScriptGraphPortFlags::Data, EScriptGraphPortFlags::MultiLink, EScriptGraphPortFlags::Persistent, EScriptGraphPortFlags::Editable }, *pData);
							}
						}
					}
					break;
			}

			}
		}
	}
	layout.SetName("Set", subject.c_str());
}

void CScriptGraphSetNode::Compile(SCompilerContext& context, IGraphNodeCompiler& compiler) const
{
	if (!GUID::IsEmpty(m_referenceGUID))
	{
		CScriptView scriptView(CScriptGraphNodeModel::GetNode().GetGraph().GetElement().GetGUID());
		const IScriptElement* pReferenceElement = scriptView.GetScriptElement(m_referenceGUID);
		if (pReferenceElement)
		{
			switch (pReferenceElement->GetType())
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
							compiler.BindCallback(&Execute);
							compiler.BindData(SRuntimeData(variablePos));
						}
					}
					break;
				}
			case EScriptElementType::ComponentInstance:
				{
					const IScriptComponentInstance& scriptComponentInstance = DynamicCast<IScriptComponentInstance>(*pReferenceElement);
					const CRuntimeClass* pClass = context.interfaces.Query<const CRuntimeClass>();
					if (pClass && scriptComponentInstance.GetProperties().GetTypeDesc())
					{
						SComponentPropertyRuntimeData runtimeData;
						runtimeData.componentMemberIndex = scriptComponentInstance.GetProperties().GetTypeDesc()->FindMemberIndexById(m_componentMemberId);
						runtimeData.componentIdx = pClass->FindComponentInstance(m_referenceGUID);
						if (runtimeData.componentIdx != InvalidIdx && runtimeData.componentMemberIndex != InvalidIdx)
						{
							compiler.BindCallback(&ExecuteSetComponentProperty);
							compiler.BindData(runtimeData);
						}
					}
					break;
				}
			}
		}
	}
}

void CScriptGraphSetNode::LoadDependencies(Serialization::IArchive& archive, const ISerializationContext& context)
{
	archive(m_referenceGUID, "referenceGUID");
	archive(m_componentMemberId,"memberId");
}

void CScriptGraphSetNode::Save(Serialization::IArchive& archive, const ISerializationContext& context)
{
	archive(m_referenceGUID, "referenceGUID");
	archive(m_componentMemberId,"memberId");
}

void CScriptGraphSetNode::Validate(Serialization::IArchive& archive, const ISerializationContext& context)
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

void CScriptGraphSetNode::RemapDependencies(IGUIDRemapper& guidRemapper)
{
	m_referenceGUID = guidRemapper.Remap(m_referenceGUID);
}

void CScriptGraphSetNode::Register(CScriptGraphNodeFactory& factory)
{
	class CCreator : public IScriptGraphNodeCreator
	{
	private:

		class CCreationCommand : public IScriptGraphNodeCreationCommand
		{
		public:

			CCreationCommand(const char* szSubject, const CryGUID& referenceGUID,uint32  componentMemberId = 0)
				: m_subject(szSubject)
				, m_referenceGUID(referenceGUID)
				, m_componentMemberId(componentMemberId)
			{}

			// IScriptGraphNodeCreationCommand

			virtual const char* GetBehavior() const override
			{
				return "Set";
			}

			virtual const char* GetSubject() const override
			{
				return m_subject.c_str();
			}

			virtual const char* GetDescription() const override
			{
				if (m_componentMemberId)
					return "Set value of the component property";
				return "Set value of variable";
			}

			virtual const char* GetStyleId() const override
			{
				return "Core::Data";
			}

			virtual IScriptGraphNodePtr Execute(const Vec2& pos) override
			{
				return std::make_shared<CScriptGraphNode>(gEnv->pSchematyc->CreateGUID(), stl::make_unique<CScriptGraphSetNode>(m_referenceGUID,m_componentMemberId), pos);
			}

			// ~IScriptGraphNodeCreationCommand

		private:

			string m_subject;
			CryGUID  m_referenceGUID;
			uint32  m_componentMemberId = 0;
		};

	public:

		// IScriptGraphNodeCreator

		virtual CryGUID GetTypeGUID() const override
		{
			return CScriptGraphSetNode::ms_typeGUID;
		}

		virtual IScriptGraphNodePtr CreateNode(const CryGUID& guid) override
		{
			return std::make_shared<CScriptGraphNode>(guid, stl::make_unique<CScriptGraphSetNode>());
		}

		virtual void PopulateNodeCreationMenu(IScriptGraphNodeCreationMenu& nodeCreationMenu, const IScriptView& scriptView, const IScriptGraph& graph) override
		{
			switch (graph.GetType())
			{
			case EScriptGraphType::Construction:
			case EScriptGraphType::Signal:
			case EScriptGraphType::Function:
				{
					auto visitScriptVariable = [&nodeCreationMenu, &scriptView](const IScriptVariable& variable) -> EVisitStatus
					{
						if (!variable.IsArray())
						{
							CStackString subject;
							scriptView.QualifyName(variable, EDomainQualifier::Global, subject);
							nodeCreationMenu.AddCommand(std::make_shared<CCreationCommand>(subject.c_str(), variable.GetGUID()));
						}
						return EVisitStatus::Continue;
					};
					scriptView.VisitScriptVariables(visitScriptVariable, EDomainScope::Derived);

					// Library variables
					// TODO: Not yet supported.
					/*CScriptView gloablView(gEnv->pSchematyc->GetScriptRegistry().GetRootElement().GetGUID());
					   auto visitLibraries = [&nodeCreationMenu](const IScriptVariable& scriptVariable) -> EVisitStatus
					   {
					   if (!scriptVariable.IsArray())
					   {
					    CStackString subject;
					    QualifyScriptElementName(gEnv->pSchematyc->GetScriptRegistry().GetRootElement(), scriptVariable, EDomainQualifier::Global, subject);
					    nodeCreationMenu.AddCommand(std::make_shared<CCreationCommand>(subject.c_str(), scriptVariable.GetGUID()));
					   }
					   return EVisitStatus::Continue;
					   };
					   gloablView.VisitScriptModuleVariables(visitLibraries);*/
					// ~TODO

								// Populate component properties
					auto visitScriptComponentInstance = [&nodeCreationMenu, &scriptView](const IScriptComponentInstance& scriptComponentInstance) -> EVisitStatus
					{
						CStackString baseName;
						baseName = "Components::";
						baseName += scriptComponentInstance.GetName();
						baseName += "::";

						const CClassDesc* pClassDesc = scriptComponentInstance.GetProperties().GetTypeDesc();
						if (!pClassDesc)
							return EVisitStatus::Continue;

						const CClassMemberDescArray &members = pClassDesc->GetMembers();
						for (const CClassMemberDesc& member : members)
						{
							CStackString name(baseName);
							name += member.GetLabel();
							uint32 memberId = member.GetId();
							nodeCreationMenu.AddCommand(std::make_shared<CCreationCommand>(name.c_str(), scriptComponentInstance.GetGUID(), memberId));
						}
						return EVisitStatus::Continue;
					};
					scriptView.VisitScriptComponentInstances(visitScriptComponentInstance, EDomainScope::All);

					break;
				}
			}
		}

		// ~IScriptGraphNodeCreator
	};

	factory.RegisterCreator(std::make_shared<CCreator>());
}

SRuntimeResult CScriptGraphSetNode::Execute(SRuntimeContext& context, const SRuntimeActivationParams& activationParams)
{
	const SRuntimeData& data = DynamicCast<SRuntimeData>(*context.node.GetData());
	Any::CopyAssign(*static_cast<CObject*>(context.pObject)->GetScratchpad().Get(data.pos), *context.node.GetInputData(EInputIdx::Value));

	return SRuntimeResult(ERuntimeStatus::Continue, EOutputIdx::Out);
}

SRuntimeResult CScriptGraphSetNode::ExecuteSetComponentProperty(SRuntimeContext& context, const SRuntimeActivationParams& activationParams)
{
	const SComponentPropertyRuntimeData& runtimeData = DynamicCast<SComponentPropertyRuntimeData>(*context.node.GetData());

	IEntityComponent* pComponent = static_cast<CObject*>(context.pObject)->GetComponent(runtimeData.componentIdx);
	if (pComponent)
	{
		const CClassMemberDescArray &members = pComponent->GetClassDesc().GetMembers();
		assert(runtimeData.componentMemberIndex < members.size());
		if (runtimeData.componentMemberIndex < members.size())
		{
			const CClassMemberDesc& member = members[runtimeData.componentMemberIndex];

			// Pointer to the member of the component
			CAnyRef memberAny(member.GetTypeDesc(), (void*)(reinterpret_cast<uint8*>(pComponent) + member.GetOffset()));

			// Assign component member to the Graph data output
			Any::CopyAssign(memberAny,*context.node.GetInputData(EInputIdx::Value));

			if (pComponent->GetEntity())
			{
				// Should we only send this event to the changed component as an optimization!?
				SEntityEvent event(ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED);
				event.nParam[0] = reinterpret_cast<uintptr_t>(pComponent);
				event.nParam[1] = member.GetId();
				//@TODO Decide later, if this event should be sent to whole entity or just this component
				//pComponent->GetEntity()->SendEvent(event);
				pComponent->SendEvent(event);
			}
		}
	}

	return SRuntimeResult(ERuntimeStatus::Continue, EOutputIdx::Out);
}

const CryGUID CScriptGraphSetNode::ms_typeGUID = "23145b7a-4ce3-45b8-a34b-1c997ea6448f"_cry_guid;
} // Schematyc

SCHEMATYC_REGISTER_SCRIPT_GRAPH_NODE(Schematyc::CScriptGraphSetNode::Register)
