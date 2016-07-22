// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ISystem.h>
#include <CryExtension/ClassWeaver.h>
#include <CryExtension/ICryUnknown.h>
#include <CryFlowGraph/IFlowBaseNode.h>

struct SSystemInitParams;

//! Base Interface for plugins.
struct ICryPlugin : public ICryUnknown
{
	CRYINTERFACE_DECLARE(ICryPlugin, 0xf899cf661df04f60, 0xa341a8a7ffdf9de5);

	//! Retrieve name of the plugin.
	virtual const char* GetName() const = 0;

	//! Retrieve category of the plugin.
	virtual const char* GetCategory() const = 0;

	//! This is called to initialize the new plugin.
	virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) = 0;

	virtual void PreUpdate() = 0;

	virtual void Update(int updateFlags, int nPauseMode) = 0;

	virtual void PostUpdate() = 0;

	virtual bool RegisterFlowNodes()                               { return false; }

	virtual bool UnregisterFlowNodes()                             { return false; }

	void         SetAssetDirectory(const char* szAssetDirectory)   { cry_strcpy(m_szAssetDirectory, szAssetDirectory); }
	void         SetBinaryDirectory(const char* szBinaryDirectory) { cry_strcpy(m_szBinaryDirectory, szBinaryDirectory); }

protected:
	char                         m_szAssetDirectory[_MAX_PATH];
	char                         m_szBinaryDirectory[_MAX_PATH];
	std::vector<TFlowNodeTypeId> m_registeredFlowNodeIds;
};

DECLARE_SHARED_POINTERS(ICryPlugin);

#ifndef _LIB
	#define USE_CRYPLUGIN_FLOWNODES                                 \
	  CAutoRegFlowNodeBase * CAutoRegFlowNodeBase::m_pFirst = nullptr; \
	  CAutoRegFlowNodeBase* CAutoRegFlowNodeBase::m_pLast = nullptr;
#else
	#define USE_CRYPLUGIN_FLOWNODES
#endif

#ifndef _LIB
	#define PLUGIN_FLOWNODE_REGISTER                                                        \
	  virtual bool RegisterFlowNodes() override                                             \
	  {                                                                                     \
	    m_registeredFlowNodeIds.clear();                                                    \
                                                                                          \
	    CAutoRegFlowNodeBase* pFactory = CAutoRegFlowNodeBase::m_pFirst;                    \
	    while (pFactory)                                                                    \
	    {                                                                                   \
	      char szNameBuffer[256];                                                           \
	      cry_strcpy(szNameBuffer, GetName());                                              \
	      cry_strcat(szNameBuffer, ":");                                                    \
	      cry_strcat(szNameBuffer, pFactory->m_sClassName);                                 \
                                                                                          \
	      TFlowNodeTypeId nodeId = gEnv->pFlowSystem->RegisterType(szNameBuffer, pFactory); \
	      m_registeredFlowNodeIds.push_back(nodeId);                                        \
	      CryLog("Successfully registered flownode '%s'", szNameBuffer);                    \
                                                                                          \
	      pFactory = pFactory->m_pNext;                                                     \
	    }                                                                                   \
                                                                                          \
	    return (m_registeredFlowNodeIds.size() > 0);                                        \
	  }
#else
	#define PLUGIN_FLOWNODE_REGISTER
#endif

#ifndef _LIB
	#define PLUGIN_FLOWNODE_UNREGISTER                                                          \
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
