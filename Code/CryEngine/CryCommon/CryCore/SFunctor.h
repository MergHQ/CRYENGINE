// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   IFunctor.h
//  Version:     v1.00
//  Created:     05/30/2013 by Paulo Zaffari.
//  Description: User header for multi DLL functors.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef  _IFUNCTOR_H_
#define  _IFUNCTOR_H_

#pragma once

#include "FunctorBaseFunction.h"
#include "FunctorBaseMember.h"

// Needed for CryFont to compile.
// Maybe for others too.
#include "smartptr.h"

struct SFunctor
{
public:
	//! Calls the functor method.
	//! \return true if a functor is registered, false otherwise.
	bool Call()
	{
		if (!m_pFunctor)
		{
			return false;
		}

		m_pFunctor->Call();

		return true;
	}

	//! Sets a new functor, common function, void return type, no arguments.
	//! \param pCallback The function pointer.
	template<typename tCallback>
	void Set(tCallback pCallback)
	{
		typedef TFunctor<tCallback> TType;
		m_pFunctor = new TType(pCallback);
	}

	//! Sets a new functor, common function, void return type, 1 argument.
	//! \param pCallback The function pointer.
	//! \param Argument1 The argument to be passed to the function.
	template<typename tCallback, typename tArgument1>
	void Set(tCallback pCallback, const tArgument1& Argument1)
	{
		typedef TFunctor<tCallback> TType;
		m_pFunctor = new TType(pCallback, Argument1);
	}

	//! Sets a new functor, common function, void return type, 2 arguments.
	//! \param tCallback The function pointer.
	//! \param tArgument1 The first argument to be passed to the function.
	//! \param tArgument2 The second argument to be passed to the function.
	template<typename tCallback, typename tArgument1, typename tArgument2>
	void Set(tCallback pCallback, const tArgument1& Argument1, const tArgument2& Argument2)
	{
		typedef TFunctor<tCallback> TType;
		m_pFunctor = new TType(pCallback, Argument1, Argument2);
	}

	//! Sets a new functor, common function, void return type, no arguments.
	//! \param pCallback The function pointer.
	template<typename tCallee>
	void Set(tCallee* pCallee, void (tCallee::* pCallback)())
	{
		typedef TFunctor<void (tCallee::*)()> TType;
		m_pFunctor = new TType(pCallee, pCallback);
	}

	//! Sets a new functor, common function, void return type, 1 argument.
	//! Parameters: the function pointer, the 2 arguments to be passed to the function, the argument.
	template<typename tCallee, typename tArgument1>
	void Set(tCallee* pCallee, void (tCallee::* pCallback)(tArgument1), const tArgument1& Argument1)
	{
		typedef TFunctor<void (tCallee::*)(tArgument1)> TType;
		m_pFunctor = new TType(pCallee, pCallback, Argument1);
	}

	//! Sets a new functor, common function, void return type, 2 arguments.
	//! Parameters: the function pointer, the 2 arguments to be passed to the function, the 2 arguments.
	template<typename tCallee, typename tArgument1, typename tArgument2>
	void Set(tCallee* pCallee, void (tCallee::* pCallback)(tArgument1, tArgument2), const tArgument1& Argument1, const tArgument2& Argument2)
	{
		typedef TFunctor<void (tCallee::*)(tArgument1, tArgument2)> TType;
		m_pFunctor = new TType(pCallee, pCallback, Argument1, Argument2);
	}

	//! Used to compare equality of two IFunctors.
	bool operator==(const SFunctor& rOther) const
	{
		return m_pFunctor == rOther.m_pFunctor;
	}

	//! Used to order IFunctors (needed for some containers, like map).
	bool operator<(const SFunctor& rOther) const
	{
		return m_pFunctor < rOther.m_pFunctor;
	}
protected:
	_smart_ptr<IFunctorBase> m_pFunctor;
};

#endif //_IFUNCTOR_H_
