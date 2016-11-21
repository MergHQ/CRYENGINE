// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Reflection/Reflection.h>
#include <Schematyc/Runtime/RuntimeGraph.h>
#include <Schematyc/Utils/GUID.h>

#include "Script/Graph/ScriptGraphNodeModel.h"

namespace Schematyc
{
class CScriptGraphSendSignalNode : public CScriptGraphNodeModel
{
public:

	enum class ETarget
	{
		Self = 0,
		Object,
		Broadcast
	};

private:

	struct EOutputIdx
	{
		enum : uint32
		{
			Out = 0
		};
	};

	struct SRuntimeData
	{
		SRuntimeData(const SGUID& _signalGUID);
		SRuntimeData(const SRuntimeData& rhs);

		static SGUID ReflectSchematycType(CTypeInfo<SRuntimeData>& typeInfo);

		SGUID        signalGUID;
	};

public:

	CScriptGraphSendSignalNode();
	CScriptGraphSendSignalNode(const SGUID& signalGUID);

	// CScriptGraphNodeModel
	virtual SGUID GetTypeGUID() const override;
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

	void                  GoToSignal();

	static SRuntimeResult ExecuteSendToSelf(SRuntimeContext& context, const SRuntimeActivationParams& activationParams);
	static SRuntimeResult ExecuteSendToObject(SRuntimeContext& context, const SRuntimeActivationParams& activationParams);
	static SRuntimeResult ExecuteBroadcast(SRuntimeContext& context, const SRuntimeActivationParams& activationParams);

public:

	static const SGUID ms_typeGUID;

private:

	SGUID   m_signalGUID;
	ETarget m_target;
};
}
