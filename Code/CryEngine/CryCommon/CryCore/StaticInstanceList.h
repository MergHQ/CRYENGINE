// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------

   Description: Static instance list. A utility class for automatic
             registration of types/data during static initialization.

   -------------------------------------------------------------------------
   History:
   - 08:06:2012: Created by Paul Slinger

*************************************************************************/

#pragma once

template<typename TYPE> class CStaticInstanceList
{
public:

	CStaticInstanceList(const CStaticInstanceList&) = delete;
	CStaticInstanceList(CStaticInstanceList&&) = delete;

	inline CStaticInstanceList()
	{
		CStaticInstanceList*& head = Head();
		this->m_pNext = head;
		head = this;
	}

	virtual ~CStaticInstanceList() {}

	static inline TYPE* GetFirstInstance()
	{
		return static_cast<TYPE*>(Head());
	}

	inline TYPE* GetNextInstance() const
	{
		return static_cast<TYPE*>(m_pNext);
	}

private:

	static inline CStaticInstanceList*& Head()
	{
		static CStaticInstanceList* pHead;
		return pHead;
	}

private:

	CStaticInstanceList* m_pNext;
};

#define CRY_PP_JOIN_XY_(x, y)                         x ## y
#define CRY_PP_JOIN_XY(x, y)                          CRY_PP_JOIN_XY_(x, y)

#define CRY_STATIC_AUTO_REGISTER_FUNCTION(CallbackFunctionPtr) \
	static Detail::CStaticAutoRegistrar<Detail::FunctionFirstArgType<decltype(CallbackFunctionPtr)>::type> \
		CRY_PP_JOIN_XY(g_cry_static_AutoRegistrar, __COUNTER__)(CallbackFunctionPtr);

namespace Detail
{
template <typename F>             struct FunctionFirstArgType;
template <typename R, typename T> struct FunctionFirstArgType<R(*)(T)> { typedef T type; };
template <typename R>             struct FunctionFirstArgType<R(*)(void)> { typedef void type; };

template <typename ... PARAM_TYPE>
class CStaticAutoRegistrar : public CStaticInstanceList< CStaticAutoRegistrar<PARAM_TYPE...> >
{
	typedef void (*CallbackPtr)(PARAM_TYPE...);
	CallbackPtr m_pCallback;

public:
	CStaticAutoRegistrar(CallbackPtr pCallback) : m_pCallback(pCallback) {}

	static void InvokeStaticCallbacks(PARAM_TYPE ... params)
	{
		for (const CStaticAutoRegistrar* pInstance = CStaticInstanceList<CStaticAutoRegistrar<PARAM_TYPE...>>::GetFirstInstance(); pInstance; pInstance = pInstance->GetNextInstance())
		{
			(*pInstance->m_pCallback)(std::forward<PARAM_TYPE>(params)...);
		}
	}
};

template <>
class CStaticAutoRegistrar<void> : public CStaticInstanceList< CStaticAutoRegistrar<void> >
{
	typedef void(*CallbackPtr)();
	CallbackPtr m_pCallback;
public:
	CStaticAutoRegistrar(CallbackPtr pCallback) : m_pCallback(pCallback) {}
	
	static void InvokeStaticCallbacks()
	{
		for (const CStaticAutoRegistrar* pInstance = GetFirstInstance(); pInstance; pInstance = pInstance->GetNextInstance())
		{
			(*pInstance->m_pCallback)();
		}
	}
};

} // Detail

template <typename T>
static void CryInvokeStaticCallbacks(T param)
{
	Detail::CStaticAutoRegistrar<T>::InvokeStaticCallbacks(param);
}