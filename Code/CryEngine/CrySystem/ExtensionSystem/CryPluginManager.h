// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryExtension/ICryPluginManager.h>

struct SPluginContainer;

class CCryPluginManager final : public ICryPluginManager, public ISystemEventListener
{
public:
	CCryPluginManager(const SSystemInitParams& initParams);
	virtual ~CCryPluginManager();

	virtual void RegisterEventListener(const CryClassID& pluginClassId, IPluginEventListener* pListener) override
	{
		// we have to simply add this listener now because the plugin can be loaded at any time
		// this should change in release builds since all the necessary plugins will be loaded upfront
		m_pluginListenerMap[pListener].push_back(pluginClassId);
	}

	virtual void RemoveEventListener(const CryClassID& pluginClassId, IPluginEventListener* pListener) override
	{
		auto it = m_pluginListenerMap.find(pListener);
		if (it != m_pluginListenerMap.end())
		{
			stl::find_and_erase(it->second, pluginClassId);
		}
	}

	// Called by CrySystem during early init to initialize the manager and load plugins
	// Plugins that require later activation can do so by listening to system events such as ESYSTEM_EVENT_PRE_RENDERER_INIT
	bool Initialize();

	void Update(IPluginUpdateListener::EPluginUpdateType updateFlags);

	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

protected:
	virtual bool                        LoadPluginFromDisk(EPluginType type, const char* path, const char* className) override;

	virtual std::shared_ptr<ICryPlugin> QueryPluginById(const CryClassID& classID) const override;
	virtual std::shared_ptr<ICryPlugin> AcquirePluginById(const CryClassID& classID) override;

private:
	bool                    LoadExtensionFile(const char* filename);
	bool                    UnloadAllPlugins();
	void                    NotifyEventListeners(const CryClassID& classID, IPluginEventListener::EPluginEvent event);

	static void             ReloadPluginCmd(IConsoleCmdArgs* pArgs);
	
	std::vector<SPluginContainer> m_pluginContainer;
	std::map<IPluginEventListener*, std::vector<CryClassID>> m_pluginListenerMap;

	const SSystemInitParams   m_systemInitParams;
	static CCryPluginManager* s_pThis;
};
