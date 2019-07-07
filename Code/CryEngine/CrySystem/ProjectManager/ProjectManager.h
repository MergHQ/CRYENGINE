// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ISystem.h>
#include <CrySystem/IProjectManager.h>
#include <CrySystem/ICryPluginManager.h>

namespace Cry
{

class CProjectManager final
	: public IProjectManager
	, public ILoadConfigurationEntrySink
{
public:
	CProjectManager();

	bool ParseProjectFile();
	void MigrateFromLegacyWorkflowIfNecessary();

	// IProjectManager
	virtual const char*  GetCurrentProjectName() const override;
	virtual CryGUID      GetCurrentProjectGUID() const override;
	virtual const char*  GetCurrentEngineID() const override;

	virtual const char*  GetCurrentProjectDirectoryAbsolute() const override;

	virtual const char*  GetCurrentAssetDirectoryRelative() const override;
	virtual const char*  GetCurrentAssetDirectoryAbsolute() const override;
	virtual const char*  GetProjectFilePath() const override;

	virtual void         StoreConsoleVariable(const char* szCVarName, const char* szValue) override;
	virtual void         SaveProjectChanges() override;

	virtual const uint16 GetPluginCount() const override { return static_cast<uint16>(m_project.plugins.size()); }
	virtual void         GetPluginInfo(uint16 index, IPluginManager::EType& typeOut, string& pathOut, DynArray<EPlatform>& platformsOut) const override;

	virtual string       LoadTemplateFile(const char* szPath, std::function<string(const char* szAlias)> aliasReplacementFunc) const override;
	// ~IProjectManager

	// ILoadConfigurationEntrySink
	virtual void OnLoadConfigurationEntry(const char* szKey, const char* szValue, const char* szGroup) override;
	// ~ILoadConfigurationEntrySink

	const std::vector<SPluginDefinition>& GetPluginDefinitions() const { return m_project.plugins; }

protected:
	void RegisterCVars();

	void LoadLegacyPluginCSV();
	void LoadLegacyGameCfg();
	void AddDefaultPlugins(unsigned int previousVersion);

	void AddPlugin(const SPluginDefinition& definition);

	bool CanMigrateFromLegacyWorkflow() const { return m_project.version == 0 && m_sys_game_folder->GetString()[0] != '\0' && !m_project.filePath.empty(); }

protected:
	SProject m_project;

	ICVar*   m_sys_project;

	// Legacy CVars
	ICVar* m_sys_game_name;
	ICVar* m_sys_dll_game;
	ICVar* m_sys_game_folder;
};

}
