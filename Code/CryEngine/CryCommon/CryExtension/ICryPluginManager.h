// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ICryPlugin.h>
#include <CryCore/Containers/CryListenerSet.h>

struct IPluginEventListener
{
	enum class EPluginEvent
	{
		EPlugin_Initialized,
		EPlugin_Unloaded
	};

	virtual ~IPluginEventListener() {}
	virtual void OnPluginEvent(const char* szPluginName, EPluginEvent event) = 0;
};

struct ICryPluginManager
{
	enum class EPluginType
	{
		EPluginType_CPP = 0,
		EPluginType_CS
	};

	ICryPluginManager() {}
	virtual ~ICryPluginManager() {}

	virtual void RegisterListener(const char* szPluginName, IPluginEventListener* pListener) = 0;
	virtual void RemoveListener(IPluginEventListener* pListener) = 0;

	virtual void Update(int updateFlags, int nPauseMode) = 0;

	template<typename T>
	T* QueryPlugin() const
	{
		ICryUnknownPtr pExtension = QueryPluginById(cryiidof<T>());

		if (pExtension)
		{
			return cryinterface_cast<T>(pExtension.get());
		}

		return nullptr;
	}

protected:
	virtual ICryPluginPtr QueryPluginById(const CryClassID& classID) const = 0;
};
