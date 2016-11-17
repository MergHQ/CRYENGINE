// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace Schematyc
{
template<typename SIGNATURE> class CDelegate;

template<typename RETURN_TYPE, typename ... PARAM_TYPES> class CDelegate<RETURN_TYPE(PARAM_TYPES ...)>
{
private:

	typedef RETURN_TYPE (* StubPtr)(void*, PARAM_TYPES ...);

	struct SGlobalFunctionStub
	{
	public:
		typedef RETURN_TYPE (* FunctionPtr)(PARAM_TYPES ...);

		static SGlobalFunctionStub& GetInstance()
		{
			static SGlobalFunctionStub s_instance;
			return s_instance;
		}

		static RETURN_TYPE Invoke(void* pObject, PARAM_TYPES ... args)
		{
			return (*(GetInstance().pFunction))(args ...);
		}

		FunctionPtr pFunction;
	};

	template<typename OBJECT_TYPE> struct SMemberFunctionStub
	{
		typedef RETURN_TYPE (OBJECT_TYPE::* FunctionPtr)(PARAM_TYPES ...);

		static RETURN_TYPE Invoke(void* pObject, PARAM_TYPES ... args)
		{
			return (static_cast<OBJECT_TYPE*>(pObject)->*(GetInstance().pFunction))(args ...);
		}

		static SMemberFunctionStub& GetInstance()
		{
			static SMemberFunctionStub s_instance;
			return s_instance;
		}

		FunctionPtr pFunction;
	};

	template<typename OBJECT_TYPE> struct SMemberFunctionStub<const OBJECT_TYPE>
	{
		typedef RETURN_TYPE (OBJECT_TYPE::* FunctionPtr)(PARAM_TYPES ...) const;

		static RETURN_TYPE Invoke(void* pObject, PARAM_TYPES ... args)
		{
			return (static_cast<OBJECT_TYPE*>(pObject)->*(GetInstance().pFunction))(args ...);
		}

		static SMemberFunctionStub& GetInstance()
		{
			static SMemberFunctionStub s_instance;
			return s_instance;
		}

		FunctionPtr pFunction;
	};

public:

	inline CDelegate()
		: m_pStub(nullptr)
		, m_pObject(nullptr)
	{}

	inline CDelegate(StubPtr pStub, void* pObject)
		: m_pStub(pStub)
		, m_pObject(pObject)
	{}

	inline CDelegate(const CDelegate& rhs)
	{
		Copy(rhs);
	}

	inline bool IsEmpty() const
	{
		return m_pStub == nullptr;
	}

	inline RETURN_TYPE operator()(PARAM_TYPES ... args) const
	{
		return (*m_pStub)(m_pObject, args ...);
	}

	inline CDelegate& operator=(const CDelegate& rhs)
	{
		Copy(rhs);
		return *this;
	}

	inline bool operator<(const CDelegate& rhs) const
	{
		return (m_pObject < rhs.m_pObject) || ((m_pObject == rhs.m_pObject) && (m_pStub < rhs.m_pStub));
	}

	inline bool operator>(const CDelegate& rhs) const
	{
		return (m_pObject > rhs.m_pObject) || ((m_pObject == rhs.m_pObject) && (m_pStub > rhs.m_pStub));
	}

	inline bool operator<=(const CDelegate& rhs) const
	{
		return operator<(*this, rhs) || operator==(*this, rhs);
	}

	inline bool operator>=(const CDelegate& rhs) const
	{
		return operator>(*this, rhs) || operator==(*this, rhs);
	}

	inline bool operator==(const CDelegate& rhs) const
	{
		return (m_pStub == rhs.m_pStub) && (m_pObject == rhs.m_pObject);
	}

	inline bool operator!=(const CDelegate& rhs) const
	{
		return (m_pStub != rhs.m_pStub) || (m_pObject != rhs.m_pObject);
	}

	static inline CDelegate FromGlobalFunction(RETURN_TYPE (* functionPointer)(PARAM_TYPES ...))
	{
		SGlobalFunctionStub::GetInstance().pFunction = functionPointer;
		return CDelegate(&SGlobalFunctionStub::Invoke, nullptr);
	}

	template<typename OBJECT_TYPE> static inline CDelegate FromMemberFunction(OBJECT_TYPE& object, RETURN_TYPE (OBJECT_TYPE::* functionPointer)(PARAM_TYPES ...))
	{
		typedef SMemberFunctionStub<OBJECT_TYPE> Stub;

		Stub::GetInstance().pFunction = functionPointer;
		return CDelegate(&Stub::Invoke, &object);
	}

	template<typename OBJECT_TYPE> static inline CDelegate FromConstMemberFunction(const OBJECT_TYPE& object, RETURN_TYPE (OBJECT_TYPE::* functionPointer)(PARAM_TYPES ...) const)
	{
		typedef SMemberFunctionStub<const OBJECT_TYPE> Stub;

		Stub::GetInstance().pFunction = functionPointer;
		return CDelegate(&Stub::Invoke, &const_cast<OBJECT_TYPE&>(object));
	}

	template<typename CLOSURE> static inline CDelegate FromLambda(CLOSURE& closure)
	{
		return CDelegate(&CDelegate::InvokeLambda<CLOSURE>, &closure);
	}

private:

	inline void Copy(const CDelegate& rhs)
	{
		m_pStub = rhs.m_pStub;
		m_pObject = rhs.m_pObject;
	}

	template<typename CLOSURE> static RETURN_TYPE InvokeLambda(void* pObject, PARAM_TYPES ... args)
	{
		return (*reinterpret_cast<CLOSURE*>(pObject))(args ...);
	}

private:

	StubPtr m_pStub;
	void*   m_pObject;
};

namespace Delegate
{
template<typename RETURN_TYPE, typename ... PARAM_TYPES> static inline CDelegate<RETURN_TYPE(PARAM_TYPES ...)> Make(RETURN_TYPE (* FUNCTION_PTR)(PARAM_TYPES ...))
{
	return CDelegate<RETURN_TYPE(PARAM_TYPES ...)>::FromGlobalFunction(FUNCTION_PTR);
}

template<typename OBJECT_TYPE, typename RETURN_TYPE, typename ... PARAM_TYPES> static inline CDelegate<RETURN_TYPE(PARAM_TYPES ...)> Make(OBJECT_TYPE& object, RETURN_TYPE (OBJECT_TYPE::* FUNCTION_PTR)(PARAM_TYPES ...))
{
	return CDelegate<RETURN_TYPE(PARAM_TYPES ...)>::FromMemberFunction(object, FUNCTION_PTR);
}

template<typename OBJECT_TYPE, typename RETURN_TYPE, typename ... PARAM_TYPES> static inline CDelegate<RETURN_TYPE(PARAM_TYPES ...)> Make(OBJECT_TYPE& object, RETURN_TYPE (OBJECT_TYPE::* FUNCTION_PTR)(PARAM_TYPES ...) const)
{
	return CDelegate<RETURN_TYPE(PARAM_TYPES ...)>::FromConstMemberFunction(object, FUNCTION_PTR);
}
} // Delegate
} // Schematyc
