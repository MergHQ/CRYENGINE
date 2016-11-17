// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Script/Graph/Nodes/ScriptGraphForNode.h"

#include <Schematyc/Compiler/IGraphNodeCompiler.h>

#include "Script/Graph/ScriptGraphNode.h"
#include "Script/Graph/ScriptGraphNodeFactory.h"

namespace Schematyc
{
CScriptGraphForNode::SRuntimeData::SRuntimeData()
	: pos(InvalidIdx)
	, bBreak(false)
{}

CScriptGraphForNode::SRuntimeData::SRuntimeData(const SRuntimeData& rhs)
	: pos(rhs.pos)
	, bBreak(rhs.bBreak)
{}

SGUID CScriptGraphForNode::SRuntimeData::ReflectSchematycType(CTypeInfo<SRuntimeData>& typeInfo)
{
	return "2cf6b75a-caed-4d69-914a-08ca7b0e5a67"_schematyc_guid;
}

CScriptGraphForNode::CScriptGraphForNode() {}

SGUID CScriptGraphForNode::GetTypeGUID() const
{
	return ms_typeGUID;
}

void CScriptGraphForNode::CreateLayout(CScriptGraphNodeLayout& layout)
{
	layout.SetName("For");
	layout.SetColor(EScriptGraphColor::Purple);

	layout.AddInput("In", SGUID(), { EScriptGraphPortFlags::Flow, EScriptGraphPortFlags::MultiLink });
	layout.AddInputWithData("Begin", GetTypeInfo<int32>().GetGUID(), { EScriptGraphPortFlags::Data, EScriptGraphPortFlags::Persistent, EScriptGraphPortFlags::Editable }, int32(0));
	layout.AddInputWithData("End", GetTypeInfo<int32>().GetGUID(), { EScriptGraphPortFlags::Data, EScriptGraphPortFlags::Persistent, EScriptGraphPortFlags::Editable }, int32(0));
	layout.AddInput("Break", SGUID(), { EScriptGraphPortFlags::Flow, EScriptGraphPortFlags::MultiLink, EScriptGraphPortFlags::End });

	layout.AddOutput("Out", SGUID(), EScriptGraphPortFlags::Flow);
	layout.AddOutput("Loop", SGUID(), EScriptGraphPortFlags::Flow);
	layout.AddOutputWithData("Pos", GetTypeInfo<int32>().GetGUID(), { EScriptGraphPortFlags::Data, EScriptGraphPortFlags::MultiLink }, int32(0));
}

void CScriptGraphForNode::Compile(SCompilerContext& context, IGraphNodeCompiler& compiler) const
{
	compiler.BindCallback(&Execute);
	compiler.BindData(SRuntimeData());
}

void CScriptGraphForNode::Register(CScriptGraphNodeFactory& factory)
{
	class CCreator : public IScriptGraphNodeCreator
	{
	private:

		class CNodeCreationMenuCommand : public IScriptGraphNodeCreationMenuCommand
		{
		public:

			// IMenuCommand

			IScriptGraphNodePtr Execute(const Vec2& pos)
			{
				return std::make_shared<CScriptGraphNode>(GetSchematycCore().CreateGUID(), stl::make_unique<CScriptGraphForNode>(), pos);
			}

			// ~IMenuCommand
		};

	public:

		// IScriptGraphNodeCreator

		virtual SGUID GetTypeGUID() const override
		{
			return CScriptGraphForNode::ms_typeGUID;
		}

		virtual IScriptGraphNodePtr CreateNode(const SGUID& guid) override
		{
			return std::make_shared<CScriptGraphNode>(guid, stl::make_unique<CScriptGraphForNode>());
		}

		virtual void PopulateNodeCreationMenu(IScriptGraphNodeCreationMenu& nodeCreationMenu, const IScriptView& scriptView, const IScriptGraph& graph) override
		{
			nodeCreationMenu.AddOption("For", "For loop", "", std::make_shared<CNodeCreationMenuCommand>());
		}

		// ~IScriptGraphNodeCreator
	};

	factory.RegisterCreator(std::make_shared<CCreator>());
}

SRuntimeResult CScriptGraphForNode::Execute(SRuntimeContext& context, const SRuntimeActivationParams& activationParams)
{
	SRuntimeData& data = DynamicCast<SRuntimeData>(*context.node.GetData());

	if (activationParams.mode == EActivationMode::Input)
	{
		switch (activationParams.portIdx)
		{
		case EInputIdx::In:
			{
				data.pos = DynamicCast<int32>(*context.node.GetInputData(EInputIdx::Begin));
				break;
			}
		case EInputIdx::Break:
			{
				data.bBreak;
				return SRuntimeResult(ERuntimeStatus::Break);
			}
		}
	}

	if (!data.bBreak)
	{
		DynamicCast<int32>(*context.node.GetOutputData(EOutputIdx::Pos)) = data.pos;

		const int32 end = DynamicCast<int32>(*context.node.GetInputData(EInputIdx::End));
		if (data.pos < end)
		{
			++data.pos;
			return SRuntimeResult(ERuntimeStatus::ContinueAndRepeat, EOutputIdx::Loop);
		}
	}

	return SRuntimeResult(ERuntimeStatus::Continue, EOutputIdx::Out);
}

const SGUID CScriptGraphForNode::ms_typeGUID = "a902d2a5-cc66-49e0-8c2e-e52b48cc7159"_schematyc_guid;
} // Schematyc

SCHEMATYC_REGISTER_SCRIPT_GRAPH_NODE(Schematyc::CScriptGraphForNode::Register)
