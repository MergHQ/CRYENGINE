// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace Serialization
{

template<class T>
struct RangeDecorator
{
	T* value;
	T  softMin;
	T  softMax;
	T  hardMin;
	T  hardMax;
};

template<class T>
RangeDecorator<T> Range(T& value, T hardMin, T hardMax)
{
	RangeDecorator<T> r;
	r.value = &value;
	r.softMin = hardMin;
	r.softMax = hardMax;
	r.hardMin = hardMin;
	r.hardMax = hardMax;
	return r;
}

template<class T>
RangeDecorator<T> Range(T& value, T softMin, T softMax, T hardMin, T hardMax)
{
	RangeDecorator<T> r;
	r.value = &value;
	r.softMin = softMin;
	r.softMax = softMax;
	r.hardMin = hardMin;
	r.hardMax = hardMax;
	return r;
}

namespace Decorators
{
//! Obsolete name, will be removed. Please use Serialization::Range instead.
using Serialization::Range;
}

}

#include "RangeImpl.h"
