// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySchematyc/Reflection/TypeDesc.h>
#include <CrySchematyc/Runtime/RuntimeGraph.h>
#include <CrySchematyc/Utils/GUID.h>

#include "Script/Graph/ScriptGraphNodeModel.h"

namespace Schematyc
{
class CScriptGraphSetNode : public CScriptGraphNodeModel
{
private:

	struct EInputIdx
	{
		enum : uint32
		{
			In = 0,
			Value
		};
	};

	struct EOutputIdx
	{
		enum : uint32
		{
			Out = 0
		};
	};

	struct SRuntimeData
	{
		SRuntimeData(uint32 _pos);
		SRuntimeData(const SRuntimeData& rhs);

		static void ReflectType(CTypeDesc<SRuntimeData>& desc);

		uint32       pos;
	};

	struct SComponentPropertyRuntimeData
	{
		static void ReflectType(CTypeDesc<SComponentPropertyRuntimeData>& desc) { desc.SetGUID("68074A67-B731-489A-BFE0-16269BD9A8DC"_cry_guid); };

		uint32 componentMemberIndex;
		uint32 componentIdx;
	};

public:

	CScriptGraphSetNode();
	CScriptGraphSetNode(const CryGUID& referenceGUID,uint32 componentMemberId = 0);

	// CScriptGraphNodeModel
	virtual CryGUID GetTypeGUID() const override;
	virtual void  CreateLayout(CScriptGraphNodeLayout& layout) override;
	virtual void  Compile(SCompilerContext& context, IGraphNodeCompiler& compiler) const override;
	virtual void  LoadDependencies(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void  Save(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void  Validate(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void  RemapDependencies(IGUIDRemapper& guidRemapper) override;
	// ~CScriptGraphNodeModel

	static void Register(CScriptGraphNodeFactory& factory);

private:

	static SRuntimeResult Execute(SRuntimeContext& context, const SRuntimeActivationParams& activationParams);
	static SRuntimeResult ExecuteSetComponentProperty(SRuntimeContext& context, const SRuntimeActivationParams& activationParams);

public:

	static const CryGUID ms_typeGUID;

private:

	CryGUID m_referenceGUID;
	uint32  m_componentMemberId = 0;
};
}
