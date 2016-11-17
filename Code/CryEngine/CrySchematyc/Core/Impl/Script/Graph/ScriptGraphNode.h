// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Script/IScriptGraph.h>
#include <Schematyc/Utils/Any.h>
#include <Schematyc/Utils/Assert.h>

namespace Schematyc
{
// Forward declare classes.
class CScriptGraphNodeModel;

struct SScriptGraphNodePort
{
	SScriptGraphNodePort();
	SScriptGraphNodePort(const CGraphPortId& _id, const char* _szName, const SGUID& _typeGUID, const ScriptGraphPortFlags& _flags, const CAnyValuePtr& _pData);

	CGraphPortId         id;
	string               name;
	SGUID                typeGUID;
	ScriptGraphPortFlags flags;
	CAnyValuePtr         pData;
};

typedef std::vector<SScriptGraphNodePort> ScriptGraphNodePorts;

class CScriptGraphNodeLayout   // #SchematycTODO : Move to separate cpp/h files?
{
public:

	CScriptGraphNodeLayout();

	void                        SetName(const char* szName);
	const char*                 GetName() const;
	void                        SetColor(EScriptGraphColor color);
	EScriptGraphColor           GetColor() const;
	ScriptGraphNodePorts&       GetInputs(); // #SchematycTODO : Rather than allowing non-const access should we just provide a Serialize() function?
	const ScriptGraphNodePorts& GetInputs() const;
	ScriptGraphNodePorts&       GetOutputs(); // #SchematycTODO : Rather than allowing non-const access should we just provide a Serialize() function?
	const ScriptGraphNodePorts& GetOutputs() const;

	void                        Exchange(CScriptGraphNodeLayout& rhs);

	inline void                 AddInput(const char* szName, const SGUID& typeGUID, const ScriptGraphPortFlags& flags)
	{
		AddInput(CGraphPortId::FromIdx(m_inputs.size()), szName, typeGUID, flags, CAnyValuePtr());
	}

	inline void AddInput(const CGraphPortId& id, const char* szName, const SGUID& typeGUID, const ScriptGraphPortFlags& flags)
	{
		AddInput(id, szName, typeGUID, flags, CAnyValuePtr());
	}

	inline void AddInputWithData(const char* szName, const SGUID& typeGUID, const ScriptGraphPortFlags& flags, const CAnyConstRef& value)
	{
		AddInput(CGraphPortId::FromIdx(m_inputs.size()), szName, typeGUID, flags, CAnyValue::CloneShared(value));
	}

	inline void AddInputWithData(const CGraphPortId& id, const char* szName, const SGUID& typeGUID, const ScriptGraphPortFlags& flags, const CAnyConstRef& value)
	{
		AddInput(id, szName, typeGUID, flags, CAnyValue::CloneShared(value));
	}

	inline void AddOutput(const char* szName, const SGUID& typeGUID, const ScriptGraphPortFlags& flags)
	{
		AddOutput(CGraphPortId::FromIdx(m_outputs.size()), szName, typeGUID, flags, CAnyValuePtr());
	}

	inline void AddOutput(const CGraphPortId& id, const char* szName, const SGUID& typeGUID, const ScriptGraphPortFlags& flags)
	{
		AddOutput(id, szName, typeGUID, flags, CAnyValuePtr());
	}

	inline void AddOutputWithData(const char* szName, const SGUID& typeGUID, const ScriptGraphPortFlags& flags, const CAnyConstRef& value)
	{
		AddOutput(CGraphPortId::FromIdx(m_outputs.size()), szName, typeGUID, flags, CAnyValue::CloneShared(value));
	}

	inline void AddOutputWithData(const CGraphPortId& id, const char* szName, const SGUID& typeGUID, const ScriptGraphPortFlags& flags, const CAnyConstRef& value)
	{
		AddOutput(id, szName, typeGUID, flags, CAnyValue::CloneShared(value));
	}

private:

	void AddInput(const CGraphPortId& id, const char* szName, const SGUID& typeGUID, const ScriptGraphPortFlags& flags, const CAnyValuePtr& pData);
	void AddOutput(const CGraphPortId& id, const char* szName, const SGUID& typeGUID, const ScriptGraphPortFlags& flags, const CAnyValuePtr& pData);

private:

	string               m_name;
	EScriptGraphColor    m_color;
	ScriptGraphNodePorts m_inputs;
	ScriptGraphNodePorts m_outputs;
};

class CScriptGraphNode : public IScriptGraphNode
{
public:

	CScriptGraphNode(const SGUID& guid, std::unique_ptr<CScriptGraphNodeModel> pModel);                  // #SchematycTODO : Make pModel the first parameter?
	CScriptGraphNode(const SGUID& guid, std::unique_ptr<CScriptGraphNodeModel> pModel, const Vec2& pos); // #SchematycTODO : Make pModel the first parameter?

	// IScriptGraphNode
	virtual void                 Attach(IScriptGraph& graph) override;
	virtual IScriptGraph&        GetGraph() override;
	virtual const IScriptGraph&  GetGraph() const override;
	virtual SGUID                GetTypeGUID() const override;
	virtual SGUID                GetGUID() const override;
	virtual const char*          GetName() const override;
	virtual ColorB               GetColor() const override;
	virtual ScriptGraphNodeFlags GetFlags() const override;
	virtual void                 SetPos(Vec2 pos) override;
	virtual Vec2                 GetPos() const override;

	// Input ports
	virtual uint32               GetInputCount() const override;
	virtual uint32               FindInputById(const CGraphPortId& id) const override;
	virtual CGraphPortId         GetInputId(uint32 inputIdx) const override;
	virtual const char*          GetInputName(uint32 inputIdx) const override;
	virtual SGUID                GetInputTypeGUID(uint32 inputIdx) const override;
	virtual ScriptGraphPortFlags GetInputFlags(uint32 inputIdx) const override;
	virtual CAnyConstPtr         GetInputData(uint32 inputIdx) const override;
	virtual ColorB               GetInputColor(uint32 inputIdx) const override;

	// Output ports
	virtual uint32               GetOutputCount() const override;
	virtual uint32               FindOutputById(const CGraphPortId& id) const override;
	virtual CGraphPortId         GetOutputId(uint32 outputIdx) const override;
	virtual const char*          GetOutputName(uint32 outputIdx) const override;
	virtual SGUID                GetOutputTypeGUID(uint32 outputIdx) const override;
	virtual ScriptGraphPortFlags GetOutputFlags(uint32 outputIdx) const override;
	virtual CAnyConstPtr         GetOutputData(uint32 outputIdx) const override;
	virtual ColorB               GetOutputColor(uint32 outputIdx) const override;

	//
	virtual void EnumerateDependencies(const ScriptDependencyEnumerator& enumerator, EScriptDependencyType type) const override;
	virtual void RemapDependencies(IGUIDRemapper& guidRemapper) override;
	virtual void ProcessEvent(const SScriptEvent& event) override;
	virtual void Serialize(Serialization::IArchive& archive) override;
	virtual void Copy(Serialization::IArchive& archive) override;
	virtual void Paste(Serialization::IArchive& archive) override;
	virtual void Validate(const Validator& validator) const override;
	virtual void Compile(SCompilerContext& context, IGraphNodeCompiler& compiler) const override;
	// ~IScriptGraphNode

	void   SetFlags(const ScriptGraphNodeFlags& flags);
	uint32 FindInputByName(const char* szName) const;
	uint32 FindOutputByName(const char* szName) const;

private:

	void RefreshLayout();

	void SerializeBasicInfo(Serialization::IArchive& archive);
	void SerializeInputs(Serialization::IArchive& archive);

private:

	SGUID                                  m_guid;
	std::unique_ptr<CScriptGraphNodeModel> m_pModel;
	Vec2                                   m_pos = Vec2(ZERO);
	ScriptGraphNodeFlags                   m_flags;
	CScriptGraphNodeLayout                 m_layout;
	IScriptGraph*                          m_pGraph = nullptr;
};
} // Schematyc
