// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#define SCHEMATYC_DELEGATE(function)                Schematyc::Delegate::SHelper<decltype(&function)>::Make<&function>()
#define SCHEMATYC_MEMBER_DELEGATE(function, object) Schematyc::Delegate::SHelper<decltype(&function)>::Make<&function>(object)

namespace Schematyc
{
template<typename SIGNATURE> class CDelegate;

template<typename RETURN_TYPE, typename ... PARAM_TYPES> class CDelegate<RETURN_TYPE(PARAM_TYPES ...)>
{
private:

	typedef RETURN_TYPE(*StubPtr)(void*, PARAM_TYPES ...);

public:

	inline CDelegate()
		: m_pStub(nullptr)
		, m_pObject(nullptr)
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

	template <RETURN_TYPE(*FUNCTION_PTR)(PARAM_TYPES ...)> static inline CDelegate FromFunction()
	{
		return CDelegate(&CDelegate::InvokeFunction<FUNCTION_PTR>);
	}

	template <typename OBJECT_TYPE, RETURN_TYPE(OBJECT_TYPE::*FUNCTION_PTR)(PARAM_TYPES ...)> static inline CDelegate FromMemberFunction(OBJECT_TYPE& object)
	{
		return CDelegate(&CDelegate::InvokeMemberFunction<OBJECT_TYPE, FUNCTION_PTR>, &object);
	}

	template <typename OBJECT_TYPE, RETURN_TYPE(OBJECT_TYPE::*FUNCTION_PTR)(PARAM_TYPES ...) const> static inline CDelegate FromConstMemberFunction(const OBJECT_TYPE& object)
	{
		return CDelegate(&CDelegate::InvokeConstMemberFunction<OBJECT_TYPE, FUNCTION_PTR>, const_cast<OBJECT_TYPE*>(&object));
	}

	template<typename CLOSURE> static inline CDelegate FromLambda(CLOSURE& closure)
	{
		return CDelegate(&CDelegate::InvokeLambda<CLOSURE>, &closure);
	}

private:

	inline CDelegate(StubPtr pStub, void* pObject = nullptr)
		: m_pStub(pStub)
		, m_pObject(pObject)
	{}

	inline void Copy(const CDelegate& rhs)
	{
		m_pStub = rhs.m_pStub;
		m_pObject = rhs.m_pObject;
	}

	template<RETURN_TYPE(*FUNCTION_PTR)(PARAM_TYPES ...)> static RETURN_TYPE InvokeFunction(void* pObject, PARAM_TYPES ... args)
	{
		return (*FUNCTION_PTR)(args ...);
	}

	template<typename OBJECT_TYPE, RETURN_TYPE(OBJECT_TYPE::*FUNCTION_PTR)(PARAM_TYPES ...)> static RETURN_TYPE InvokeMemberFunction(void* pObject, PARAM_TYPES ... args)
	{
		return ((static_cast<OBJECT_TYPE*>(pObject))->*FUNCTION_PTR)(args ...);
	}

	template<typename OBJECT_TYPE, RETURN_TYPE(OBJECT_TYPE::*FUNCTION_PTR)(PARAM_TYPES ...) const> static RETURN_TYPE InvokeConstMemberFunction(void* pObject, PARAM_TYPES ... args)
	{
		return ((static_cast<const OBJECT_TYPE*>(pObject))->*FUNCTION_PTR)(args ...);
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
template <typename TYPE> struct SHelper;

template<typename RETURN_TYPE, typename ... PARAM_TYPES> struct SHelper<RETURN_TYPE(*)(PARAM_TYPES ...)>
{
	typedef CDelegate<RETURN_TYPE(PARAM_TYPES ...)> Type;

	template<RETURN_TYPE(*FUNCTION_PTR)(PARAM_TYPES ...)> static inline Type Make()
	{
		return Type::template FromFunction<FUNCTION_PTR>();
	}
};

template<typename OBJECT_TYPE, typename RETURN_TYPE, typename ... PARAM_TYPES> struct SHelper<RETURN_TYPE(OBJECT_TYPE::*)(PARAM_TYPES ...)>
{
	typedef CDelegate<RETURN_TYPE(PARAM_TYPES ...)> Type;

	template<RETURN_TYPE(OBJECT_TYPE::*FUNCTION_PTR)(PARAM_TYPES ...)> static inline Type Make(OBJECT_TYPE& object)
	{
		return Type::template FromMemberFunction<OBJECT_TYPE, FUNCTION_PTR>(object);
	}
};

template<typename OBJECT_TYPE, typename RETURN_TYPE, typename ... PARAM_TYPES> struct SHelper<RETURN_TYPE(OBJECT_TYPE::*)(PARAM_TYPES ...) const>
{
	typedef CDelegate<RETURN_TYPE(PARAM_TYPES ...)> Type;

	template<RETURN_TYPE(OBJECT_TYPE::*FUNCTION_PTR)(PARAM_TYPES ...) const> static inline Type Make(const OBJECT_TYPE& object)
	{
		return Type::template FromConstMemberFunction<OBJECT_TYPE, FUNCTION_PTR>(object);
	}
};
} // Delegate
} // Schematyc
