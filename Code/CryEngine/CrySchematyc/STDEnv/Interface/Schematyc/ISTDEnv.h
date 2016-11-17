// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryExtension/ClassWeaver.h>
#include <CryGame/IGameFramework.h>

namespace Schematyc
{
// Forward declare interfaces.
struct ISpatialIndex;

struct ISTDEnv : public ICryUnknown
{
	CRYINTERFACE_DECLARE(ISTDEnv, 0x2846c43757b24803, 0xa281b1938659395e)
};

// Create standard environment.
//////////////////////////////////////////////////////////////////////////
ISTDEnv* CreateSTDEnv(IGameFramework* pGameFramework);
} // Schematyc

// Get Schematyc standard environment pointer.
//////////////////////////////////////////////////////////////////////////
inline Schematyc::ISTDEnv* GetSchematycSTDEnvPtr()
{
	static Schematyc::ISTDEnv* pSTDEnv = nullptr;
	if (!pSTDEnv)
	{
		pSTDEnv = gEnv->pGameFramework->QueryExtension<Schematyc::ISTDEnv>();
	}
	return pSTDEnv;
}

// Get Schematyc standard environment.
//////////////////////////////////////////////////////////////////////////
inline Schematyc::ISTDEnv& GetSchematycSTDEnv()
{
	Schematyc::ISTDEnv* pSTDEnv = GetSchematycSTDEnvPtr();
	if (!pSTDEnv)
	{
		CryFatalError("Schematyc standard environment uninitialized!");
	}
	return *pSTDEnv;
}
