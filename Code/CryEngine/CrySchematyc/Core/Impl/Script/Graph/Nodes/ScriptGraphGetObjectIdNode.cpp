// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Script/Graph/Nodes/ScriptGraphGetObjectIdNode.h"

#include <Schematyc/IObject.h>
#include <Schematyc/Compiler/IGraphNodeCompiler.h>

#include <Schematyc/Script/Elements/IScriptVariable.h>

#include "Runtime/RuntimeClass.h"
#include "Script/ScriptView.h"
#include "Script/Graph/ScriptGraphNode.h"
#include "Script/Graph/ScriptGraphNodeFactory.h"
#include "SerializationUtils/SerializationContext.h"

namespace Schematyc
{
CScriptGraphGetObjectIdNode::CScriptGraphGetObjectIdNode() {}

SGUID CScriptGraphGetObjectIdNode::GetTypeGUID() const
{
	return ms_typeGUID;
}

void CScriptGraphGetObjectIdNode::CreateLayout(CScriptGraphNodeLayout& layout)
{
	layout.SetColor(EScriptGraphColor::Blue);
	layout.SetName("GetObjectId");
	layout.AddOutputWithData("ObjectId", GetTypeInfo<ObjectId>().GetGUID(), { EScriptGraphPortFlags::Data, EScriptGraphPortFlags::MultiLink, EScriptGraphPortFlags::Pull }, ObjectId());
}

void CScriptGraphGetObjectIdNode::Compile(SCompilerContext& context, IGraphNodeCompiler& compiler) const
{
	compiler.BindCallback(&Execute);
}

void CScriptGraphGetObjectIdNode::Register(CScriptGraphNodeFactory& factory)
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
				return std::make_shared<CScriptGraphNode>(GetSchematycCore().CreateGUID(), stl::make_unique<CScriptGraphGetObjectIdNode>(), pos);
			}

			// ~IMenuCommand
		};

	public:

		// IScriptGraphNodeCreator

		virtual SGUID GetTypeGUID() const override
		{
			return CScriptGraphGetObjectIdNode::ms_typeGUID;
		}

		virtual IScriptGraphNodePtr CreateNode(const SGUID& guid) override
		{
			return std::make_shared<CScriptGraphNode>(guid, stl::make_unique<CScriptGraphGetObjectIdNode>());
		}

		virtual void PopulateNodeCreationMenu(IScriptGraphNodeCreationMenu& nodeCreationMenu, const IScriptView& scriptView, const IScriptGraph& graph) override
		{
			if (scriptView.GetScriptClass())
			{
				nodeCreationMenu.AddOption("GetObjectId", "Get id of this object", "", std::make_shared<CNodeCreationMenuCommand>());
			}
		}

		// ~IScriptGraphNodeCreator
	};

	factory.RegisterCreator(std::make_shared<CCreator>());
}

SRuntimeResult CScriptGraphGetObjectIdNode::Execute(SRuntimeContext& context, const SRuntimeActivationParams& activationParams)
{
	DynamicCast<ObjectId>(*context.node.GetOutputData(EOutputIdx::Value)) = static_cast<IObject*>(context.pObject)->GetId();

	return SRuntimeResult(ERuntimeStatus::Continue);
}

const SGUID CScriptGraphGetObjectIdNode::ms_typeGUID = "6b0c3534-1117-4a57-bb15-e43c51aff2e0"_schematyc_guid;
} // Schematyc

SCHEMATYC_REGISTER_SCRIPT_GRAPH_NODE(Schematyc::CScriptGraphGetObjectIdNode::Register)
