// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySchematyc/FundamentalTypes.h>
#include <CrySchematyc/Reflection/TypeDesc.h>
#include <CrySchematyc/Runtime/RuntimeGraph.h>
#include <CrySchematyc/Utils/GUID.h>

#include "Script/Graph/ScriptGraphNodeModel.h"

#include <CryFlowGraph/IFlowSystem.h>

namespace Schematyc
{

// Forward declare interfaces.
struct IEnvFunction;
struct IScriptFunction;

class CScriptGraphFlowGraphNode : public CScriptGraphNodeModel
{
public:

	struct EInputIdx
	{
		enum : uint32
		{
			In = 0,
			FirstParam
		};
	};

	struct EOutputIdx
	{
		enum : uint32
		{
			Out = 0,
			FirstParam
		};
	};

public:

	CScriptGraphFlowGraphNode();
	CScriptGraphFlowGraphNode(const string &flowNodeTypeName, const CryGUID& objectGUID);

	// CScriptGraphNodeModel
	virtual CryGUID GetTypeGUID() const override;
	virtual void  CreateLayout(CScriptGraphNodeLayout& layout) override;
	virtual void  Compile(SCompilerContext& context, IGraphNodeCompiler& compiler) const override;
	virtual void  LoadDependencies(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void  Save(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void  Edit(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void  Validate(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void  RemapDependencies(IGUIDRemapper& guidRemapper) override;
	// ~CScriptGraphNodeModel

	static void Register(CScriptGraphNodeFactory& factory);

private:

	void                  CreateInputsAndOutputs(CScriptGraphNodeLayout& layout);

	static SRuntimeResult ExecuteFlowGraphNode(SRuntimeContext& context, const SRuntimeActivationParams& activationParams);

public:

	static const CryGUID ms_typeGUID;

private:

	//SElementId m_functionId;
	CryGUID    m_objectGUID;

	TFlowNodeTypeId m_flowNodeTypeId = 0;
	string m_flowNodeTypeName;
};

} // Schematyc
