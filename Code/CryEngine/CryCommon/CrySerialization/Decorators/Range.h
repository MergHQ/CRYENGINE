// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace Serialization
{

template<typename Type>
struct DefaultSinglestep
{
	static double value()
	{
		return std::is_integral<Type>::value ? 1.0 : 0.1;
	}
};

template<class T>
struct RangeDecorator
{
	T* value;
	T hardMin;
	T hardMax;
	T singleStep;

	void YASLI_SERIALIZE_METHOD(class yasli::Archive& ar) {}
};

template<class T>
RangeDecorator<T> Range(T& value, T hardMin, T hardMax, T singleStep = (T)DefaultSinglestep<T>::value())
{
	RangeDecorator<T> r;
	r.value = &value;
	r.hardMin = hardMin;
	r.hardMax = hardMax;
	r.singleStep = singleStep;
	return r;
}

namespace Decorators
{
//! Obsolete name, will be removed. Please use Serialization::Range instead.
using Serialization::Range;
}

}

#include "RangeImpl.h"
