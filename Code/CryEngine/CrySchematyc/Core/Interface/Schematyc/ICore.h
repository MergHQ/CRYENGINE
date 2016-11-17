// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryExtension/ClassWeaver.h>
#include <CryGame/IGameFramework.h>
#include <CrySerialization/Forward.h>

#include "Schematyc/FundamentalTypes.h"
#include "Schematyc/Utils/Delegate.h"

namespace Schematyc
{
// Forward declare interfaces.
struct ICompiler;
struct IScriptView;
struct IEnvRegistrar;
struct IEnvRegistry;
struct ILog;
struct ILogRecorder;
struct IObject;
struct IRuntimeRegistry;
struct IScriptRegistry;
struct ISerializationContext;
struct ISettingsManager;
struct ITimerSystem;
struct IUpdateScheduler;
struct IValidatorArchive;
// Forward declare structures.
struct SGUID;
struct SObjectParams;
struct SSerializationContextParams;
struct SValidatorArchiveParams;
// Forward declare classes.
class CRuntimeParams;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(IScriptView)
DECLARE_SHARED_POINTERS(ISerializationContext)
DECLARE_SHARED_POINTERS(IValidatorArchive)

typedef CDelegate<SGUID ()> GUIDGenerator;

struct ICore : public ICryUnknown
{
	CRYINTERFACE_DECLARE(ICore, 0x041b8bda35d74341, 0xbde7f0ca69be2595)

	virtual void                     SetGUIDGenerator(const GUIDGenerator& guidGenerator) = 0;
	virtual SGUID                    CreateGUID() const = 0;

	virtual const char*              GetFileFormat() const = 0;
	virtual const char*              GetRootFolder() const = 0;
	virtual const char*              GetScriptsFolder() const = 0;     // #SchematycTODO : Do we really need access to this outside script registry?
	virtual const char*              GetSettingsFolder() const = 0;    // #SchematycTODO : Do we really need access to this outside env registry?
	virtual bool                     IsExperimentalFeatureEnabled(const char* szFeatureName) const = 0;

	virtual IEnvRegistry&            GetEnvRegistry() = 0;
	virtual IScriptRegistry&         GetScriptRegistry() = 0;
	virtual IRuntimeRegistry&        GetRuntimeRegistry() = 0;
	virtual ICompiler&               GetCompiler() = 0;
	virtual ILog&                    GetLog() = 0;
	virtual ILogRecorder&            GetLogRecorder() = 0;
	virtual ISettingsManager&        GetSettingsManager() = 0;
	virtual IUpdateScheduler&        GetUpdateScheduler() = 0;
	virtual ITimerSystem&            GetTimerSystem() = 0;

	virtual IValidatorArchivePtr     CreateValidatorArchive(const SValidatorArchiveParams& params) const = 0;
	virtual ISerializationContextPtr CreateSerializationContext(const SSerializationContextParams& params) const = 0;
	virtual IScriptViewPtr           CreateScriptView(const SGUID& scopeGUID) const = 0;

	virtual IObject*                 CreateObject(const SObjectParams& params) = 0;
	virtual IObject*                 GetObject(ObjectId objectId) = 0;
	virtual void                     DestroyObject(ObjectId objectId) = 0;
	virtual void                     SendSignal(ObjectId objectId, const SGUID& signalGUID, CRuntimeParams& params) = 0;
	virtual void                     BroadcastSignal(const SGUID& signalGUID, CRuntimeParams& params) = 0;

	virtual void                     PrePhysicsUpdate() = 0;
	virtual void                     Update() = 0;

	virtual void                     RefreshLogFileSettings() = 0;
	virtual void                     RefreshEnv() = 0;
};

// Create core.
//////////////////////////////////////////////////////////////////////////
ICore* CreateCore(IGameFramework* pGameFramework);
} // Schematyc

// Get Schematyc core pointer.
//////////////////////////////////////////////////////////////////////////
inline Schematyc::ICore* GetSchematycCorePtr()
{
	static Schematyc::ICore* pCore = nullptr;
	if (!pCore)
	{
		pCore = gEnv->pGameFramework->QueryExtension<Schematyc::ICore>();
	}
	return pCore;
}

// Get Schematyc core.
//////////////////////////////////////////////////////////////////////////
inline Schematyc::ICore& GetSchematycCore()
{
	Schematyc::ICore* pCore = GetSchematycCorePtr();
	if (!pCore)
	{
		CryFatalError("Schematyc core uninitialized!");
	}
	return *pCore;
}
