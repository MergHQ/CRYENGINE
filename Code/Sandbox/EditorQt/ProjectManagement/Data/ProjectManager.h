// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryIcon.h>
#include <CrySerialization/Forward.h>
#include <vector>

// Information required for:
// 1. Represent existing project in GUI
// 2. Update different json configs
struct SProjectDescription
{
	SProjectDescription();

	void Serialize(Serialization::IArchive& ar);
	int  GetVersionMajor() const;
	int  GetVersionMinor() const;
	bool IsValid() const;

	bool FindAndUpdateCryProjPath();

	string  id;         // Unix data time of the project creation (seconds)
	string  name;
	string  rootFolder;
	int     state;

	string  engineKey;
	string  engineVersion;
	int     engineBuild;

	string  templateName; // Statistics, not used in Sandbox
	string  templatePath; // Statistics, not used in Sandbox
	uint64  lastOpened;   // Unix data time (seconds)
	string  language;     // Can be empty, used by statistics only

	string  fullPathToCryProject; // Used in Sandbox only

	CryIcon icon;
};

// Loads and keeps track of project descriptions
// These information is shared between Sandbox and Launcher in single Json file
class CProjectManager
{
public:
	CProjectManager();

	const std::vector<SProjectDescription>& GetProjects() const;

	const SProjectDescription*              GetLastUsedProject() const;
	void                                    AddProject(const SProjectDescription& projectDescr);
	bool                                    SetCurrentTime(const SProjectDescription& projectDescr);

	CCrySignal<void()>                           signalBeforeProjectsUpdated;
	CCrySignal<void()>                           signalAfterProjectsUpdated;
	CCrySignal<void(const SProjectDescription*)> signalProjectDataChanged;

private:
	void SaveProjectDescriptions();

	const string                     m_pathToProjectList;

	std::vector<SProjectDescription> m_validProjects;

	// No one model will get data from here. Used as write-as-read data
	std::vector<SProjectDescription> m_invalidProjects;
};