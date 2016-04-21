// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   OptArgs.h
//  Version:     v1.00
//  Created:     2014-04 by J Scott Peter
//  Compilers:   Visual Studio 2010
//  Description: Facilities for defining and combining general-purpose or specific options, for functions or structs.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#pragma once

//! Facilities for collecting options in a struct, and quickly constructing them.
//! Used, for example, as construction argument. Safer and more informative than bool arguments.
//! Example:
//!     struct FObjectOpts;
//!     struct CObject { CObject(FObjectOpts = ZERO); };
//!     CObject object_def();
//!     CObject object( FObjectOpts().Size(8).AllowGrowth(true) );
template<class TOpt, class T, class TContainer, int NInit = 0>
struct TOptVar
{
	typedef T TValue;

	TOptVar()
		: _val(T(NInit))
	{
		offset(this);
	}
	TOptVar(T val)
		: _val(val) {}

	operator T() const
	{ return _val; }
	T    operator()() const
	{ return _val; }
	T    operator+() const
	{ return _val; }
	bool operator!() const
	{ return !_val; }

	TContainer& operator()(T val)
	{
		_val = val;
		return *(TContainer*)((char*)this - offset());
	}

private:
	T             _val;

	static size_t offset(void* self = 0)
	{
		static size_t _offset = TContainer::static_offset(self);
		return _offset;
	}
};

// *INDENT-OFF* - Formatter not work properly on these macro's

#define OPT_STRUCT(TOpts)                                                                                  \
  typedef TOpts TThis; typedef uint TInt;                                                                  \
  static size_t static_offset(void* var) { static void* _first = var; return (char*)var - (char*)_first; } \
  TOpts(void* p = 0) {}                                                                                    \
  TOpts operator()() const { return TOpts(*this); }                                                        \

#define OPT_VAR(Type, Var)                              \
  enum E ## Var {}; TOptVar<E ## Var, Type, TThis> Var; \

#define OPT_VAR_INIT(Type, Var, init)                         \
  enum E ## Var {}; TOptVar<E ## Var, Type, TThis, init> Var; \

#define BIT_STRUCT(Struc, Int)                                                                \
  typedef Struc TThis; typedef Int TInt;                                                      \
  TInt Mask() const { return *(const TInt*)this; }                                            \
  TInt& Mask() { return *(TInt*)this; }                                                       \
  Struc(TInt init = 0) { COMPILE_TIME_ASSERT(sizeof(TThis) == sizeof(TInt)); Mask() = init; } \

#define BIT_VAR(Var)                                     \
  TInt _ ## Var : 1;                                     \
  bool Var() const { return _ ## Var; }                  \
  TThis& Var(bool val) { _ ## Var = val; return *this; } \

// *INDENT-ON*
