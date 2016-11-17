// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Script/Graph/Nodes/ScriptGraphBeginNode.h"

#include <Schematyc/Compiler/CompilerContext.h>
#include <Schematyc/Compiler/IGraphNodeCompiler.h>
#include <Schematyc/Env/IEnvRegistry.h>
#include <Schematyc/Env/Elements/IEnvSignal.h>
#include <Schematyc/Script/IScriptRegistry.h>
#include <Schematyc/Script/Elements/IScriptFunction.h>
#include <Schematyc/Script/Elements/IScriptSignal.h>
#include <Schematyc/Script/Elements/IScriptSignalReceiver.h>
#include <Schematyc/Utils/Any.h>

#include "Runtime/RuntimeClass.h"
#include "Script/Graph/ScriptGraphNode.h"
#include "Script/Graph/ScriptGraphNodeFactory.h"

namespace Schematyc
{
CScriptGraphBeginNode::CScriptGraphBeginNode() {}

void CScriptGraphBeginNode::Init()
{
	CScriptGraphNodeModel::GetNode().SetFlags({ EScriptGraphNodeFlags::NotCopyable, EScriptGraphNodeFlags::NotRemovable });
}

SGUID CScriptGraphBeginNode::GetTypeGUID() const
{
	return ms_typeGUID;
}

void CScriptGraphBeginNode::CreateLayout(CScriptGraphNodeLayout& layout)
{
	layout.SetName("Begin");
	layout.SetColor(EScriptGraphColor::Green);
	layout.AddOutput("Out", SGUID(), { EScriptGraphPortFlags::Flow, EScriptGraphPortFlags::Begin });

	const IScriptElement& scriptElement = CScriptGraphNodeModel::GetNode().GetGraph().GetElement();
	switch (scriptElement.GetElementType())
	{
	case EScriptElementType::Function:
		{
			const IScriptFunction& scriptFunction = DynamicCast<IScriptFunction>(scriptElement);
			for (uint32 functionInputIdx = 0, functionInputCount = scriptFunction.GetInputCount(); functionInputIdx < functionInputCount; ++functionInputIdx)
			{
				CAnyConstPtr pData = scriptFunction.GetInputData(functionInputIdx);
				if (pData)
				{
					layout.AddOutputWithData(CGraphPortId::FromGUID(scriptFunction.GetInputGUID(functionInputIdx)), scriptFunction.GetInputName(functionInputIdx), scriptFunction.GetInputTypeId(functionInputIdx).guid, { EScriptGraphPortFlags::Data, EScriptGraphPortFlags::MultiLink }, *pData);
				}
			}
			break;
		}
	case EScriptElementType::SignalReceiver:
		{
			const IScriptSignalReceiver& scriptSignalReceiver = DynamicCast<IScriptSignalReceiver>(scriptElement);
			switch (scriptSignalReceiver.GetType())
			{
			case EScriptSignalReceiverType::EnvSignal:
				{
					const IEnvSignal* pEnvSignal = GetSchematycCore().GetEnvRegistry().GetSignal(scriptSignalReceiver.GetSignalGUID());
					if (pEnvSignal)
					{
						/*for (uint32 signalInputIdx = 0, signalInputCount = pEnvSignal->GetInputCount(); signalInputIdx < signalInputCount; ++signalInputIdx)
						   {
						   CAnyConstPtr pData = pEnvSignal->GetInputData(signalInputIdx);
						   if (pData)
						   {
						    layout.AddOutputWithData(CGraphPortId::FromUniqueId(pEnvSignal->GetInputId(signalInputIdx)), pData->GetTypeInfo().guid, pEnvSignal->GetInputTypeId(signalInputIdx).guid, { EScriptGraphPortFlags::Data, EScriptGraphPortFlags::MultiLink }, *pData);
						   }
						   }*/
					}
					break;
				}
			case EScriptSignalReceiverType::ScriptSignal:
				{
					const IScriptSignal* pScriptSignal = DynamicCast<IScriptSignal>(GetSchematycCore().GetScriptRegistry().GetElement(scriptSignalReceiver.GetSignalGUID()));
					if (pScriptSignal)
					{
						for (uint32 signalInputIdx = 0, signalInputCount = pScriptSignal->GetInputCount(); signalInputIdx < signalInputCount; ++signalInputIdx)
						{
							CAnyConstPtr pData = pScriptSignal->GetInputData(signalInputIdx);
							if (pData)
							{
								layout.AddOutputWithData(CGraphPortId::FromGUID(pScriptSignal->GetInputGUID(signalInputIdx)), pScriptSignal->GetInputName(signalInputIdx), pScriptSignal->GetInputTypeId(signalInputIdx).guid, { EScriptGraphPortFlags::Data, EScriptGraphPortFlags::MultiLink }, *pData);
							}
						}
					}
					break;
				}
			}
			break;
		}
	}
}

void CScriptGraphBeginNode::Compile(SCompilerContext& context, IGraphNodeCompiler& compiler) const
{
	CRuntimeClass* pClass = context.interfaces.Query<CRuntimeClass>();
	if (pClass)
	{
		const IScriptElement& scriptElement = CScriptGraphNodeModel::GetNode().GetGraph().GetElement();
		switch (scriptElement.GetElementType())
		{
		case EScriptElementType::Constructor:
			{
				pClass->AddConstructor(compiler.GetGraphIdx(), SRuntimeActivationParams(compiler.GetGraphNodeIdx(), EOutputIdx::Out, EActivationMode::Output));
				compiler.BindCallback(&Execute);
				break;
			}
		case EScriptElementType::Destructor:
			{
				pClass->AddDestructor(compiler.GetGraphIdx(), SRuntimeActivationParams(compiler.GetGraphNodeIdx(), EOutputIdx::Out, EActivationMode::Output));
				compiler.BindCallback(&Execute);
				break;
			}
		case EScriptElementType::Function:
			{
				pClass->AddFunction(scriptElement.GetGUID(), compiler.GetGraphIdx(), SRuntimeActivationParams(compiler.GetGraphNodeIdx(), EOutputIdx::Out, EActivationMode::Output));
				compiler.BindCallback(&ExecuteFunction);
				break;
			}
		case EScriptElementType::StateMachine:
			{
				const uint32 stateMachineIdx = pClass->FindStateMachine(scriptElement.GetGUID());
				if (stateMachineIdx != InvalidIdx)
				{
					pClass->SetStateMachineBeginFunction(stateMachineIdx, compiler.GetGraphIdx(), SRuntimeActivationParams(compiler.GetGraphNodeIdx(), EOutputIdx::Out, EActivationMode::Output));
					compiler.BindCallback(&Execute);
				}
				else
				{
					SCHEMATYC_COMPILER_ERROR("Failed to retrieve class state machine!");
				}
				break;
			}
		case EScriptElementType::SignalReceiver:
			{
				const IScriptSignalReceiver& scriptSignalReceiver = DynamicCast<IScriptSignalReceiver>(scriptElement);
				pClass->AddSignalReceiver(scriptSignalReceiver.GetSignalGUID(), compiler.GetGraphIdx(), SRuntimeActivationParams(compiler.GetGraphNodeIdx(), EOutputIdx::Out, EActivationMode::Output));
				compiler.BindCallback(&ExecuteSignalReceiver);
				break;
			}
		}
	}
}

void CScriptGraphBeginNode::Register(CScriptGraphNodeFactory& factory)
{
	class CCreator : public IScriptGraphNodeCreator
	{
	public:

		// IScriptGraphNodeCreator

		virtual SGUID GetTypeGUID() const override
		{
			return CScriptGraphBeginNode::ms_typeGUID;
		}

		virtual IScriptGraphNodePtr CreateNode(const SGUID& guid) override
		{
			return std::make_shared<CScriptGraphNode>(guid, stl::make_unique<CScriptGraphBeginNode>());
		}

		virtual void PopulateNodeCreationMenu(IScriptGraphNodeCreationMenu& nodeCreationMenu, const IScriptView& scriptView, const IScriptGraph& graph) override {}

		// ~IScriptGraphNodeCreator
	};

	factory.RegisterCreator(std::make_shared<CCreator>());
}

SRuntimeResult CScriptGraphBeginNode::Execute(SRuntimeContext& context, const SRuntimeActivationParams& activationParams)
{
	return SRuntimeResult(ERuntimeStatus::Continue, EOutputIdx::Out);
}

SRuntimeResult CScriptGraphBeginNode::ExecuteFunction(SRuntimeContext& context, const SRuntimeActivationParams& activationParams)
{
	for (uint8 outputIdx = EOutputIdx::FirstParam, outputCount = context.node.GetOutputCount(); outputIdx < outputCount; ++outputIdx)
	{
		if (context.node.IsDataOutput(outputIdx))
		{
			CAnyConstPtr pSrcValue = context.params.GetInput(outputIdx - EOutputIdx::FirstParam);
			if (pSrcValue)
			{
				Any::CopyAssign(*context.node.GetOutputData(outputIdx), *pSrcValue);
			}
		}
	}

	return SRuntimeResult(ERuntimeStatus::Continue, EOutputIdx::Out);
}

SRuntimeResult CScriptGraphBeginNode::ExecuteSignalReceiver(SRuntimeContext& context, const SRuntimeActivationParams& activationParams)
{
	for (uint8 outputIdx = EOutputIdx::FirstParam, outputCount = context.node.GetOutputCount(); outputIdx < outputCount; ++outputIdx)
	{
		if (context.node.IsDataOutput(outputIdx))
		{
			CAnyConstPtr pSrcValue = context.params.GetInput(outputIdx - EOutputIdx::FirstParam);
			if (pSrcValue)
			{
				Any::CopyAssign(*context.node.GetOutputData(outputIdx), *pSrcValue);
			}
		}
	}

	return SRuntimeResult(ERuntimeStatus::Continue, EOutputIdx::Out);
}

const SGUID CScriptGraphBeginNode::ms_typeGUID = "12bdfa06-ba95-4e48-bb2d-bb48a7080abc"_schematyc_guid;
} // Schematyc

SCHEMATYC_REGISTER_SCRIPT_GRAPH_NODE(Schematyc::CScriptGraphBeginNode::Register)
