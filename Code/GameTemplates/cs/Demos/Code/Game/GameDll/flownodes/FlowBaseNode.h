// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.


#pragma once


#include <CryFlowGraph/IFlowSystem.h>


enum ENodeCloneType
{
	eNCT_Singleton,		// node has only one instance; not cloned
	eNCT_Instanced,		// new instance of the node will be created
	eNCT_Cloned			// node clone method will be called
};

class CAutoRegFlowNodeBaseZero : public IFlowNodeFactory
{
public:
	CAutoRegFlowNodeBaseZero(const char* sClassName)
	{
		m_sClassName = sClassName;
		m_pNext = 0;
		if (!m_pLast)
		{
			m_pFirst = this;
		}
		else
		{
			m_pLast->m_pNext = this;
		}

		m_pLast = this;
	}

	virtual ~CAutoRegFlowNodeBaseZero()
	{
		CAutoRegFlowNodeBaseZero* node = m_pFirst;
		CAutoRegFlowNodeBaseZero* prev = nullptr;
		while (node && node != this)
		{
			prev = node;
			node = node->m_pNext;
		}

		assert(node);
		if (node)
		{
			if (prev)
				prev->m_pNext = m_pNext;
			else
				m_pFirst = m_pNext;

			if (m_pLast == this)
				m_pLast = prev;
		}
	}

	void AddRef() override {}
	void Release() override {}

	void Reset() override {}

	virtual void GetMemoryUsage(ICrySizer * s) const override
	{
		SIZER_SUBCOMPONENT_NAME(s, "CAutoRegFlowNodeZero");
	}

	const char *m_sClassName;
	CAutoRegFlowNodeBaseZero* m_pNext;
	static CAutoRegFlowNodeBaseZero *m_pFirst;
	static CAutoRegFlowNodeBaseZero *m_pLast;
};

template <class T>
class CAutoRegFlowNode : public CAutoRegFlowNodeBaseZero
{
public:
	CAutoRegFlowNode(const char *sClassName)
		: CAutoRegFlowNodeBaseZero(sClassName)
	{
	}

	IFlowNodePtr Create(IFlowNode::SActivationInfo * pActInfo) override
	{
		PREFAST_SUPPRESS_WARNING(6326)
			if (T::myCloneType == eNCT_Singleton)
			{
				if (!m_pInstance.get())
					m_pInstance = new T(pActInfo);

				return m_pInstance;
			}
			else if (T::myCloneType == eNCT_Instanced)
			{
				return new T(pActInfo);
			}
			else if (T::myCloneType == eNCT_Cloned)
			{
				if (m_pInstance.get())
					return m_pInstance->Clone(pActInfo);
				else
				{
					m_pInstance = new T(pActInfo);
					return m_pInstance;
				}
			}
			else
			{
				assert(false);
			}
	}

private:
	IFlowNodePtr m_pInstance;	// only applies for singleton nodes
};

#if CRY_PLATFORM_WINDOWS && defined(_LIB)
#define CRY_EXPORT_STATIC_LINK_VARIABLE( Var ) \
	extern "C" { int lib_func_##Var() { return (int)&Var; } } \
	__pragma( message("#pragma comment(linker,\"/include:_lib_func_"#Var"\")") )
#else
#define CRY_EXPORT_STATIC_LINK_VARIABLE( Var )
#endif


#define REGISTER_FLOW_NODE( FlowNodeClassName,FlowNodeClass ) \
	CAutoRegFlowNode<FlowNodeClass> g_AutoReg##FlowNodeClass ( FlowNodeClassName );\
	CRY_EXPORT_STATIC_LINK_VARIABLE( g_AutoReg##FlowNodeClass )

#define REGISTER_FLOW_NODE_EX( FlowNodeClassName,FlowNodeClass,RegName ) \
	CAutoRegFlowNode<FlowNodeClass> g_AutoReg##RegName ( FlowNodeClassName );\
	CRY_EXPORT_STATIC_LINK_VARIABLE( g_AutoReg##RegName )


class CFlowBaseNodeInternal : public IFlowNode
{
	template <ENodeCloneType CLONE_TYPE> friend class CFlowBaseNode;

private:
	// private ctor/dtor to prevent classes directly derived from this;
	//	the exception is CFlowBaseNode (friended above)
	CFlowBaseNodeInternal() { m_refs = 0; };
	virtual ~CFlowBaseNodeInternal() {}

public:
	//IFlowNode
	virtual void AddRef() override { ++m_refs; };
	virtual void Release() override { if (0 >= --m_refs)	delete this; };

	virtual IFlowNodePtr Clone(SActivationInfo *pActInfo) override = 0;
	virtual bool SerializeXML(SActivationInfo *, const XmlNodeRef&, bool) override { return true; }
	virtual void Serialize(SActivationInfo *, TSerialize ser) override {}
	virtual void PostSerialize(SActivationInfo *) override {}
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo) override {};
	//~IFlowNode

protected:
	bool InputEntityIsLocalPlayer(const SActivationInfo* const pActInfo) const
	{
		bool bRet = true;

		if (pActInfo->pEntity)
		{
			bRet = (pActInfo->pEntity->GetId() == LOCAL_PLAYER_ENTITY_ID);
		}
		else
		{
			if (gEnv->bMultiplayer)
				bRet = false;
		}

		return bRet;
	}

private:
	int m_refs;
};


template <ENodeCloneType CLONE_TYPE>
class CFlowBaseNode : public CFlowBaseNodeInternal
{
public:
	CFlowBaseNode() : CFlowBaseNodeInternal() {}

	static const int myCloneType = CLONE_TYPE;
};

// specialization for singleton nodes: Clone() is
//	marked so derived classes cannot override it.
template <>
class CFlowBaseNode<eNCT_Singleton> : public CFlowBaseNodeInternal
{
public:
	CFlowBaseNode<eNCT_Singleton>() : CFlowBaseNodeInternal()	{}

	virtual IFlowNodePtr Clone(SActivationInfo *pActInfo) final { return this; }

	static const int myCloneType = eNCT_Singleton;
};
