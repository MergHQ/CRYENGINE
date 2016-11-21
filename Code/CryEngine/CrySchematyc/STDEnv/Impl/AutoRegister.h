// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryCore/StaticInstanceList.h>

#define SCHEMATYC_AUTO_REGISTER(callbackPtr) static Schematyc::CAutoRegistrar SCHEMATYC_PP_JOIN_XY(g_schematycAutoRegistrar, __COUNTER__)(callbackPtr);

namespace Schematyc
{
// Forward declare interfaces.
struct IEnvRegistrar;

class CAutoRegistrar : public CStaticInstanceList<CAutoRegistrar>
{
public:

	typedef void(*CallbackPtr)(IEnvRegistrar& registrar);

public:

	CAutoRegistrar(CallbackPtr pCallback);

	static void Process(IEnvRegistrar& registrar);

private:

	CallbackPtr m_pCallback;
};
} // Schematyc
