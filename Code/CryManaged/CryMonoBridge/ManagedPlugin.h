// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ICryPlugin.h>

struct IMonoClass;
struct IMonoObject;
class CMonoLibrary;

// Main entry-point handler for managed code, indirectly created by the plugin manager
class CManagedPlugin 
	: public ICryPlugin
	, public ISystemEventListener
{
public:
	CManagedPlugin(const char* szBinaryPath);
	virtual ~CManagedPlugin();

	// ICryUnknown
	virtual ICryFactory* GetFactory() const override { return nullptr; }

	virtual void* QueryInterface(const CryInterfaceID& iid) const override { return nullptr; }
	virtual void* QueryComposite(const char* name) const override { return nullptr; }
	// ~ICryUnknown

	// ICryPlugin
	virtual const char* GetName() const override { return m_pluginName; }

	virtual const char* GetCategory() const override { return "Managed"; }

	virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override { return true; }

	virtual void OnPluginUpdate(EPluginUpdateType updateType) override {}
	// ~ICryPlugin

	// ISystemEventListener
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
	// ~ISystemEventListener

protected:
	const char* TryGetPlugin() const;

	bool InitializePlugin();

protected:
	CMonoLibrary* m_pLibrary;

	IMonoClass*   m_pClass;
	std::shared_ptr<IMonoObject>   m_pMonoObject;

	string        m_pluginName;
	string        m_libraryPath;
};