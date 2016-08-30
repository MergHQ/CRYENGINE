// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryExtension/ICryPluginManager.h>

struct SPluginContainer;
struct SPluginDescriptor;
typedef std::vector<SPluginContainer>       TPluginList;
typedef CListenerSet<IPluginEventListener*> TPluginEventListeners;

class CCryPluginManager final : public ICryPluginManager, public ISystemEventListener
{
public:
	CCryPluginManager(const SSystemInitParams& initParams);
	virtual ~CCryPluginManager();

	virtual void RegisterListener(const char* szPluginName, IPluginEventListener* pListener) override
	{
		// we have to simply add this listener now because the plugin can be loaded at any time
		// this should change in release builds since all the necessary plugins will be loaded upfront
		m_pluginListeners.Add(pListener, szPluginName);
	}

	virtual void RemoveListener(IPluginEventListener* pListener) override
	{
		m_pluginListeners.Remove(pListener);
	}

	virtual void Update(int updateFlags, int nPauseMode) override;
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

protected:
	virtual ICryPluginPtr QueryPluginById(const CryClassID& classID) const override;

private:
	bool                    Initialize();
	bool                    LoadExtensionFile(const char* filename);
	bool                    LoadPlugin(const SPluginDescriptor& pluginDescriptor);
	bool                    UnloadPlugin(const SPluginDescriptor& pluginDescriptor, bool bReloadOther = false);
	bool                    UnloadAllPlugins();
	void                    NotifyListeners(const char* szPluginName, IPluginEventListener::EPluginEvent event);

	static void             ReloadPluginCmd(IConsoleCmdArgs* pArgs);

	const SPluginContainer* FindPluginContainer(const string& pluginName) const;

	TPluginList               m_pluginContainer;
	TPluginEventListeners     m_pluginListeners;

	const SSystemInitParams   m_systemInitParams;
	static CCryPluginManager* m_pThis;
};
