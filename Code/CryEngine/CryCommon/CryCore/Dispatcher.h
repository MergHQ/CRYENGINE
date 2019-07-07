// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "StlUtils.h"

template<class Object, class... Args>
struct Dispatcher: SmallDynArray<Object*, uint, NAlloc::ModuleAlloc>
{
	typedef void (Object::*Function)(Args...);

	Dispatcher(Function func)
		: m_function(func) {}

	void add(Object* obj)
	{
		stl::push_back_unique(*this, obj);
		this->shrink_to_fit();
	}
	void operator()(Args... args) const
	{
		for (auto obj : *this)
			(obj->*m_function)(args...);
	}

private:
	Function m_function;
};

template<class Object, class Call>
struct CallDispatcher: SmallDynArray<Object*, uint, NAlloc::ModuleAlloc>
{
	void add(Object* obj)
	{
		stl::push_back_unique(*this, obj);
		this->shrink_to_fit();
	}
	template<class... Args>
	void operator()(Args&&... args) const
	{
		for (auto obj : *this)
			Call::call(obj, std::forward<Args>(args)...);
	}
};
