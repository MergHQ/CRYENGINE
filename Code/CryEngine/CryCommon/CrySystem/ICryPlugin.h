// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ISystem.h>
#include <CryExtension/ClassWeaver.h>
#include <CryExtension/ICryUnknown.h>
#include <CryExtension/ICryFactoryRegistryImpl.h>
#include <CryFlowGraph/IFlowBaseNode.h>

struct SSystemInitParams;
struct IPluginUpdateListener
{
	enum EPluginUpdateType
	{
		EUpdateType_NoUpdate         = BIT(0),
		EUpdateType_PrePhysicsUpdate = BIT(1),
		EUpdateType_Update           = BIT(2)
	};

	virtual ~IPluginUpdateListener() {}
	virtual void OnPluginUpdate(EPluginUpdateType updateType) = 0;
};

struct ICryPlugin : public ICryUnknown, IPluginUpdateListener
{
	CRYINTERFACE_DECLARE(ICryPlugin, 0xF491A0DB38634FCA, 0xB6E6BCFE2D98EEA2);

	virtual ~ICryPlugin()
	{
#ifndef _LIB
		if (gEnv)
		{
			if (auto pSystem = gEnv->pSystem)
			{
				ICryFactoryRegistryImpl* pCryFactoryImpl = static_cast<ICryFactoryRegistryImpl*>(pSystem->GetCryFactoryRegistry());
				pCryFactoryImpl->UnregisterFactories(g_pHeadToRegFactories);
			}
#if (defined(_LAUNCHER) && defined(CRY_IS_MONOLITHIC_BUILD)) || !defined(_LIB)
			if (auto pConsole = gEnv->pConsole)
			{
				// Unregister all commands that were registered from within the plugin
				for (auto& it : g_moduleCommands)
				{
					pConsole->RemoveCommand(it);
				}
				g_moduleCommands.clear();

				// Unregister all CVars that were registered from within the plugin
				for (auto& it : g_moduleCVars)
				{
					pConsole->UnregisterVariable(it);
				}
				g_moduleCVars.clear();
			}
#endif
		}
#endif
	}

	//! Retrieve name of the plugin.
	virtual const char* GetName() const = 0;

	//! Retrieve category of the plugin.
	virtual const char* GetCategory() const = 0;

	//! This is called to initialize the new plugin.
	virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) = 0;

	virtual bool RegisterFlowNodes() { return false; }
	virtual bool UnregisterFlowNodes() { return false; }

	uint8 GetUpdateFlags() const { return m_updateFlags; }
	
	void SetUpdateFlags(uint8 flags) { m_updateFlags = flags; }

protected:
	uint8 m_updateFlags;
	std::vector<TFlowNodeTypeId> m_registeredFlowNodeIds;
};

#ifndef _LIB
	#define USE_CRYPLUGIN_FLOWNODES
#else
	#define USE_CRYPLUGIN_FLOWNODES
#endif

#ifndef _LIB
	#define PLUGIN_FLOWNODE_REGISTER                                                                   \
	  virtual bool RegisterFlowNodes() override                                                        \
	  {                                                                                                \
	    m_registeredFlowNodeIds.clear();                                                               \
	                                                                                                   \
	    CAutoRegFlowNodeBase* pFactory = CAutoRegFlowNodeBase::s_pFirst;                               \
	    while (pFactory && gEnv->pFlowSystem)                                                          \
	    {                                                                                              \
	      TFlowNodeTypeId nodeId = gEnv->pFlowSystem->RegisterType(pFactory->m_szClassName, pFactory); \
	      m_registeredFlowNodeIds.push_back(nodeId);                                                   \
	      CryLog("Successfully registered flownode '%s'", pFactory->m_szClassName);                    \
	      pFactory = pFactory->m_pNext;                                                                \
	    }                                                                                              \
	    return (m_registeredFlowNodeIds.size() > 0);                                                   \
	  }
#else
	#define PLUGIN_FLOWNODE_REGISTER
#endif

#ifndef _LIB
	#define PLUGIN_FLOWNODE_UNREGISTER                                                             \
	  virtual bool UnregisterFlowNodes() override                                                  \
	  {                                                                                            \
	    bool bSuccess = true;                                                                      \
	    const size_t numFlownodes = m_registeredFlowNodeIds.size();                                \
	                                                                                               \
	    for (size_t i = 0; i < numFlownodes; ++i)                                                  \
	    {                                                                                          \
	      if (gEnv->pFlowSystem)                                                                   \
	      {                                                                                        \
	        const char* szNameBuffer = gEnv->pFlowSystem->GetTypeName(m_registeredFlowNodeIds[i]); \
	        CryLog("Unregistering flownode '%s'", szNameBuffer);                                   \
	        if (!gEnv->pFlowSystem->UnregisterType(szNameBuffer))                                  \
	        {                                                                                      \
	          bSuccess = false;                                                                    \
	          CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR,                                 \
	                     "Error unregistering flownode '%s'", szNameBuffer);                       \
	        }                                                                                      \
	      }                                                                                        \
	    }                                                                                          \
	    m_registeredFlowNodeIds.clear();                                                           \
	    return bSuccess;                                                                           \
	  }
#else
	#define PLUGIN_FLOWNODE_UNREGISTER
#endif
