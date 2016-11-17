// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryExtension/ClassWeaver.h>
#include <Schematyc/ICore.h>

namespace Schematyc
{
// Forward declare interfaces.
struct ILogOutput;
// Forward declare classes.
class CCompiler;
class CEnvRegistry;
class CLog;
class CLogRecorder;
class CObjectPool;
class CRuntimeRegistry;
class CScriptRegistry;
class CSettingsManager;
class CTimerSystem;
class CUpdateScheduler;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(ILogOutput)

class CCore : public ICore
{
	CRYINTERFACE_SIMPLE(ICore)

	CRYGENERATE_CLASS(CCore, ms_szClassName, 0x96d98d9835aa4fb6, 0x830b53dbfe71908d)

public:

	void Init();

	// ICore

	virtual void                     SetGUIDGenerator(const GUIDGenerator& guidGenerator) override;
	virtual SGUID                    CreateGUID() const override;

	virtual const char*              GetFileFormat() const override;
	virtual const char*              GetRootFolder() const override;
	virtual const char*              GetScriptsFolder() const override;
	virtual const char*              GetSettingsFolder() const override;
	virtual bool                     IsExperimentalFeatureEnabled(const char* szFeatureName) const override;

	virtual IEnvRegistry&            GetEnvRegistry() override;
	virtual IScriptRegistry&         GetScriptRegistry() override;
	virtual IRuntimeRegistry&        GetRuntimeRegistry() override;
	virtual ICompiler&               GetCompiler() override;
	virtual ILog&                    GetLog() override;
	virtual ILogRecorder&            GetLogRecorder() override;
	virtual ISettingsManager&        GetSettingsManager() override;
	virtual IUpdateScheduler&        GetUpdateScheduler() override;
	virtual ITimerSystem&            GetTimerSystem() override;

	virtual IValidatorArchivePtr     CreateValidatorArchive(const SValidatorArchiveParams& params) const override;
	virtual ISerializationContextPtr CreateSerializationContext(const SSerializationContextParams& params) const override;
	virtual IScriptViewPtr           CreateScriptView(const SGUID& scopeGUID) const override;

	virtual IObject*                 CreateObject(const SObjectParams& params) override;
	virtual IObject*                 GetObject(ObjectId objectId) override;
	virtual void                     DestroyObject(ObjectId objectId) override;
	virtual void                     SendSignal(ObjectId objectId, const SGUID& signalGUID, CRuntimeParams& params) override;
	virtual void                     BroadcastSignal(const SGUID& signalGUID, CRuntimeParams& params) override;

	virtual void                     PrePhysicsUpdate() override;
	virtual void                     Update() override;

	virtual void                     RefreshLogFileSettings() override;
	virtual void                     RefreshEnv() override;

	// ~ICore

	void       RefreshLogFileStreams();
	void       RefreshLogFileMessageTypes();

	CRuntimeRegistry& GetRuntimeRegistryImpl();
	CCompiler& GetCompilerImpl();

public:

	static const char* ms_szClassName;

private:

	GUIDGenerator                     m_guidGenerator;
	mutable string                    m_scriptsFolder;    // #SchematycTODO : How can we avoid making this mutable?
	mutable string                    m_settingsFolder;   // #SchematycTODO : How can we avoid making this mutable?
	std::unique_ptr<CEnvRegistry>     m_pEnvRegistry;
	std::unique_ptr<CScriptRegistry>  m_pScriptRegistry;
	std::unique_ptr<CRuntimeRegistry> m_pRuntimeRegistry;
	std::unique_ptr<CCompiler>        m_pCompiler;
	std::unique_ptr<CObjectPool>      m_pObjectPool;
	std::unique_ptr<CTimerSystem>     m_pTimerSystem;
	std::unique_ptr<CLog>             m_pLog;
	ILogOutputPtr                     m_pLogFileOutput;
	std::unique_ptr<CLogRecorder>     m_pLogRecorder;
	std::unique_ptr<CSettingsManager> m_pSettingsManager;
	std::unique_ptr<CUpdateScheduler> m_pUpdateScheduler;
};
} // Schematyc

//////////////////////////////////////////////////////////////////////////
inline Schematyc::CCore* GetSchematycCoreImplPtr()
{
	return static_cast<Schematyc::CCore*>(GetSchematycCorePtr());
}

// Get Schematyc core.
//////////////////////////////////////////////////////////////////////////
inline Schematyc::CCore& GetSchematycCoreImpl()
{
	return static_cast<Schematyc::CCore&>(GetSchematycCore());
}
