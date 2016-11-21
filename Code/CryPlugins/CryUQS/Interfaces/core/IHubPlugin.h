// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ICryPlugin.h>
#include <CryExtension/ICryPluginManager.h>

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// IHubPlugin
		//
		//===================================================================================

		class IHubPlugin : public ICryPlugin
		{
			CRYINTERFACE_DECLARE(IHubPlugin, 0xde73f1db0c37498f, 0x8396d76e43988c60)

			// virtual ~IHubPlugin() {} is already provided by the CRYINTERFACE_DECLARE class-weaver macro

		private:

			//===================================================================================
			//
			// CCachedHubPtr
			//
			// - caches a pointer to the one and only global UQS Hub
			// - resets that pointer when the UQS plugin gets unloaded and retrieves the pointer again on first access
			//
			//===================================================================================

			class CCachedHubPtr : public IPluginEventListener
			{
			public:
				explicit          CCachedHubPtr();
								  ~CCachedHubPtr();
				IHub*             GetHubPtr();

			private:
				// IPluginEventListener
				virtual void      OnPluginEvent(const CryClassID& pluginClassId, EPluginEvent event) override;
				// ~IPluginEventListener

			private:
				IHub*             m_pHub;
			};

		public:
			// these are meant to be used by game code and other client code to get access to the potentially instantiated global UQS Hub instance
			static IHub*          GetHubPtr();
			static IHub&          GetHub();

		private:

			// ICryPlugin: forward these to the derived class
			virtual const char*   GetName() const override = 0;      // this method might eventually get removed, since the "name" of the plugin will be read from the cryplugin.csv already
			virtual const char*   GetCategory() const override = 0;  // (this as well, although the cryplugin.csv doesn't provide a "category")
			virtual bool          Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override = 0;
			virtual void          OnPluginUpdate(EPluginUpdateType updateType) override = 0;
			// ~ICryPlugin

			virtual IHub&         GetHubImplementation() = 0;
		};

		// This can be called after the plugin system has loaded and instantiated the UQS to obtain a pointer to the one and only IHub instance.
		// Returns nullptr if the UQS plugin is not loaded or has already been unloaded.
		// Checking for nullptr can be used to see if the UQS is actually installed and available for use.
		inline IHub* IHubPlugin::GetHubPtr()
		{
			static CCachedHubPtr cachedHubPtr;
			return cachedHubPtr.GetHubPtr();  // might still be a nullptr
		}

		// Same functionality as GetHubPtr() except that it triggers a fatal error if no IHub has been installed yet.
		inline IHub& IHubPlugin::GetHub()
		{
			IHub* pHub = GetHubPtr();
			if (!pHub)
			{
				CryFatalError("UQS hub not yet installed! (or got un-installed already)");
			}
			return *pHub;
		}

		inline IHubPlugin::CCachedHubPtr::CCachedHubPtr()
			: m_pHub(nullptr)
		{
			gEnv->pSystem->GetIPluginManager()->RegisterEventListener(cryiidof<IHubPlugin>(), this);
		}

		inline IHubPlugin::CCachedHubPtr::~CCachedHubPtr()
		{
			if (gEnv && gEnv->pSystem && gEnv->pSystem->GetIPluginManager())
			{
				gEnv->pSystem->GetIPluginManager()->RemoveEventListener(cryiidof<IHubPlugin>(), this);
			}
		}

		inline IHub* IHubPlugin::CCachedHubPtr::GetHubPtr()
		{
			if (!m_pHub)
			{
				if (IHubPlugin* pHubPlugin = gEnv->pSystem->GetIPluginManager()->QueryPlugin<IHubPlugin>())
				{
					m_pHub = &pHubPlugin->GetHubImplementation();
				}
			}
			return m_pHub;  // might still be a nullptr
		}

		inline void IHubPlugin::CCachedHubPtr::OnPluginEvent(const CryClassID& pluginClassId, IPluginEventListener::EPluginEvent event)
		{
			if (pluginClassId == cryiidof<IHubPlugin>())
			{
				// reset the cached Hub pointer when the plugin gets unloaded
				if (event == IPluginEventListener::EPluginEvent::Unloaded)
				{
					m_pHub = nullptr;
				}
			}
		}

	}
}
