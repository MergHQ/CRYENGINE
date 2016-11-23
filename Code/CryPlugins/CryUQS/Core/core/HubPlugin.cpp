// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

// - needs to be included once per DLL module
// - otherwise, you'd get linker errors regarding CryAssert, memory manager, etc.
// - not required when building a .lib
#include <CryCore/Platform/platform_impl.inl>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// CHubPlugin
		//
		//===================================================================================

		class CHubPlugin : public IHubPlugin, public ISystemEventListener
		{
			CRYINTERFACE_BEGIN()
			CRYINTERFACE_ADD(IHubPlugin)
			CRYINTERFACE_ADD(ICryPlugin)
			CRYINTERFACE_END()

			CRYGENERATE_SINGLETONCLASS(CHubPlugin, "Plugin_UQS", 0x2a2f00e0f0684baf, 0xb31bb3c8f78b3477)

			CHubPlugin();
			virtual ~CHubPlugin();

		private:
			// ICryPlugin (forwarded by IHubPlugin)
			virtual const char*     GetName() const override;
			virtual const char*     GetCategory() const override;
			virtual bool            Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override;
			virtual void            OnPluginUpdate(EPluginUpdateType updateType) override;
			// ~ICryPlugin

			// IHubPlugin
			virtual IHub&           GetHubImplementation() override;
			// ~IHubPlugin

			// ISystemEventListener
			virtual void            OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
			// ~ISystemEventListener

		private:
			std::unique_ptr<CHub>   m_pHub;
		};

		CRYREGISTER_SINGLETON_CLASS(CHubPlugin)

		// the ctor and dtor were declared by some code-weaver macros (specifically by _ENFORCE_CRYFACTORY_USAGE() which is used by CRYGENERATE_SINGLETONCLASS())

		CHubPlugin::CHubPlugin()
		{
			m_updateFlags = 0;  // the ctor of ICryPlugin base class should have done that, but didn't
			GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);
		}

		CHubPlugin::~CHubPlugin()
		{
			GetISystem()->GetISystemEventDispatcher()->RemoveListener(this);
		}

		const char* CHubPlugin::GetName() const
		{
			return "UQS";
		}

		const char* CHubPlugin::GetCategory() const
		{
			return "Plugin";
		}

		bool CHubPlugin::Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
		{
			// Notice: we currently do *not* yet instantiate the UQS Hub here, because some of its sub-systems rely on the presence of sub-systems of ISystem, like IInput, which
			// are still NULL at this point in time. So instead, we instantiate the UQS Hub upon ESYSTEM_EVENT_CRYSYSTEM_INIT_DONE (see OnSystemEvent()).
			return true;
		}

		void CHubPlugin::OnPluginUpdate(EPluginUpdateType updateType)
		{
			// nothing (the game or editor shall call IHub::Update() directly whenever it wants queries to get processed)
		}

		IHub& CHubPlugin::GetHubImplementation()
		{
			// if this assert fails, then some client code tried to access the one and only UQS Hub instance before ESYSTEM_EVENT_CRYSYSTEM_INIT_DONE (where it gets instantiated).
			assert(m_pHub.get());
			return *m_pHub.get();
		}

		void CHubPlugin::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
		{
			switch (event)
			{
			case ESYSTEM_EVENT_CRYSYSTEM_INIT_DONE:
				assert(!m_pHub.get());
				m_pHub.reset(new CHub);
				break;
			}
		}

	}
}
