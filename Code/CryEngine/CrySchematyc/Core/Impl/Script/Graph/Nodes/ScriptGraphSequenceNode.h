// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/FundamentalTypes.h>
#include <Schematyc/Reflection/Reflection.h>
#include <Schematyc/Runtime/RuntimeGraph.h>
#include <Schematyc/Utils/GUID.h>

#include "Script/ScriptVariableData.h"
#include "Script/Graph/ScriptGraphNodeModel.h"

namespace Schematyc
{
// Forward declare classes.
class CAnyValue;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(CAnyValue)

class CScriptGraphSequenceNode : public CScriptGraphNodeModel
{
private:

	struct SOutput
	{
		void Serialize(Serialization::IArchive& archive);

		string name;
		bool   bIsDuplicate = false;
	};

	typedef std::vector<SOutput> Outputs;

public:

	// CScriptGraphNodeModel
	virtual SGUID GetTypeGUID() const override;
	virtual void  CreateLayout(CScriptGraphNodeLayout& layout) override;
	virtual void  Compile(SCompilerContext& context, IGraphNodeCompiler& compiler) const override;
	virtual void  PostLoad(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void  Save(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void  Edit(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void  Validate(Serialization::IArchive& archive, const ISerializationContext& context) override;
	// ~CScriptGraphNodeModel

	static void Register(CScriptGraphNodeFactory& factory);

private:

	void                  RefreshOutputs();

	static SRuntimeResult Execute(SRuntimeContext& context, const SRuntimeActivationParams& activationParams);

public:

	static const SGUID ms_typeGUID;

private:

	Outputs m_outputs;
};
} // Schematyc
