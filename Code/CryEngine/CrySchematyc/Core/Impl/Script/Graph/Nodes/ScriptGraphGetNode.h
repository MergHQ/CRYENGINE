// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Reflection/Reflection.h>
#include <Schematyc/Runtime/RuntimeGraph.h>
#include <Schematyc/Utils/GUID.h>

#include "Script/Graph/ScriptGraphNodeModel.h"

namespace Schematyc
{
class CScriptGraphGetNode : public CScriptGraphNodeModel
{
private:

	struct EOutputIdx
	{
		enum : uint32
		{
			Value = 0
		};
	};

	struct SRuntimeData
	{
		SRuntimeData(uint32 _pos);
		SRuntimeData(const SRuntimeData& rhs);

		static SGUID ReflectSchematycType(CTypeInfo<SRuntimeData>& typeInfo);

		uint32       pos;
	};

public:

	CScriptGraphGetNode();
	CScriptGraphGetNode(const SGUID& referenceGUID);

	// CScriptGraphNodeModel
	virtual void  Compile(SCompilerContext& context, IGraphNodeCompiler& compiler) const override;
	virtual SGUID GetTypeGUID() const override;
	virtual void  CreateLayout(CScriptGraphNodeLayout& layout) override;
	virtual void  LoadDependencies(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void  Save(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void  Validate(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void  RemapDependencies(IGUIDRemapper& guidRemapper) override;
	// ~CScriptGraphNodeModel

	static void Register(CScriptGraphNodeFactory& factory);

private:

	static SRuntimeResult Execute(SRuntimeContext& context, const SRuntimeActivationParams& activationParams);
	static SRuntimeResult ExecuteArray(SRuntimeContext& context, const SRuntimeActivationParams& activationParams);

public:

	static const SGUID ms_typeGUID;

private:

	SGUID m_referenceGUID;
};
}
