// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryCore/SmallFunction.h>

namespace Cry
{

//! Alias for a unique pointer with a polymorphic deleter, using our SmallFunc as a function container.
//! Main application: returning resources from factory functions across module boundaries,
//! where the ownership is given to the user but the deletion still has to happen in the original module
template <typename T>
using xmodule_ptr = std::unique_ptr<T, SmallFunction<void(T*)> >;

//! Similar factory function to make_unique for xmodule_ptr
template<typename T, typename TFunc, typename... TArgs>
inline xmodule_ptr<T> make_xmodule(TFunc&& deleter, TArgs&&... args)
{
	return ( xmodule_ptr<T>( new T(std::forward<TArgs>(args)...), std::forward<TFunc>(deleter) ) );
}

}