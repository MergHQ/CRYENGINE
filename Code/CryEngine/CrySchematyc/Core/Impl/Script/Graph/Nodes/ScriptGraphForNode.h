// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Reflection/TypeDesc.h>
#include <Schematyc/Runtime/RuntimeGraph.h>
#include <Schematyc/Utils/GUID.h>

#include "Script/Graph/ScriptGraphNodeModel.h"

namespace Schematyc
{
class CScriptGraphForNode : public CScriptGraphNodeModel
{
private:

	struct EInputIdx
	{
		enum : uint32
		{
			In = 0,
			Begin,
			End,
			Break
		};
	};

	struct EOutputIdx
	{
		enum : uint32
		{
			Out = 0,
			Loop,
			Pos
		};
	};

	struct SRuntimeData
	{
		SRuntimeData();
		SRuntimeData(const SRuntimeData& rhs);

		static void ReflectType(CTypeDesc<SRuntimeData>& desc);

		int32        pos;
		bool         bBreak;
	};

public:

	CScriptGraphForNode();

	// CScriptGraphNodeModel
	virtual SGUID GetTypeGUID() const override;
	virtual void  CreateLayout(CScriptGraphNodeLayout& layout) override;
	virtual void  Compile(SCompilerContext& context, IGraphNodeCompiler& compiler) const override;
	// ~CScriptGraphNodeModel

	static void Register(CScriptGraphNodeFactory& factory);

private:

	static SRuntimeResult Execute(SRuntimeContext& context, const SRuntimeActivationParams& activationParams);

public:

	static const SGUID ms_typeGUID;
};
} // Schematyc
