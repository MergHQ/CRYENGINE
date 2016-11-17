// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryExtension/ClassWeaver.h>
#include <Schematyc/ISTDEnv.h>

namespace Schematyc
{
// Froward declare interfaces.
struct IEnvRegistrar;
// Forward declare classes.
class CEntityObjectClassRegistry;
class CEntityObjectDebugger;
class CEntityObjectMap;
class CSystemStateMonitor;

class CSTDEnv : public ISTDEnv
{
	CRYINTERFACE_SIMPLE(ISTDEnv)

	CRYGENERATE_CLASS(CSTDEnv, ms_szClassName, 0x034c1d02501547ab, 0xb216c87cc5258a61)

public:

	const CSystemStateMonitor&  GetSystemStateMonitor() const;
	CEntityObjectClassRegistry& GetEntityObjectClassRegistry();
	CEntityObjectMap&           GetEntityObjectMap();
	CEntityObjectDebugger&      GetEntityObjectDebugger();

private:

	void RegisterPackage(IEnvRegistrar& registrar);

public:

	static const char* ms_szClassName;

private:

	std::unique_ptr<CSystemStateMonitor>        m_pSystemStateMonitor;
	std::unique_ptr<CEntityObjectClassRegistry> m_pEntityObjectClassRegistry;
	std::unique_ptr<CEntityObjectMap>           m_pEntityObjectMap;
	std::unique_ptr<CEntityObjectDebugger>      m_pEntityObjectDebugger;
	CConnectionScope                            m_connectionScope;
};
} // Schematyc

//////////////////////////////////////////////////////////////////////////
inline Schematyc::CSTDEnv* GetSchematycSTDEnvImplPtr()
{
	return static_cast<Schematyc::CSTDEnv*>(GetSchematycSTDEnvPtr());
}

// Get Schematyc framework.
//////////////////////////////////////////////////////////////////////////
inline Schematyc::CSTDEnv& GetSchematycSTDEnvImpl()
{
	return static_cast<Schematyc::CSTDEnv&>(GetSchematycSTDEnv());
}
