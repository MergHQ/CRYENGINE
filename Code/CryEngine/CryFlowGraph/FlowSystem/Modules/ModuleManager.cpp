// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ModuleManager.h"

#include "Module.h"
#include "FlowModuleNodes.h"

#include <CryFlowGraph/IFlowBaseNode.h>

#define MODULE_FOLDER_NAME ("\\FlowgraphModules\\")

#if !defined (_RELEASE)
void RenderModuleDebugInfo();
#endif

AllocateConstIntCVar(CFlowGraphModuleManager, CV_fg_debugmodules);

IFlowGraphModule* CFlowGraphModuleManager::CModuleIterator::Next()
{
	if (m_cur == m_pModuleManager->m_Modules.end())
		return NULL;
	IFlowGraphModule* pCur = m_cur->second;
	++m_cur;
	return pCur;
}

//////////////////////////////////////////////////////////////////////////

CFlowGraphModuleManager::CFlowGraphModuleManager()
	: m_listeners(1)
{
	m_nextModuleId = 0;
	gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);

	DefineConstIntCVarName("fg_debugmodules", CV_fg_debugmodules, 0, VF_NULL, "Display Flowgraph Modules debug information.\n" \
	  "0=Disabled"                                                                                           \
	  "1=Modules only"                                                                                       \
	  "2=Modules + Module Instances");
	fg_debugmodules_filter = REGISTER_STRING("fg_debugmodules_filter", "", VF_NULL,
	                                         "List of module names to display with the CVar 'fg_debugmodules'. Partial names can be supplied, but not regular expressions.");

#if !defined (_RELEASE)
	CRY_ASSERT_MESSAGE(gEnv->pGameFramework, "Unable to register as Framework listener!");
	if (gEnv->pGameFramework)
	{
		gEnv->pGameFramework->RegisterListener(this, "FlowGraphModuleManager", FRAMEWORKLISTENERPRIORITY_GAME);
	}
#endif
}

CFlowGraphModuleManager::~CFlowGraphModuleManager()
{
	Shutdown();
	gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);

	gEnv->pConsole->UnregisterVariable("fg_debugmodules", true);
	gEnv->pConsole->UnregisterVariable("fg_debugmodules_filter", true);

#if !defined (_RELEASE)
	if (gEnv->pGameFramework)
	{
		gEnv->pGameFramework->UnregisterListener(this);
	}
#endif
}

void CFlowGraphModuleManager::Shutdown()
{
	ClearModules();
	m_listeners.Clear();
}

void CFlowGraphModuleManager::ClearModules()
{
	TModuleMap::iterator i = m_Modules.begin();
	TModuleMap::iterator end = m_Modules.end();
	for (; i != end; ++i)
	{
		if (i->second)
		{
			for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
			{
				notifier->OnModuleDestroyed(i->second);
			}

			i->second->Destroy();
			SAFE_DELETE(i->second);

			for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
			{
				notifier->OnPostModuleDestroyed();
			}
		}
	}
	m_Modules.clear();
	m_ModuleIds.clear();
	m_ModulesPathInfo.clear();
	m_nextModuleId = 0;
}

/* Serialization */

CFlowGraphModule* CFlowGraphModuleManager::PreLoadModuleFile(const char* moduleName, const char* fileName, bool bGlobal)
{
	// NB: the module name passed in might be a best guess based on the filename. The actual name
	// comes from within the module xml.

	// first check for existing module
	CFlowGraphModule* pModule = static_cast<CFlowGraphModule*>(GetModule(moduleName));
	if (pModule)
	{
		for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
		{
			notifier->OnModuleDestroyed(pModule);
		}
		// exists, reload
		pModule->Destroy();
		pModule->PreLoadModule(fileName);

		for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
		{
			notifier->OnPostModuleDestroyed();
		}
	}
	else
	{
		// not found, create
		pModule = new CFlowGraphModule(m_nextModuleId++);
		pModule->SetType(bGlobal ? IFlowGraphModule::eT_Global : IFlowGraphModule::eT_Level);

		TModuleId id = pModule->GetId();
		m_Modules[id] = pModule;

		pModule->PreLoadModule(fileName);
		AddModulePathInfo(pModule->GetName(), fileName);

		m_ModuleIds[pModule->GetName()] = id;
	}

	return pModule;
}

void CFlowGraphModuleManager::LoadModuleGraph(const char* moduleName, const char* fileName, IFlowGraphModuleListener::ERootGraphChangeReason rootGraphChangeReason)
{
	LOADING_TIME_PROFILE_SECTION_ARGS(fileName);
	// first check for existing module - must exist by this point
	CFlowGraphModule* pModule = static_cast<CFlowGraphModule*>(GetModule(moduleName));
	assert(pModule);

	if (pModule)
	{
		if (pModule->LoadModuleGraph(moduleName, fileName))
		{
			for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
			{
				notifier->OnRootGraphChanged(pModule, rootGraphChangeReason);
			}
		}
	}
}

IFlowGraphModule* CFlowGraphModuleManager::LoadModuleFile(const char* moduleName, const char* fileName, bool bGlobal)
{
	// first check for existing module
	CFlowGraphModule* pModule = static_cast<CFlowGraphModule*>(GetModule(moduleName));
	if (pModule)
	{
		for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
		{
			notifier->OnModuleDestroyed(pModule);
		}

		pModule->Destroy();

		for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
		{
			notifier->OnPostModuleDestroyed();
		}
	}

	pModule = PreLoadModuleFile(moduleName, fileName, bGlobal);
	LoadModuleGraph(moduleName, fileName, IFlowGraphModuleListener::ERootGraphChangeReason::LoadModuleFile);

	return pModule;
}

bool CFlowGraphModuleManager::DeleteModuleXML(const char* moduleName)
{
	if (m_ModulesPathInfo.empty())
		return false;

	TModulesPathInfo::iterator modulePathEntry = m_ModulesPathInfo.find(moduleName);
	if (m_ModulesPathInfo.end() != modulePathEntry)
	{
		if (remove(modulePathEntry->second) == 0)
		{
			m_ModulesPathInfo.erase(moduleName);

			// also remove module itself
			TModuleIdMap::iterator idIt = m_ModuleIds.find(moduleName);
			assert(idIt != m_ModuleIds.end());
			TModuleId id = idIt->second;
			TModuleMap::iterator modIt = m_Modules.find(id);
			assert(modIt != m_Modules.end());

			if (modIt != m_Modules.end() && idIt != m_ModuleIds.end())
			{
				for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
				{
					notifier->OnModuleDestroyed(GetModule(id));
				}

				m_Modules[id]->Destroy();
				delete m_Modules[id];
				m_Modules[id] = NULL;

				m_ModuleIds.erase(idIt);
				m_Modules.erase(modIt);

				if (m_Modules.empty())
					m_nextModuleId = 0;

				for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
				{
					notifier->OnPostModuleDestroyed();
				}
			}

			return true;
		}
	}

	return false;
}

bool CFlowGraphModuleManager::RenameModuleXML(const char* moduleName, const char* newName)
{
	if (m_ModulesPathInfo.empty())
		return false;

	TModulesPathInfo::iterator modulePathEntry = m_ModulesPathInfo.find(moduleName);
	if (m_ModulesPathInfo.end() != modulePathEntry)
	{
		string newNameWithPath = PathUtil::GetPathWithoutFilename(modulePathEntry->second);
		newNameWithPath += newName;
		newNameWithPath += ".xml";

		if (rename(modulePathEntry->second, newNameWithPath.c_str()) == 0)
		{
			m_ModulesPathInfo.insert(TModulesPathInfo::value_type(PathUtil::GetFileName(newNameWithPath), newNameWithPath));
			m_ModulesPathInfo.erase(moduleName);
			return true;
		}
	}

	return false;
}

void CFlowGraphModuleManager::ScanFolder(const string& folderName, bool bGlobal)
{
	_finddata_t fd;
	intptr_t handle = 0;
	ICryPak* pPak = gEnv->pCryPak;

	CryFixedStringT<512> searchString = folderName.c_str();
	searchString.append("*.*");

	handle = pPak->FindFirst(searchString.c_str(), &fd);

	CryFixedStringT<512> moduleName("");
	string newFolder("");

	if (handle > -1)
	{
		do
		{
			if (!strcmp(fd.name, ".") || !strcmp(fd.name, "..") || (fd.attrib & _A_HIDDEN))
				continue;

			if (fd.attrib & _A_SUBDIR)
			{
				newFolder = folderName;
				newFolder = newFolder + fd.name;
				newFolder = newFolder + "\\";
				ScanFolder(newFolder, bGlobal);
			}
			else
			{
				moduleName = fd.name;
				if (!strcmpi(PathUtil::GetExt(moduleName.c_str()), "xml"))
				{
					PathUtil::RemoveExtension(moduleName);
					PathUtil::MakeGamePath(folderName);

					// initial load: creates module, registers nodes
					CFlowGraphModule* pModule = PreLoadModuleFile(moduleName.c_str(), PathUtil::GetPathWithoutFilename(folderName) + fd.name, bGlobal);
					// important: the module should be added using its internal name rather than the filename
					m_ModulesPathInfo.insert(TModulesPathInfo::value_type(pModule->GetName(), PathUtil::GetPathWithoutFilename(folderName) + fd.name));
				}
			}

		}
		while (pPak->FindNext(handle, &fd) >= 0);

		pPak->FindClose(handle);
	}
}

void CFlowGraphModuleManager::RescanModuleNames(bool bGlobal)
{
	LOADING_TIME_PROFILE_SECTION;
	CryFixedStringT<512> path = "";

	if (bGlobal)
	{
		path = PathUtil::GetGameFolder().c_str();
		path += "\\Libs\\";
	}
	else
	{
		if (gEnv->IsEditor())
		{
			char* levelName;
			char* levelPath;
			gEnv->pGameFramework->GetEditorLevel(&levelName, &levelPath);
			path = levelPath;
		}
		else if (gEnv->pGameFramework->GetILevelSystem())
		{
			ILevelInfo* pLevel = gEnv->pGameFramework->GetILevelSystem()->GetCurrentLevel();
			if (pLevel)
			{
				path = pLevel->GetPath();
			}
		}
	}

	if (false == path.empty())
	{
		path += MODULE_FOLDER_NAME;
		ScanFolder(path, bGlobal);
	}
}

void CFlowGraphModuleManager::ScanForModules()
{
	LOADING_TIME_PROFILE_SECTION;

	// first remove any existing modules
	ClearModules();

	// during scanning, modules will be PreLoaded - created, and nodes registered. This ensures that when we actually
	//	load the graphs, the nodes already exist - even if one module contains the nodes relating to another module.
	RescanModuleNames(true);
	RescanModuleNames(false);

	// Second pass: loading the graphs, now all nodes should exist.
	for (TModulesPathInfo::const_iterator it = m_ModulesPathInfo.begin(), end = m_ModulesPathInfo.end(); it != end; ++it)
	{
		LoadModuleGraph(it->first, it->second, IFlowGraphModuleListener::ERootGraphChangeReason::ScanningForModules);
	}

	for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
	{
		notifier->OnScannedForModules();
	}
}

bool CFlowGraphModuleManager::SaveModule(const char* moduleName, XmlNodeRef saveTo)
{
	TModuleId moduleId = stl::find_in_map(m_ModuleIds, moduleName, MODULEID_INVALID);
	if (moduleId != MODULEID_INVALID)
	{
		CFlowGraphModule* pModule = stl::find_in_map(m_Modules, moduleId, nullptr);
		if (pModule)
		{
			return pModule->SaveModuleXml(saveTo);
		}
	}

	return false;
}

/* Iterator */
IModuleIteratorPtr CFlowGraphModuleManager::CreateModuleIterator()
{
	if (m_iteratorPool.empty())
	{
		return new CModuleIterator(this);
	}
	else
	{
		CModuleIterator* pIter = m_iteratorPool.back();
		m_iteratorPool.pop_back();
		new(pIter) CModuleIterator(this);
		return pIter;
	}
}

/* Getters/Setters */

IFlowGraphModule* CFlowGraphModuleManager::GetModule(TModuleId moduleId) const
{
	return stl::find_in_map(m_Modules, moduleId, NULL);
}

IFlowGraphModule* CFlowGraphModuleManager::GetModule(const char* moduleName) const
{
	TModuleId id = stl::find_in_map(m_ModuleIds, moduleName, MODULEID_INVALID);
	return GetModule(id);
}

IFlowGraphModule* CFlowGraphModuleManager::GetModule(IFlowGraphPtr pFlowgraph) const
{
	for (TModuleMap::const_iterator it = m_Modules.begin(), end = m_Modules.end(); it != end; ++it)
	{
		CFlowGraphModule* pModule = it->second;

		if (pModule && pModule->HasInstanceGraph(pFlowgraph))
		{
			return pModule;
		}
	}

	return NULL;
}

const char* CFlowGraphModuleManager::GetModulePath(const char* name)
{
	if (m_ModulesPathInfo.empty())
		return NULL;

	TModulesPathInfo::iterator modulePathEntry = m_ModulesPathInfo.find(name);
	if (m_ModulesPathInfo.end() != modulePathEntry)
	{
		return modulePathEntry->second;
	}

	return NULL;
}

bool CFlowGraphModuleManager::AddModulePathInfo(const char* moduleName, const char* path)
{
	TModulesPathInfo::iterator modulePathEntry = m_ModulesPathInfo.find(moduleName);
	if (m_ModulesPathInfo.end() != modulePathEntry)
	{
		return false;
	}

	m_ModulesPathInfo.insert(TModulesPathInfo::value_type(moduleName, path));
	return true;
}

const char* CFlowGraphModuleManager::GetStartNodeName(const char* moduleName) const
{
	static CryFixedStringT<64> temp;
	temp.Format("Module:Start_%s", moduleName);
	return temp.c_str();
}

const char* CFlowGraphModuleManager::GetReturnNodeName(const char* moduleName) const
	{
	static CryFixedStringT<64> temp;
	temp.Format("Module:End_%s", moduleName);
	return temp.c_str();
	}

const char* CFlowGraphModuleManager::GetCallerNodeName(const char* moduleName) const
{
	static CryFixedStringT<64> temp;
	temp.Format("Module:Call_%s", moduleName);
	return temp.c_str();
}

/* Create Module Nodes */
bool CFlowGraphModuleManager::CreateModuleNodes(const char* moduleName, TModuleId moduleId)
{
	assert(moduleId != MODULEID_INVALID);

	CFlowGraphModule* pModule = static_cast<CFlowGraphModule*>(GetModule(moduleId));
	if (pModule)
	{
		pModule->RegisterNodes();
	}

	return true;
}

/* Module handling */
void CFlowGraphModuleManager::RemoveCompletedModuleInstances()
{
	for (TModuleMap::iterator it = m_Modules.begin(), end = m_Modules.end(); it != end; ++it)
{
		it->second->RemoveCompletedInstances();
}
}

void CFlowGraphModuleManager::DestroyActiveModuleInstances()
{
	for (TModuleMap::const_iterator it = m_Modules.begin(), end = m_Modules.end(); it != end; ++it)
	{
		if (CFlowGraphModule* pModule = it->second)
		{
			pModule->RemoveAllInstances();
			pModule->RemoveCompletedInstances();
			pModule->ClearCallNodesForInstances();
			pModule->ClearGlobalControlNodes();
		}
	}
}

void CFlowGraphModuleManager::ClearModuleRequestedInstances()
{
	for (TModuleMap::const_iterator it = m_Modules.begin(), end = m_Modules.end(); it != end; ++it)
{
		if (CFlowGraphModule* pModule = it->second)
	{
			pModule->ClearCallNodesForInstances();
		}
	}
}

bool CFlowGraphModuleManager::RegisterListener(IFlowGraphModuleListener* pListener, const char* name)
{
	return m_listeners.Add(pListener, name);
}

void CFlowGraphModuleManager::UnregisterListener(IFlowGraphModuleListener* pListener)
{
	m_listeners.Remove(pListener);
}

void CFlowGraphModuleManager::BroadcastModuleInstanceStarted(IFlowGraphModule* module, TModuleInstanceId instanceID)
{
	for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
	{
		notifier->OnModuleInstanceCreated(module, instanceID);
	}
}

void CFlowGraphModuleManager::BroadcastModuleInstanceFinished(IFlowGraphModule* module, TModuleInstanceId instanceID)
{
	for (CListenerSet<IFlowGraphModuleListener*>::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
	{
		notifier->OnModuleInstanceDestroyed(module, instanceID);
	}
}

/* ISystemEventListener */
void CFlowGraphModuleManager::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
	case ESYSTEM_EVENT_GAME_POST_INIT:
	case ESYSTEM_EVENT_LEVEL_LOAD_START:
		{
			// this has to come after all other nodes are reloaded
			// as it will trigger the editor to reload the fg classes!
			// reload all modules (since they need to register the start and end node types)
			ScanForModules();
		}
		break;
	case ESYSTEM_EVENT_LEVEL_UNLOAD:
{
			if (gEnv->pGameFramework->GetILevelSystem()->IsLevelLoaded())
				ClearModules();
		}
		break;
	case ESYSTEM_EVENT_EDITOR_GAME_MODE_CHANGED:
	case ESYSTEM_EVENT_EDITOR_SIMULATION_MODE_CHANGED:
	{
			if (wparam == 0)
		{
				DestroyActiveModuleInstances();
			}
		}
		break;
	}
}

/* IGameFrameworkListener */
void CFlowGraphModuleManager::OnPostUpdate(float fDeltaTime)
{
#if !defined (_RELEASE)
	if (CV_fg_debugmodules > 0)
	{
		RenderModuleDebugInfo();
	}
#endif
}

/* Debug Draw */
#if !defined (_RELEASE)
void inline DrawModule2dLabel(float x, float y, float fontSize, const float* pColor, const char* pText)
{
	IRenderAuxText::DrawText(Vec3(x, y, 0.5f), fontSize, IRenderAuxText::AColor(pColor),
		eDrawText_2D | eDrawText_800x600 | eDrawText_FixedSize | eDrawText_Monospace, pText);
}

void DrawModuleTextLabel(float x, float y, const float* pColor, const char* pFormat, ...)
{
	char buffer[512];

	va_list args;
	va_start(args, pFormat);
	cry_vsprintf(buffer, pFormat, args);
	va_end(args);

	DrawModule2dLabel(x, y, 1.4f, pColor, buffer);
}

void RenderModuleDebugInfo()
{
	CRY_ASSERT_MESSAGE(gEnv->pFlowSystem, "No Flowsystem available!");

	if (!gEnv->pFlowSystem || !gEnv->pFlowSystem->GetIModuleManager())
		return;

	IModuleIteratorPtr pIter = gEnv->pFlowSystem->GetIModuleManager()->CreateModuleIterator();
	if (!pIter)
		return;

	const int count = pIter->Count();

	if (count == 0)
	{
		static const float colorRed[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
		DrawModuleTextLabel(10, 20, colorRed, "NO FLOWGRAPH MODULES AVAILABLE!");
		return;
	}

	if (IFlowGraphModule* pModule = pIter->Next())
	{
		float py = 15;
		const float dy = 15;
		const float dy_space = 5;
		static const float colorWhite[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		static const float colorBlue[4] = { 0.3f, 0.8f, 1.0f, 1.0f };
		static const float colorGray[4] = { 0.3f, 0.3f, 0.3f, 1.0f };
		static const float colorGreen[4] = { 0.3f, 1.0f, 0.8f, 1.0f };

		const float col1 = 10;
		const float col4 = col1 + 350;
		const float col5 = col4 + 40;

		DrawModuleTextLabel(col1, py, colorWhite, "Module");
		DrawModuleTextLabel(col4, py, colorWhite, "Instances");

		py += dy + dy_space;

		for (int i = 0; i < count; ++i)
		{
			// skip this module if the filter is being used and it's not there
			string filter = gEnv->pConsole->GetCVar("fg_debugmodules_filter")->GetString();
			string moduleName = pModule->GetName();
			moduleName.MakeLower();
			filter.MakeLower();
			if (!filter.empty() && moduleName.find(filter) == string::npos)
			{
				pModule = pIter->Next();
				continue;
			}

			// module details
			const int runningInstances = pModule->GetRunningInstancesCount();

			DrawModuleTextLabel(col1, py, runningInstances ? colorBlue : colorGray, "%s", pModule->GetName());
			DrawModuleTextLabel(col4, py, runningInstances ? colorGreen : colorGray, "%d", runningInstances);

			// instances' details
			if (CFlowGraphModuleManager::CV_fg_debugmodules == 2 && runningInstances > 0)
			{
				IModuleInstanceIteratorPtr instanceIter = pModule->CreateInstanceIterator();
				if (!instanceIter)
				{
					continue;
				}

				DrawModuleTextLabel(col5, py, colorWhite, "IDs:");
				stack_string s_idList("");
				for (IModuleInstance* pInstance = instanceIter->Next(); pInstance; pInstance = instanceIter->Next())
					{
					if (!pInstance->m_bUsed)
						{
						continue;
						}
					EntityId entityId = static_cast<SModuleInstance*>(pInstance)->m_entityId;
					stack_string s_inst;
					if (entityId != INVALID_ENTITYID)
					{
						s_inst.Format(", %d (Entity %u)", (pInstance->m_instanceId != MODULEINSTANCE_INVALID ? pInstance->m_instanceId : -1), entityId);
					}
				else
				{
						s_inst.Format(", %u", pInstance->m_instanceId);
					}
					s_idList.append(s_inst.c_str());
				}
				DrawModuleTextLabel(col5 + 40, py, colorGreen, "%s", s_idList.size() > 2 ? s_idList.c_str() + 2 : "-");
			}

			py += dy;
			pModule = pIter->Next();
		}
	}
}
#endif
