// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Reflection/Reflection.h>
#include <Schematyc/Runtime/RuntimeGraph.h>
#include <Schematyc/SerializationUtils/MultiPassSerializer.h>
#include <Schematyc/Utils/GUID.h>

#include "Script/ScriptVariableData.h"
#include "Script/Graph/ScriptGraphNodeModel.h"

namespace Schematyc
{
class CScriptGraphFormatStringNode : public CScriptGraphNodeModel
{
private:

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
			Result
		};
	};

public: // #SchematycTODO :  Workaround for serialization of EElementForm!

	enum class EElementForm
	{
		Const,
		Input
	};

	struct SElement : public CMultiPassSerializer
	{
		SElement();

		// CMultiPassSerializer
		virtual void LoadDependencies(Serialization::IArchive& archive, const ISerializationContext& context) override;
		virtual void PostLoad(Serialization::IArchive& archive, const ISerializationContext& context) override;
		virtual void Save(Serialization::IArchive& archive, const ISerializationContext& context) override;
		virtual void Edit(Serialization::IArchive& archive, const ISerializationContext& context) override;
		// ~CMultiPassSerializer

		bool IsValidInput() const;

		EElementForm        form;
		string              text;
		CScriptVariableData data;
	};

	typedef std::vector<SElement> Elements;

	DECLARE_SHARED_POINTERS(Elements)

	struct SRuntimeData
	{
		SRuntimeData(const ElementsPtr& _pElements);
		SRuntimeData(const SRuntimeData& rhs);

		static SGUID ReflectSchematycType(CTypeInfo<SRuntimeData>& typeInfo);

		ElementsPtr  pElements;
	};

public:

	CScriptGraphFormatStringNode();

	// CScriptGraphNodeModel
	virtual SGUID GetTypeGUID() const override;
	virtual void  CreateLayout(CScriptGraphNodeLayout& layout) override;
	virtual void  Compile(SCompilerContext& context, IGraphNodeCompiler& compiler) const override;
	virtual void  LoadDependencies(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void  PostLoad(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void  Save(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void  Edit(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void  RemapDependencies(IGUIDRemapper& guidRemapper) override;
	// ~CScriptGraphNodeModel

	static void Register(CScriptGraphNodeFactory& factory);

private:

	void                  SerializeElements(Serialization::IArchive& archive);

	static SRuntimeResult Execute(SRuntimeContext& context, const SRuntimeActivationParams& activationParams);

public:

	static const SGUID ms_typeGUID;

private:

	Elements m_elements;
};
}
