// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryFlowGraph/IFlowSystem.h>

//////////////////////////////////////////////////////////////////////////
// Enum used for templating node base class
//////////////////////////////////////////////////////////////////////////
enum ENodeCloneType
{
	eNCT_Singleton, // node has only one instance; not cloned
	eNCT_Instanced, // new instance of the node will be created
};

//////////////////////////////////////////////////////////////////////////
// Auto registration for flow nodes. Handles both instanced and
//	singleton nodes
//////////////////////////////////////////////////////////////////////////
class CAutoRegFlowNodeBase : public IFlowNodeFactory
{
public:
	CAutoRegFlowNodeBase(const char* sClassName)
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

	virtual ~CAutoRegFlowNodeBase()
	{
		CAutoRegFlowNodeBase* node = m_pFirst;
		CAutoRegFlowNodeBase* prev = nullptr;
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

	void         AddRef()  {}
	void         Release() {}

	void         Reset()   {}

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		SIZER_SUBCOMPONENT_NAME(s, "CAutoRegFlowNodeBase");
	}

	//////////////////////////////////////////////////////////////////////////
	const char*                  m_sClassName;
	CAutoRegFlowNodeBase*        m_pNext;
	static CAutoRegFlowNodeBase* m_pFirst;
	static CAutoRegFlowNodeBase* m_pLast;
	//////////////////////////////////////////////////////////////////////////
};

//////////////////////////////////////////////////////////////////////////
template<class T>
class CAutoRegFlowNode : public CAutoRegFlowNodeBase
{
public:
	CAutoRegFlowNode(const char* sClassName)
		: CAutoRegFlowNodeBase(sClassName)
	{
	}

	IFlowNodePtr Create(IFlowNode::SActivationInfo* pActInfo)
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
		else
		{
			assert(false);
		}
	}

private:
	IFlowNodePtr m_pInstance; // only applies for singleton nodes
};

#if CRY_PLATFORM_WINDOWS && defined(_LIB)
	#define CRY_EXPORT_STATIC_LINK_VARIABLE(Var)                        \
	  extern "C" { INT_PTR lib_func_ ## Var() { return (INT_PTR)&Var; } \
	  }                                                                 \
	  __pragma(message("#pragma comment(linker,\"/include:_lib_func_" # Var "\")"))
#else
	#define CRY_EXPORT_STATIC_LINK_VARIABLE(Var)
#endif

//////////////////////////////////////////////////////////////////////////
// Use this define to register a new flow node class. Handles
//	both instanced and singleton nodes
// Ex. REGISTER_FLOW_NODE( "Delay",CFlowDelayNode )
//////////////////////////////////////////////////////////////////////////
#define REGISTER_FLOW_NODE(FlowNodeClassName, FlowNodeClass)                     \
  CAutoRegFlowNode<FlowNodeClass> g_AutoReg ## FlowNodeClass(FlowNodeClassName); \
  CRY_EXPORT_STATIC_LINK_VARIABLE(g_AutoReg ## FlowNodeClass);

// macro similar to REGISTER_FLOW_NODE, but allows a different name for registration
#define REGISTER_FLOW_NODE_EX(FlowNodeClassName, FlowNodeClass, RegName)   \
  CAutoRegFlowNode<FlowNodeClass> g_AutoReg ## RegName(FlowNodeClassName); \
  CRY_EXPORT_STATIC_LINK_VARIABLE(g_AutoReg ## RegName);

//////////////////////////////////////////////////////////////////////////
// Internal base class for flow nodes. Node classes shouldn't be
//	derived directly from this, but should derive from CFlowBaseNode<>
//////////////////////////////////////////////////////////////////////////
class CFlowBaseNodeInternal : public IFlowNode
{
	template<ENodeCloneType CLONE_TYPE> friend class CFlowBaseNode;
public:
	// private ctor/dtor to prevent classes directly derived from this;
	//	the exception is CFlowBaseNode (friended above)
	CFlowBaseNodeInternal() { m_refs = 0; };
	virtual ~CFlowBaseNodeInternal() {}

	//////////////////////////////////////////////////////////////////////////
	// IFlowNode
	virtual void         AddRef()                                                  { ++m_refs; };
	virtual void         Release()                                                 { if (0 >= --m_refs) delete this; };

	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo)                          = 0;
	virtual bool         SerializeXML(SActivationInfo*, const XmlNodeRef&, bool)   { return true; }
	virtual void         Serialize(SActivationInfo*, TSerialize ser)               {}
	virtual void         PostSerialize(SActivationInfo*)                           {}
	virtual void         ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) {};

private:
	int m_refs;
};

//////////////////////////////////////////////////////////////////////////
// Base class that nodes can derive from.
//	eg1: Singleton node:
//	class CSingletonNode : public CFlowBaseNode<eNCT_Singleton>
//
//	eg2: Instanced node:
//	class CInstancedNode : public CFlowBaseNode<eNCT_Instanced>
//
//	Instanced nodes must override Clone (pure virtual in base), while
//	Singleton nodes cannot - will result in a compile error.
//////////////////////////////////////////////////////////////////////////

template<ENodeCloneType CLONE_TYPE>
class CFlowBaseNode : public CFlowBaseNodeInternal
{
public:
	CFlowBaseNode() : CFlowBaseNodeInternal() {}

	static const int myCloneType = CLONE_TYPE;
};

// specialization for instanced nodes: Clone() is purely virtual to force derived classes to override it
template<>
class CFlowBaseNode<eNCT_Instanced> : public CFlowBaseNodeInternal
{
public:
	CFlowBaseNode() : CFlowBaseNodeInternal() {}

	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) = 0;

	static const int myCloneType = eNCT_Instanced;
};

// specialization for singleton nodes: Clone() is marked final so that derived classes cannot override it.
template<>
class CFlowBaseNode<eNCT_Singleton> : public CFlowBaseNodeInternal
{
public:
	CFlowBaseNode() : CFlowBaseNodeInternal() {}

	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) final { return this; }

	static const int myCloneType = eNCT_Singleton;
};
