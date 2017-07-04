// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Script/Graph/Nodes/ScriptGraphFlowGraphNode.h"

#include <CrySerialization/Decorators/ActionButton.h>
#include <CrySchematyc/Compiler/CompilerContext.h>
#include <CrySchematyc/Compiler/IGraphNodeCompiler.h>
#include <CrySchematyc/Env/IEnvRegistry.h>
#include <CrySchematyc/Env/Elements/IEnvComponent.h>
#include <CrySchematyc/Env/Elements/IEnvFunction.h>
#include <CrySchematyc/Script/IScriptRegistry.h>
#include <CrySchematyc/Script/Elements/IScriptComponentInstance.h>
#include <CrySchematyc/Script/Elements/IScriptFunction.h>
#include <CrySchematyc/Utils/Any.h>
#include <CrySchematyc/Utils/IGUIDRemapper.h>
#include <CrySchematyc/Utils/StackString.h>
#include <CrySchematyc/Utils/SharedString.h>

#include "Object.h"
#include "CVars.h"
#include "Runtime/RuntimeClass.h"
#include "Script/ScriptView.h"
#include "Script/Graph/ScriptGraphNode.h"
#include "Script/Graph/ScriptGraphNodeFactory.h"
#include "SerializationUtils/SerializationContext.h"

#include <CryFlowGraph/IFlowBaseNode.h>

namespace Schematyc {
	
const CryGUID CScriptGraphFlowGraphNode::ms_typeGUID = "7067329D-AFED-4321-9D18-D4B1CA433B3A"_cry_guid;

namespace FlowGraph {

	struct CFlowGraphDummyClass : public IFlowGraph
	{
		EntityId m_entityId;

		// NFlowSystemUtils::IFlowSystemTyped
		virtual void DoActivatePort(const SFlowAddress, const NFlowSystemUtils::Wrapper<SFlowSystemVoid>& value) final {};
		virtual void DoActivatePort(const SFlowAddress, const NFlowSystemUtils::Wrapper<int>& value) final {};
		virtual void DoActivatePort(const SFlowAddress, const NFlowSystemUtils::Wrapper<float>& value) final {};
		virtual void DoActivatePort(const SFlowAddress, const NFlowSystemUtils::Wrapper<EntityId>& value) final {};
		virtual void DoActivatePort(const SFlowAddress, const NFlowSystemUtils::Wrapper<Vec3>& value) final {};
		virtual void DoActivatePort(const SFlowAddress, const NFlowSystemUtils::Wrapper<string>& value) final {};
		virtual void DoActivatePort(const SFlowAddress, const NFlowSystemUtils::Wrapper<bool>& value) final {};
		//~NFlowSystemUtils::IFlowSystemTyped

		
		//IFlowGraph
		virtual void          AddRef() final {};
		virtual void          Release() final {};
		virtual IFlowGraphPtr Clone() final { return this; };
		virtual void Clear() final {};
		virtual void RegisterHook(IFlowGraphHookPtr) final {};
		virtual void UnregisterHook(IFlowGraphHookPtr) final {};
		virtual IFlowNodeIteratorPtr CreateNodeIterator() final { return nullptr; };
		virtual IFlowEdgeIteratorPtr CreateEdgeIterator() final { return nullptr; };
		virtual void SetGraphEntity(EntityId id, int nIndex = 0) final { m_entityId = id; };
		virtual EntityId GetGraphEntity(int nIndex) const final { return m_entityId; };
		virtual void SetEnabled(bool bEnable) final {};
		virtual bool IsEnabled() const final { return true; };
		virtual void SetActive(bool bActive) final {};
		virtual bool IsActive() const final { return true; };
		virtual void UnregisterFromFlowSystem() final {};
		virtual void SetType(IFlowGraph::EFlowGraphType type) final {};
		virtual IFlowGraph::EFlowGraphType GetType() const final { return eFGT_Default; };
		virtual const char* GetDebugName() const final { return ""; };
		virtual void SetDebugName(const char* sName) final {};
		virtual void Update() final {};
		virtual bool SerializeXML(const XmlNodeRef& root, bool reading) final { return false; };
		virtual void Serialize(TSerialize ser) final {};
		virtual void PostSerialize() final {};
		virtual void InitializeValues() final {};
		virtual void PrecacheResources() final {};
		virtual void EnsureSortedEdges() final {};
		virtual SFlowAddress ResolveAddress(const char* addr, bool isOutput) final { return SFlowAddress(0,0,false); };
		virtual TFlowNodeId  ResolveNode(const char* name) final { assert(0); return 0; };
		virtual TFlowNodeId  CreateNode(TFlowNodeTypeId typeId, const char* name, void* pUserData = 0) final { assert(0); return 0; };
		virtual TFlowNodeId     CreateNode(const char* typeName, const char* name, void* pUserData = 0) final { assert(0); return 0; };
		virtual IFlowNodeData*  GetNodeData(TFlowNodeId id) final { assert(0); return nullptr; };
		virtual bool            SetNodeName(TFlowNodeId id, const char* sName) final { return true; };
		virtual const char*     GetNodeName(TFlowNodeId id) final { return ""; };
		virtual TFlowNodeTypeId GetNodeTypeId(TFlowNodeId id) final { return 0; };
		virtual const char*     GetNodeTypeName(TFlowNodeId id) final { return ""; };
		virtual void            RemoveNode(const char* name) final {};
		virtual void            RemoveNode(TFlowNodeId id) final {};
		virtual void            SetUserData(TFlowNodeId id, const XmlNodeRef& data) final {};
		virtual XmlNodeRef      GetUserData(TFlowNodeId id) final { return nullptr; };
		virtual bool            LinkNodes(SFlowAddress from, SFlowAddress to) final { return false; };
		virtual void            UnlinkNodes(SFlowAddress from, SFlowAddress to) final {};
		virtual void RegisterFlowNodeActivationListener(SFlowNodeActivationListener* listener) final {};
		virtual void RemoveFlowNodeActivationListener(SFlowNodeActivationListener* listener) final {};
		virtual bool NotifyFlowNodeActivationListeners(TFlowNodeId srcNode, TFlowPortId srcPort, TFlowNodeId toNode, TFlowPortId toPort, const char* value) final { return false; };
		virtual void     SetEntityId(TFlowNodeId, EntityId) final {};
		virtual EntityId GetEntityId(TFlowNodeId) final { return m_entityId; };
		virtual IFlowGraphPtr GetClonedFlowGraph() const final { return const_cast<CFlowGraphDummyClass*>(this); };
		virtual void GetNodeConfiguration(TFlowNodeId id, SFlowNodeConfig&) final {};
		virtual void SetRegularlyUpdated(TFlowNodeId, bool) final {};
		virtual void RequestFinalActivation(TFlowNodeId) final {};
		virtual void ActivateNode(TFlowNodeId) final {};
		virtual void ActivatePortAny(SFlowAddress output, const TFlowInputData&) final {};
		virtual void ActivatePortCString(SFlowAddress output, const char* cstr) final {};
		virtual bool SetInputValue(TFlowNodeId node, TFlowPortId port, const TFlowInputData&) final { return false; };
		virtual bool IsOutputConnected(SFlowAddress output) final { return false; };
		virtual const TFlowInputData* GetInputValue(TFlowNodeId node, TFlowPortId port) final { return nullptr; };
		virtual bool GetActivationInfo(const char* nodeName, IFlowNode::SActivationInfo& actInfo) final { return false; };
		virtual void SetSuspended(bool suspend = true) final {};
		virtual bool IsSuspended() const final { return false; };
		virtual void SetAIAction(IAIAction* pAIAction) final {};
		virtual IAIAction* GetAIAction() const final { return nullptr; };
		virtual void SetCustomAction(ICustomAction* pCustomAction) final {};
		virtual ICustomAction* GetCustomAction() const final { return nullptr; };
		virtual void GetMemoryUsage(ICrySizer* s) const final {};

		virtual void RemoveGraphTokens() final {};
		virtual bool AddGraphToken(const SGraphToken& token) final { return false; };
		virtual size_t GetGraphTokenCount() const final { return 0; };
		virtual const IFlowGraph::SGraphToken* GetGraphToken(size_t index) const final { return nullptr; };
		virtual const char*                    GetGlobalNameForGraphToken(const char* tokenName) const final { return ""; };

		virtual TFlowGraphId GetGraphId() const final { return 0; };
		//~IFlowGraph
	};

	CAnyValuePtr FlowGraphTypeToAny(int flowGraphTypeIndex)
	{
		static CAnyValuePtr s_flowTypesTable[eFDT_Bool+1]; // Assumes eFDT_Bool is the last type in EFlowDataTypes
		assert(flowGraphTypeIndex-eFDT_Any >= 0 && flowGraphTypeIndex-eFDT_Any < CRY_ARRAY_COUNT(s_flowTypesTable));

		int arrayIndex = flowGraphTypeIndex - eFDT_Any;
		if (s_flowTypesTable[arrayIndex])
		{
			return s_flowTypesTable[arrayIndex];
		}

		CAnyValuePtr pAny;
		switch (flowGraphTypeIndex)
		{
		case eFDT_Any:
			break;

		case eFDT_Void:
			break;

		case eFDT_Int:
			pAny = CAnyValue::MakeShared<int>(0);
			break;

		case eFDT_Float:
			pAny = CAnyValue::MakeShared<float>(0.0f);
			break;

		case eFDT_EntityId:
			pAny = CAnyValue::MakeShared<EntityId>(0);
			break;

		case eFDT_Vec3:
			pAny = CAnyValue::MakeShared<Vec3>(Vec3(0, 0, 0));
			break;

		case eFDT_String:
			pAny = CAnyValue::MakeShared<CSharedString>("");
			break;

		case eFDT_Bool:
			pAny = CAnyValue::MakeShared<bool>(0);
			break;
		}
		s_flowTypesTable[arrayIndex] = pAny;
		return pAny;
	}


	CAnyValuePtr FlowGraphVariantToAny(const TFlowInputDataVariant &variant)
	{
		int flowGraphTypeIndex = variant.index();

		CAnyValuePtr pAny;
		switch (flowGraphTypeIndex)
		{
		case eFDT_Any:
			break;

		case eFDT_Void:
			break;

		case eFDT_Int:
			pAny = CAnyValue::MakeShared<int>(stl::get<int>(variant));
			break;

		case eFDT_Float:
			pAny = CAnyValue::MakeShared<float>(stl::get<float>(variant));
			break;

		case eFDT_EntityId:
			pAny = CAnyValue::MakeShared<EntityId>(stl::get<EntityId>(variant));
			break;

		case eFDT_Vec3:
			pAny = CAnyValue::MakeShared<Vec3>(stl::get<Vec3>(variant));
			break;

		case eFDT_String:
			pAny = CAnyValue::MakeShared<CSharedString>(stl::get<string>(variant).c_str());
			break;

		case eFDT_Bool:
			pAny = CAnyValue::MakeShared<bool>(stl::get<bool>(variant));
			break;
		}
		return pAny;
	}
	
	void AssignAnyToFlowGraphInputData(const CAnyConstPtr &any,TFlowInputData &flowInputData)
	{
		EFlowDataTypes flowType = flowInputData.GetType();
		CAnyConstPtr pAny;
		switch (flowType)
		{
		case eFDT_Any:
			break;

		case eFDT_Void:
			break;

		case eFDT_Int:
			flowInputData.Set<int>( *DynamicCast<int>(any) );
			break;

		case eFDT_Float:
			flowInputData.Set<float>( *DynamicCast<float>(any) );
			break;

		case eFDT_EntityId:
			flowInputData.Set<EntityId>( *DynamicCast<EntityId>(any) );
			break;

		case eFDT_Vec3:
			flowInputData.Set<Vec3>( *DynamicCast<Vec3>(any) );
			break;

		case eFDT_String:
			{
				const CSharedString* pStr = DynamicCast<CSharedString>(any);
				if (pStr)
				{
					flowInputData.Set<string>( pStr->c_str() );
				}
			}
			break;

		case eFDT_Bool:
			flowInputData.Set<bool>( *DynamicCast<bool>(any) );
			break;
		}
	}

	void AssignFlowGraphVariantToAny(const TFlowInputDataVariant &variant,const CAnyPtr &any)
	{
		int flowGraphTypeIndex = variant.index();

		switch (flowGraphTypeIndex)
		{
		case eFDT_Any:
			break;

		case eFDT_Void:
			break;

		case eFDT_Int:
			*DynamicCast<int>(any) = stl::get<int>(variant);
			break;

		case eFDT_Float:
			*DynamicCast<float>(any) = stl::get<float>(variant);
			break;

		case eFDT_EntityId:
			*DynamicCast<EntityId>(any) = stl::get<EntityId>(variant);
			break;

		case eFDT_Vec3:
			*DynamicCast<Vec3>(any) = stl::get<Vec3>(variant);
			break;

		case eFDT_String:
			*DynamicCast<CSharedString>(any) = stl::get<string>(variant).c_str();
			break;

		case eFDT_Bool:
			*DynamicCast<bool>(any) = stl::get<bool>(variant);
			break;
		}
	}
}

struct SEnvFlowGraphRuntimeData
{
	static void ReflectType(CTypeDesc<SEnvFlowGraphRuntimeData>& desc)
	{
		desc.SetGUID("89A6C32B-5070-4155-9329-1B79C8022A31"_cry_guid);
	}

	struct SFlowGraphNode
	{
		int inputCount = 0;
		int outputCount = 0;
		SFlowNodeConfig config;
		IFlowNodePtr pNode = nullptr;
		std::vector<TFlowInputData> inputData;
		bool hasEntity  = false;
		bool hasInputVoidPorts = false;

		SFlowGraphNode(TFlowNodeTypeId typeId)
		{
			FlowGraph::CFlowGraphDummyClass flowGraph;
			IFlowNode::SActivationInfo actInfo(&flowGraph);

			pNode = gEnv->pFlowSystem->CreateNodeOfType(&actInfo,typeId);

			pNode->GetConfiguration(config);

			hasEntity = 0 != (config.nFlags & EFLN_TARGET_ENTITY);
			if (!config.pInputPorts)
				inputCount = 0;
			else
				for (inputCount = 0; config.pInputPorts[inputCount].name; inputCount++)
					;
			if (0 != (config.nFlags & EFLN_DYNAMIC_OUTPUT))
				outputCount = 64; // Allow for so many output ports to be made
			else if (!config.pOutputPorts)
				outputCount = 0;
			else
				for (outputCount = 0; config.pOutputPorts[outputCount].name; outputCount++)
					;

			inputData.resize(inputCount);
			for (int i = 0; i < inputCount; i++)
			{
				inputData[i] = config.pInputPorts[i].defaultData;
				if (config.pInputPorts[i].defaultData.GetType() == eFDT_Void)
				{
					hasInputVoidPorts = true;
				}
			}
		}
	};


	SEnvFlowGraphRuntimeData( TFlowNodeTypeId typeId )
	{
		m_pNode = std::make_shared<SFlowGraphNode>(typeId);
	}

	std::shared_ptr<SFlowGraphNode> m_pNode;
};

struct STempFGNodeActivationUserData
{
	CRuntimeGraphNodeInstance* pNode;
	SEnvFlowGraphRuntimeData::SFlowGraphNode *pFlowGraphNode;
	int outputPort;
};

CScriptGraphFlowGraphNode::CScriptGraphFlowGraphNode() {}

CScriptGraphFlowGraphNode::CScriptGraphFlowGraphNode(const string &flowNodeTypeName, const CryGUID& objectGUID)
	: m_objectGUID(objectGUID)
	, m_flowNodeTypeName(flowNodeTypeName)
{

	if (gEnv->pFlowSystem && m_flowNodeTypeName)
	{
		m_flowNodeTypeId = gEnv->pFlowSystem->GetTypeId(m_flowNodeTypeName);
	}
}

CryGUID CScriptGraphFlowGraphNode::GetTypeGUID() const
{
	return ms_typeGUID;
}

void CScriptGraphFlowGraphNode::CreateLayout(CScriptGraphNodeLayout& layout)
{
	layout.SetStyleId("Node::FlowGraph");

	stack_string subject;

	CreateInputsAndOutputs(layout);

	subject = "FG:";
	subject += m_flowNodeTypeName.c_str();

	layout.SetName(nullptr, subject.c_str());
}

void CScriptGraphFlowGraphNode::Compile(SCompilerContext& context, IGraphNodeCompiler& compiler) const
{
	compiler.BindCallback(&ExecuteFlowGraphNode);
	compiler.BindData(SEnvFlowGraphRuntimeData(m_flowNodeTypeId));
}

void CScriptGraphFlowGraphNode::LoadDependencies(Serialization::IArchive& archive, const ISerializationContext& context)
{
	archive(m_flowNodeTypeName, "flowNodeType");
	archive(m_objectGUID, "objectGUID");

	if (archive.isInput())
	{
		if (gEnv->pFlowSystem && m_flowNodeTypeName)
		{
			m_flowNodeTypeId = gEnv->pFlowSystem->GetTypeId(m_flowNodeTypeName);
		}
	}
}

void CScriptGraphFlowGraphNode::Save(Serialization::IArchive& archive, const ISerializationContext& context)
{
	archive(m_flowNodeTypeName, "flowNodeType");
	archive(m_objectGUID, "objectGUID");
}

void CScriptGraphFlowGraphNode::Edit(Serialization::IArchive& archive, const ISerializationContext& context)
{
	Validate(archive, context);
}

void CScriptGraphFlowGraphNode::Validate(Serialization::IArchive& archive, const ISerializationContext& context)
{
}

void CScriptGraphFlowGraphNode::RemapDependencies(IGUIDRemapper& guidRemapper)
{
	m_objectGUID = guidRemapper.Remap(m_objectGUID);
}

void CScriptGraphFlowGraphNode::Register(CScriptGraphNodeFactory& factory)
{
	class CCreator : public IScriptGraphNodeCreator
	{
	private:

		class CCreationCommand : public IScriptGraphNodeCreationCommand
		{
		public:

			CCreationCommand(const char* szSubject, const char* szDescription, const string& flowNodeTypeName, const CryGUID& objectGUID = CryGUID())
				: m_subject(szSubject)
				, m_description(szDescription)
				, m_flowNodeTypeName(flowNodeTypeName)
				, m_objectGUID(objectGUID)
			{}

			// IScriptGraphNodeCreationCommand

			virtual const char* GetBehavior() const override
			{
				return "Function";
			}

			virtual const char* GetSubject() const override
			{
				return m_subject.c_str();
			}

			virtual const char* GetDescription() const override
			{
				return m_description.c_str();
			}

			virtual const char* GetStyleId() const override
			{
				return "Node::FlowGraph";
			}

			virtual IScriptGraphNodePtr Execute(const Vec2& pos) override
			{
				return std::make_shared<CScriptGraphNode>(gEnv->pSchematyc->CreateGUID(), stl::make_unique<CScriptGraphFlowGraphNode>(m_flowNodeTypeName, m_objectGUID), pos);
			}

			// ~IScriptGraphNodeCreationCommand

		private:

			string     m_subject;
			string     m_description;
			string     m_flowNodeTypeName;
			CryGUID    m_objectGUID;
		};

	public:

		// IScriptGraphNodeCreator

		virtual CryGUID GetTypeGUID() const override
		{
			return CScriptGraphFlowGraphNode::ms_typeGUID;
		}

		virtual IScriptGraphNodePtr CreateNode(const CryGUID& guid) override
		{
			return std::make_shared<CScriptGraphNode>(guid, stl::make_unique<CScriptGraphFlowGraphNode>());
		}

		virtual void PopulateNodeCreationMenu(IScriptGraphNodeCreationMenu& nodeCreationMenu, const IScriptView& scriptView, const IScriptGraph& graph) override
		{
			if(CVars::sc_allowFlowGraphNodes)
			{
				CryGUID objectGUID = CryGUID::Null();

				IFlowNodeTypeIteratorPtr pTypeIterator = gEnv->pFlowSystem->CreateNodeTypeIterator();
				IFlowNodeTypeIterator::SNodeType nodeType;
				while (pTypeIterator->Next(nodeType))
				{
					string nodeTypeName = nodeType.typeName;
					stack_string nodeName = "FlowGraph::";
					nodeName += nodeTypeName;
					SEnvFlowGraphRuntimeData data(nodeType.typeId);

					nodeCreationMenu.AddCommand(std::make_shared<CCreationCommand>(nodeName.c_str(), data.m_pNode->config.sDescription, nodeTypeName, objectGUID));
				}
			}
		}

		// ~IScriptGraphNodeCreator
	};

	factory.RegisterCreator(std::make_shared<CCreator>());
}

void CScriptGraphFlowGraphNode::CreateInputsAndOutputs(CScriptGraphNodeLayout& layout)
{
	SEnvFlowGraphRuntimeData data(m_flowNodeTypeId);
	if (!data.m_pNode)
		return;

	int inputCount = data.m_pNode->inputCount;
	int outputCount = data.m_pNode->outputCount;

	SEnvFlowGraphRuntimeData::SFlowGraphNode *fgNode = data.m_pNode.get();

	if (!fgNode->hasInputVoidPorts)
	{
		layout.AddInput("In", CryGUID(), { EScriptGraphPortFlags::Flow, EScriptGraphPortFlags::MultiLink });
	}
	layout.AddOutput("Out", CryGUID(), EScriptGraphPortFlags::Flow);

	for (uint32 inputIdx = 0; inputIdx < inputCount; ++inputIdx)
	{
		const char* szPortName = data.m_pNode->config.pInputPorts[inputIdx].name;
		const char* szPortHumanName = data.m_pNode->config.pInputPorts[inputIdx].humanName;
		if (!szPortHumanName)
			szPortHumanName = szPortName;

		CAnyValuePtr pData = FlowGraph::FlowGraphVariantToAny(fgNode->inputData[inputIdx].GetVariant());
		if (pData)
		{
			layout.AddInputWithData(
				CUniqueId::FromString(szPortName),
				szPortHumanName,
				pData->GetTypeDesc().GetGUID(),
				{ EScriptGraphPortFlags::Data, EScriptGraphPortFlags::Persistent, EScriptGraphPortFlags::Editable }, 
				*pData
			);
		}
		else
		{
			layout.AddInput(
				CUniqueId::FromString(szPortName),
				szPortHumanName,
				CryGUID(),
				{ EScriptGraphPortFlags::Flow, EScriptGraphPortFlags::MultiLink });
		}
	}

	for (uint32 outputIdx = 0; outputIdx < outputCount; ++outputIdx)
	{
		const char* szPortName = data.m_pNode->config.pOutputPorts[outputIdx].name;
		const char* szPortHumanName = data.m_pNode->config.pOutputPorts[outputIdx].humanName;
		if (!szPortHumanName)
			szPortHumanName = szPortName;

		CAnyValuePtr pData = FlowGraph::FlowGraphTypeToAny(data.m_pNode->config.pOutputPorts[outputIdx].type);
		if (pData)
		{
			layout.AddOutputWithData(
				CUniqueId::FromString(szPortName),
				szPortHumanName,
				pData->GetTypeDesc().GetGUID(),
				{ EScriptGraphPortFlags::Data, EScriptGraphPortFlags::MultiLink },
				*pData
			);
		}
		else
		{
			layout.AddOutput(
				CUniqueId::FromString(szPortName),
				szPortHumanName,
				CryGUID(),
				{ EScriptGraphPortFlags::Flow }
			);

		}
	}
}

SRuntimeResult CScriptGraphFlowGraphNode::ExecuteFlowGraphNode(SRuntimeContext& context, const SRuntimeActivationParams& activationParams)
{
	SEnvFlowGraphRuntimeData& data = DynamicCast<SEnvFlowGraphRuntimeData>(*context.node.GetData());

	CObject* pObject = static_cast<CObject*>(context.pObject);
	
	SEnvFlowGraphRuntimeData::SFlowGraphNode* pFGNode = data.m_pNode.get();

	int inputPortOffset = context.node.GetInputCount() - pFGNode->inputCount;
	
	// Copy Schematyc node input data to the FlowGraph node input data
	for (int inputIdx = 0,count = context.node.GetInputCount(); inputIdx < count; inputIdx++)
	{
		int flowNodeInputIdx = inputIdx - inputPortOffset; // 0 input port id can be Flow "In"
		assert(flowNodeInputIdx < pFGNode->inputCount);

		if (flowNodeInputIdx >= 0)
		{
			if (context.node.IsDataInput(inputIdx))
			{
				CAnyConstPtr pAny = context.node.GetInputData(inputIdx);
				FlowGraph::AssignAnyToFlowGraphInputData(pAny, pFGNode->inputData[flowNodeInputIdx]);
			}
			// Activate all linked inputs
			if (inputIdx == activationParams.portIdx || context.node.IsInputLinked(inputIdx))
			{
				// Mark this flow graph node input as Active
				pFGNode->inputData[flowNodeInputIdx].SetUserFlag(true);
			}
		}
	}

	auto activateOutputLambda = [](IFlowNode::SActivationInfo *pActInfo,int nOutputPort, const TFlowInputData& value)
	{
		// Copy FlowGraph node output data to Schematyc node output data
		STempFGNodeActivationUserData *pUserData = static_cast<STempFGNodeActivationUserData*>(pActInfo->m_pUserData);
		
		if (pUserData->pFlowGraphNode->config.pOutputPorts[nOutputPort].type != eFDT_Void)
		{
			CAnyPtr pAnyOutput = pUserData->pNode->GetOutputData(nOutputPort+1);
			FlowGraph::AssignFlowGraphVariantToAny(value.GetVariant(),pAnyOutput);
		}
		else
		{
			// Only void outputs can be out flow triggers
			pUserData->outputPort = nOutputPort;
		}
	};

	RuntimeGraphPortIdx outputPortId = EOutputIdx::Out;

	if (!pFGNode->inputData.empty())
	{
		IEntity* pNodeEntity = nullptr;
		FlowGraph::CFlowGraphDummyClass flowGraph;
		if (pObject && pObject->GetEntity())
		{
			pNodeEntity = pObject->GetEntity();
			flowGraph.m_entityId = pNodeEntity->GetId();
		}
		TFlowInputData *pInputData = &pFGNode->inputData[0];
		
		STempFGNodeActivationUserData tempUserData;
		tempUserData.pNode = &context.node;
		tempUserData.outputPort = -1;
		tempUserData.pFlowGraphNode = pFGNode;

		IFlowNode::SActivationInfo fgActivateInfo(nullptr, 0, &tempUserData, pInputData);
		fgActivateInfo.pEntity = pNodeEntity;
		fgActivateInfo.activateOutputCallback = activateOutputLambda;
		pFGNode->pNode->ProcessEvent(IFlowNode::EFlowEvent::eFE_Activate, &fgActivateInfo);

		if (tempUserData.outputPort != -1)
		{
			// Adjust output portid from this node.
			outputPortId = EOutputIdx::Out + 1 + tempUserData.outputPort;
		}
	}

	return SRuntimeResult(ERuntimeStatus::Continue, outputPortId);
}

} // Schematyc

SCHEMATYC_REGISTER_SCRIPT_GRAPH_NODE(Schematyc::CScriptGraphFlowGraphNode::Register)
