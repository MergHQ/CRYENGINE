// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <vector>

#include "CryFunction.h"

namespace CrySignalPrivate
{
template<typename Signature>
struct Observer
{
	std::function<Signature> m_function;
	uintptr_t                m_id;

	template<typename Object, typename MemberFunction>
	void SetMemberFunction(Object* const pObject, MemberFunction function)
	{
		m_function = function_cast<Signature>(WrapMemberFunction(pObject, function));
	}

	template<typename Function>
	void SetFunction(Function& function)
	{
		//If you get an error here, wrap your parameter in std::function<ParamSignature>(yourParam)
		//TODO : improve
		m_function = function_cast<Signature>(function);
	}
};

template<typename Signature> class CrySignalBase
{
};

template<>
class CrySignalBase<void()>
{
public:
	void operator()() const
	{
		//Iterating is done as such on purpose to avoid problems if the signal is resized during iteration
		const int count = m_observers.size();
		for (int i = count - 1; i >= 0; i--)
		{
			m_observers[i].m_function();
		}
	}

protected:
	typedef Observer<void ()> TObserver;
	std::vector<TObserver> m_observers;
};

template<typename Arg1>
class CrySignalBase<void(Arg1)>
{
public:
	void operator()(Arg1 arg1) const
	{
		const int count = m_observers.size();
		for (int i = count - 1; i >= 0; i--)
		{
			m_observers[i].m_function(arg1);
		}
	}

protected:
	typedef Observer<void (Arg1)> TObserver;
	std::vector<TObserver> m_observers;
};

template<typename Arg1, typename Arg2>
class CrySignalBase<void(Arg1, Arg2)>
{
public:
	void operator()(Arg1 arg1, Arg2 arg2) const
	{
		const int count = m_observers.size();
		for (int i = count - 1; i >= 0; i--)
		{
			m_observers[i].m_function(arg1, arg2);
		}
	}

protected:
	typedef Observer<void (Arg1, Arg2)> TObserver;
	std::vector<TObserver> m_observers;
};

template<typename Arg1, typename Arg2, typename Arg3>
class CrySignalBase<void(Arg1, Arg2, Arg3)>
{
public:
	void operator()(Arg1 arg1, Arg2 arg2, Arg3 arg3) const
	{
		const int count = m_observers.size();
		for (int i = count - 1; i >= 0; i--)
		{
			m_observers[i].m_function(arg1, arg2, arg3);
		}
	}

protected:
	typedef Observer<void (Arg1, Arg2, Arg3)> TObserver;
	std::vector<TObserver> m_observers;
};

template<typename Arg1, typename Arg2, typename Arg3, typename Arg4>
class CrySignalBase<void(Arg1, Arg2, Arg3, Arg4)>
{
public:
	void operator()(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) const
	{
		const int count = m_observers.size();
		for (int i = count - 1; i >= 0; i--)
		{
			m_observers[i].m_function(arg1, arg2, arg3, arg4);
		}
	}

protected:
	typedef Observer<void (Arg1, Arg2, Arg3, Arg4)> TObserver;
	std::vector<TObserver> m_observers;
};

template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
class CrySignalBase<void(Arg1, Arg2, Arg3, Arg4, Arg5)>
{
public:
	void operator()(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5) const
	{
		const int count = m_observers.size();
		for (int i = count - 1; i >= 0; i--)
		{
			m_observers[i].m_function(arg1, arg2, arg3, arg4, arg5);
		}
	}

protected:
	typedef Observer<void(Arg1, Arg2, Arg3, Arg4, Arg5)> TObserver;
	std::vector<TObserver> m_observers;
};
}

//////////////////////////////////////////////////////////////////////////

template<typename Signature>
class CCrySignal : public CrySignalPrivate::CrySignalBase<Signature>
{
public:

	CCrySignal();
	~CCrySignal();

	template<typename Object, typename MemberFunction>
	void Connect(Object* const pObject, MemberFunction function, const uintptr_t forceId = 0);

	template<typename Function>
	void Connect(Function& function, const uintptr_t id = 0);

	void DisconnectObject(void* const pObject);
	void DisconnectById(const uintptr_t id);
	void DisconnectAll();
};

//////////////////////////////////////////////////////////////////////////

template<typename Signature>
CCrySignal<Signature>::CCrySignal()
{

}

template<typename Signature>
CCrySignal<Signature>::~CCrySignal()
{
	DisconnectAll();
}

template<typename Signature>
template<typename Object, typename MemberFunction>
void CCrySignal<Signature >::Connect(Object* const pObject, MemberFunction function, const uintptr_t forceId /*= 0*/)
{
	TObserver observer;
	observer.m_id = forceId == 0 ? reinterpret_cast<uintptr_t>(pObject) : forceId;
	observer.SetMemberFunction(pObject, function);
	m_observers.push_back(observer);
}

template<typename Signature>
template<typename Function>
void CCrySignal<Signature >::Connect(Function& function, const uintptr_t id /*= 0*/)
{
	TObserver observer;
	observer.m_id = id;
	observer.SetFunction(function);
	m_observers.push_back(observer);
}

template<typename Signature>
void CCrySignal<Signature >::DisconnectObject(void* const pObject)
{
	DisconnectById(reinterpret_cast<uintptr_t>(pObject));
}

template<typename Signature>
void CCrySignal<Signature >::DisconnectById(const uintptr_t id)
{
	for (auto it = m_observers.begin(); it != m_observers.end(); )
	{
		if (it->m_id == id)
			it = m_observers.erase(it);
		else
			++it;
	}
}

template<typename Signature>
void CCrySignal<Signature >::DisconnectAll()
{
	m_observers.clear();
}
