// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Script/Graph/Nodes/ScriptGraphBranchNode.h"

#include <Schematyc/Compiler/IGraphNodeCompiler.h>

#include "Script/Graph/ScriptGraphNode.h"
#include "Script/Graph/ScriptGraphNodeFactory.h"

namespace Schematyc
{
CScriptGraphBranchNode::CScriptGraphBranchNode() {}

SGUID CScriptGraphBranchNode::GetTypeGUID() const
{
	return ms_typeGUID;
}

void CScriptGraphBranchNode::CreateLayout(CScriptGraphNodeLayout& layout)
{
	layout.SetName("Branch");
	layout.SetStyleId("Core::FlowControl");
	layout.SetColor(EScriptGraphColor::Purple);

	layout.AddInput("In", SGUID(), { EScriptGraphPortFlags::Flow, EScriptGraphPortFlags::MultiLink });
	layout.AddInputWithData("Value", GetTypeInfo<bool>().GetGUID(), { EScriptGraphPortFlags::Data, EScriptGraphPortFlags::Persistent, EScriptGraphPortFlags::Editable }, bool(false));

	layout.AddOutput("True", SGUID(), EScriptGraphPortFlags::Flow);
	layout.AddOutput("False", SGUID(), EScriptGraphPortFlags::Flow);
}

void CScriptGraphBranchNode::Compile(SCompilerContext& context, IGraphNodeCompiler& compiler) const
{
	compiler.BindCallback(&Execute);
}

void CScriptGraphBranchNode::Register(CScriptGraphNodeFactory& factory)
{
	class CCreator : public IScriptGraphNodeCreator
	{
	private:

		class CCreationCommand : public IScriptGraphNodeCreationCommand
		{
		public:

			// IScriptGraphNodeCreationCommand

			virtual const char* GetBehavior() const override
			{
				return "Branch";
			}

			virtual const char* GetSubject() const override
			{
				return nullptr;
			}

			virtual const char* GetDescription() const override
			{
				return "Branch flow";
			}

			virtual const char* GetStyleId() const override
			{
				return "Core::FlowControl";
			}

			virtual IScriptGraphNodePtr Execute(const Vec2& pos) override
			{
				return std::make_shared<CScriptGraphNode>(gEnv->pSchematyc->CreateGUID(), stl::make_unique<CScriptGraphBranchNode>(), pos);
			}

			// ~IScriptGraphNodeCreationCommand
		};

	public:

		// IScriptGraphNodeCreator

		virtual SGUID GetTypeGUID() const override
		{
			return CScriptGraphBranchNode::ms_typeGUID;
		}

		virtual IScriptGraphNodePtr CreateNode(const SGUID& guid) override
		{
			return std::make_shared<CScriptGraphNode>(guid, stl::make_unique<CScriptGraphBranchNode>());
		}

		virtual void PopulateNodeCreationMenu(IScriptGraphNodeCreationMenu& nodeCreationMenu, const IScriptView& scriptView, const IScriptGraph& graph) override
		{
			nodeCreationMenu.AddCommand(std::make_shared<CCreationCommand>());
		}

		// ~IScriptGraphNodeCreator
	};

	factory.RegisterCreator(std::make_shared<CCreator>());
}

SRuntimeResult CScriptGraphBranchNode::Execute(SRuntimeContext& context, const SRuntimeActivationParams& activationParams)
{
	const bool bValue = DynamicCast<bool>(*context.node.GetInputData(EInputIdx::Value));
	if (bValue)
	{
		return SRuntimeResult(ERuntimeStatus::Continue, EOutputIdx::True);
	}
	else
	{
		return SRuntimeResult(ERuntimeStatus::Continue, EOutputIdx::False);
	}
}

const SGUID CScriptGraphBranchNode::ms_typeGUID = "b30f637b-555a-4e2e-8558-87fa49d470c1"_schematyc_guid;
} // Schematyc

SCHEMATYC_REGISTER_SCRIPT_GRAPH_NODE(Schematyc::CScriptGraphBranchNode::Register)
