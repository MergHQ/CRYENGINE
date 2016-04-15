// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace metautils
{
template<bool Condition, typename Ty1, typename Ty2>
struct select;

template<typename Ty1, typename Ty2>
struct select<true, Ty1, Ty2>
{
	typedef Ty1 type;
};

template<typename Ty1, typename Ty2>
struct select<false, Ty1, Ty2>
{
	typedef Ty2 type;
};

//! Identifies whether types Ty1 and Ty2 are the same including const & volatile.
template<typename Ty1, typename Ty2>
struct is_same;

template<typename Ty1>
struct is_same<Ty1, Ty1>
{
	enum { value = true, };
};

template<typename Ty1, typename Ty2>
struct is_same
{
	enum { value = false, };
};

//! Removes top level const qualifier.
template<class Ty>
struct remove_const
{
	typedef Ty type;
};

template<class Ty>
struct remove_const<const Ty>
{
	typedef Ty type;
};

template<class Ty>
struct remove_const<const Ty[]>
{
	typedef Ty type[];
};

template<class Ty, unsigned int N>
struct remove_const<const Ty[N]>
{
	typedef Ty type[N];
};

//! Determines whether type Ty is const qualified.
template<typename Ty>
struct is_const
{
	enum { value = false };
};

template<typename Ty>
struct is_const<const Ty>
{
	enum { value = true };
};

template<class Ty, unsigned int N>
struct is_const<Ty[N]>
{
	enum { value = false };
};

template<class Ty, unsigned int N>
struct is_const<const Ty[N]>
{
	enum { value = true };
};

template<class Ty>
struct is_const<Ty&>
{
	enum { value = false };
};

}
