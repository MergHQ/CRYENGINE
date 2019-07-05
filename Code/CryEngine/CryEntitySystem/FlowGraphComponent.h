#pragma once

#include <CryFlowGraph/IFlowBaseNode.h>
#include <CrySchematyc/Env/IEnvRegistry.h>
#include <CrySchematyc/Env/Elements/IEnvComponent.h>
#include <CrySchematyc/Env/Elements/EnvFunction.h>
#include <CrySchematyc/Utils/Any.h>
#include <CrySchematyc/Utils/SharedString.h>
#include <CrySchematyc/Env/Elements/EnvClass.h>
#include <CrySchematyc/IObject.h>

class CEntityComponentSignalFlowNode final : public CFlowBaseNode<eNCT_Singleton>, public Schematyc::IObjectSignalListener
{
public:
	explicit CEntityComponentSignalFlowNode(const Schematyc::IEnvComponent& envComponent, const Schematyc::IEnvSignal& signal);
	~CEntityComponentSignalFlowNode();

	// CFlowBaseNode
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) final;
	virtual void GetMemoryUsage(ICrySizer* pSizer) const final { pSizer->Add(*this); }
	virtual void GetConfiguration(SFlowNodeConfig& config) final;
	// ~CFlowBaseNode

	// IObjectSignalListener
	virtual void ProcessSignal(const Schematyc::SObjectSignal& signal) final;
	// ~IObjectSignalListener

protected:
	void CreateSchematycEntity();

	const Schematyc::IEnvComponent& m_envComponent;
	const Schematyc::IEnvSignal& m_envSignal;

	IEntity* m_pCurrentEntity = nullptr;
	int m_currentComponentIndex = 0;
	
	SActivationInfo m_activationInfo;

	std::vector<SInputPortConfig> m_inputs;
	std::vector<SOutputPortConfig> m_outputs;
};

class CEntityComponentGetterFlowNode final : public CFlowBaseNode<eNCT_Singleton>
{
public:
	explicit CEntityComponentGetterFlowNode(const Schematyc::CClassMemberDescArray& memberDescArray, const Schematyc::IEnvComponent& component);

	// CFlowBaseNode
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) final;
	virtual void GetMemoryUsage(ICrySizer* pSizer) const final { pSizer->Add(*this); }
	virtual void GetConfiguration(SFlowNodeConfig& config) final;
	// ~CFlowBaseNode

protected:
	const Schematyc::IEnvComponent& m_envComponent;
	const Schematyc::CClassMemberDescArray& m_memberDescArray;

	std::vector<SInputPortConfig> m_inputs;
	std::vector<SOutputPortConfig> m_outputs;
};

class CEntityComponentSetterFlowNode final : public CFlowBaseNode<eNCT_Singleton>
{
public:
	explicit CEntityComponentSetterFlowNode(const Schematyc::CClassMemberDescArray& memberDescArray, const Schematyc::IEnvComponent& component);

	// CFlowBaseNode
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) final;
	virtual void GetMemoryUsage(ICrySizer* pSizer) const final { pSizer->Add(*this); }
	virtual void GetConfiguration(SFlowNodeConfig& config) final;
	// ~CFlowBaseNode

protected:
	const Schematyc::IEnvComponent& m_envComponent;
	const Schematyc::CClassMemberDescArray& m_memberDescArray;

	std::vector<SInputPortConfig> m_inputs;
};

class CEntityComponentFunctionFlowNode final : public CFlowBaseNode<eNCT_Singleton>
{
public:
	CEntityComponentFunctionFlowNode(SActivationInfo* pActInfo, const Schematyc::IEnvComponent& component, const Schematyc::CEnvFunction& function);

	// CFlowBaseNode
	virtual void GetMemoryUsage(ICrySizer* pSizer) const final { pSizer->Add(*this); }
	virtual void GetConfiguration(SFlowNodeConfig& config) final;
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) final;
	// ~CFlowBaseNode

protected:
	const Schematyc::IEnvComponent& m_envComponent;
	const Schematyc::CEnvFunction&  m_envFunction;

	std::vector<SInputPortConfig> m_inputs;
};

class CEntityComponentFlowNodeFactory final : public IFlowNodeFactory
{
public:
	CEntityComponentFlowNodeFactory(const Schematyc::IEnvComponent& component, const Schematyc::CEnvFunction& function) : m_envComponent(component), m_envFunction(function) {}
	virtual ~CEntityComponentFlowNodeFactory() = default;

	// IFlowNodeFactory
	virtual IFlowNodePtr Create(IFlowNode::SActivationInfo* pActInfo) override { return new CEntityComponentFunctionFlowNode(pActInfo, m_envComponent, m_envFunction); }
	virtual void         GetMemoryUsage(ICrySizer* pSizer) const override 
	{
		pSizer->Add(*this);
	}
	virtual void         Reset() override {}
	// ~IFlowNodeFactory

protected:
	const Schematyc::IEnvComponent& m_envComponent;
	const Schematyc::CEnvFunction&  m_envFunction;
};

class CEntityComponentFlowNodeFactoryGetter final : public IFlowNodeFactory
{
public:
	CEntityComponentFlowNodeFactoryGetter(const Schematyc::IEnvComponent& component, const Schematyc::CClassMemberDescArray& memberClass) : m_envComponent(component), m_memberDescArray(memberClass) {}
	virtual ~CEntityComponentFlowNodeFactoryGetter() = default;

	// IFlowNodeFactory
	virtual IFlowNodePtr Create(IFlowNode::SActivationInfo* pActInfo) override { return new CEntityComponentGetterFlowNode(m_memberDescArray, m_envComponent); }
	virtual void         GetMemoryUsage(ICrySizer* pSizer) const override
	{
		pSizer->Add(*this);
	}
	virtual void         Reset() override {}
	// ~IFlowNodeFactory

protected:
	const Schematyc::IEnvComponent& m_envComponent;
	const Schematyc::CClassMemberDescArray& m_memberDescArray;
};

class CEntityComponentFlowNodeFactorySetter final : public IFlowNodeFactory
{
public:
	CEntityComponentFlowNodeFactorySetter(const Schematyc::IEnvComponent& component, const Schematyc::CClassMemberDescArray& memberClass) : m_envComponent(component), m_memberDescArray(memberClass) {}
	virtual ~CEntityComponentFlowNodeFactorySetter() = default;

	// IFlowNodeFactory
	virtual IFlowNodePtr Create(IFlowNode::SActivationInfo* pActInfo) override { return new CEntityComponentSetterFlowNode(m_memberDescArray, m_envComponent); }
	virtual void         GetMemoryUsage(ICrySizer* pSizer) const override
	{
		pSizer->Add(*this);
	}
	virtual void         Reset() override {}
	// ~IFlowNodeFactory

protected:
	const Schematyc::IEnvComponent& m_envComponent;
	const Schematyc::CClassMemberDescArray& m_memberDescArray;
};

class CEntityComponentFlowNodeFactorySignal final : public IFlowNodeFactory
{
public:
	CEntityComponentFlowNodeFactorySignal(const Schematyc::IEnvComponent& component, const Schematyc::IEnvSignal& signal) : m_envComponent(component), m_envSignal(signal) {}
	virtual ~CEntityComponentFlowNodeFactorySignal() = default;

	//IFlowNodeFactory
	virtual IFlowNodePtr Create(IFlowNode::SActivationInfo* pActInfo) override { return new CEntityComponentSignalFlowNode(m_envComponent, m_envSignal);  }
	virtual void         Reset() override {}
	virtual void         GetMemoryUsage(ICrySizer* pSizer) const override
	{
		pSizer->Add(*this);
	}
	//~IFlowNodeFactory

protected:
	const Schematyc::IEnvComponent& m_envComponent;
	const Schematyc::IEnvSignal& m_envSignal;
};
