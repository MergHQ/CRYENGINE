// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ProjectManager.h"
#include "ProjectManagement/UI/SelectProjectDialog.h"
#include "ProjectManagement/Utils.h"

#include <FileUtils.h>
#include <PathUtils.h>

#include <CrySerialization/yasli/JSONIArchive.h>
#include <CrySerialization/yasli/JSONOArchive.h>

#include <QDateTime>
#include <QDirIterator>

namespace Private_ProjectManager
{

string FindFirstFileInFolder(string folder, const char* szTemplate)
{
	//Non-recursive search (in root folder only) for a FIRST file with szTemplate
	const QStringList filter(szTemplate);
	QDirIterator it(folder.c_str(), filter, QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot);
	while (it.hasNext())
	{
		return it.next().toStdString().c_str();
	}

	return "";
}

struct JsonData
{
	void Serialize(Serialization::IArchive& ar)
	{
		if (!ar.isOutput())
		{
			// Loading
			int version = 0;
			ar(version, "version");
			switch (version)
			{
			case 1:
				ar(projects, "projects");
				break;
			}
		}
		else
		{
			// Saving only in the latest version
			ar(1, "version");
			ar(projects, "projects");
		}
	}

	std::vector<SProjectDescription> projects;
};

}

SProjectDescription::SProjectDescription()
	: state(0)
	, engineBuild(0)
	, lastOpened(0)
{
}

void SProjectDescription::Serialize(Serialization::IArchive& ar)
{
	ar(id, "id", "id");
	ar(name, "name", "name");

	if (ar.isOutput())
	{
		// Save in Launcher format
		string tmp = PathUtil::ToDosPath(rootFolder);
		ar(tmp, "path", "path");
	}
	else
	{
		// Convert on-load
		string tmp;
		ar(tmp, "path", "path");
		rootFolder = PathUtil::ToUnixPath(tmp);
	}

	ar(state, "state", "state");
	ar(language, "language", "Language");
	ar(templateName, "templateName", "Template Name");
	ar(templatePath, "templatePath", "Template Path");
	ar(engineKey, "engineKey", "Engine Key");
	ar(engineBuild, "engineBuild", "Engine Build");
	ar(engineVersion, "engineVersion", "Engine Version");
	ar(lastOpened, "lastOpened", "Last Opened");

	ar(fullPathToCryProject, "fullPathToCryProject", "Full Path To CryProject");
	if (ar.isInput())
	{
		fullPathToCryProject = PathUtil::ToUnixPath(fullPathToCryProject);
	}

	if (ar.isInput())
	{
		string driveLetter;
		string directory;
		string originalFilename;
		string extension;

		PathUtil::SplitPath(fullPathToCryProject, driveLetter, directory, originalFilename, extension);
		icon = GetProjectThumbnail(driveLetter + directory, "icons:General/Project_Default.ico");
	}
}

int SProjectDescription::GetVersionMajor() const
{
	return engineVersion[0] - '0';
}

int SProjectDescription::GetVersionMinor() const
{
	return engineVersion[2] - '0';
}

bool SProjectDescription::IsValid() const
{
	// Format: "5.5"
	return engineVersion.length() == 3 && engineVersion[1] == '.';
}

bool SProjectDescription::FindAndUpdateCryProjPath()
{
	if (FileUtils::FileExists(fullPathToCryProject))
	{
		return true;
	}

	if (!FileUtils::FolderExists(rootFolder))
	{
		return false;
	}

	string foundPath = Private_ProjectManager::FindFirstFileInFolder(rootFolder, "*.cryproject");
	if (foundPath.empty())
	{
		return false;
	}

	// Update only in one case: when we are 100% sure, that the cryproject file exists
	fullPathToCryProject.swap(foundPath);
	return true;
}

//////////////////////////////////////////////////////////////////////////
CProjectManager::CProjectManager()
	: m_pathToProjectList(GetCryEngineProgramDataFolder() + "projects.json")
{
	Private_ProjectManager::JsonData jsonData;
	yasli::JSONIArchive ia;
	if (ia.load(m_pathToProjectList.c_str()))
	{
		ia(jsonData);
	}

	const SCryEngineVersion currEngineVersion = GetCurrentCryEngineVersion();

	for (auto& proj : jsonData.projects)
	{
		if (!proj.IsValid())
		{
			m_invalidProjects.push_back(proj);
			continue;
		}

		if (currEngineVersion.GetVersionMajor() != proj.GetVersionMajor() || currEngineVersion.GetVersionMinor() != proj.GetVersionMinor())
		{
			m_invalidProjects.push_back(proj);
			continue;
		}

		if (!proj.FindAndUpdateCryProjPath())
		{
			m_invalidProjects.push_back(proj);
		}
		else
		{
			m_validProjects.push_back(proj);
		}
	}
}

const std::vector<SProjectDescription>& CProjectManager::GetProjects() const
{
	return m_validProjects;
}

const SProjectDescription* CProjectManager::GetLastUsedProject() const
{
	auto it = std::max_element(m_validProjects.cbegin(), m_validProjects.cend(), [](const SProjectDescription& lhs, const SProjectDescription& rhs)
	{
		return lhs.lastOpened < rhs.lastOpened;
	});

	if (it == m_validProjects.cend())
	{
		return nullptr;
	}

	return &(*it);
}

void CProjectManager::AddProject(const SProjectDescription& projectDescr)
{
	signalBeforeProjectsUpdated();
	m_validProjects.push_back(projectDescr);
	signalAfterProjectsUpdated();

	SaveProjectDescriptions();
}

bool CProjectManager::SetCurrentTime(const SProjectDescription& projectDescr)
{
	auto it = std::find_if(m_validProjects.begin(), m_validProjects.end(), [projectDescr](SProjectDescription& curr) -> bool
	{
		return curr.fullPathToCryProject.CompareNoCase(projectDescr.fullPathToCryProject) == 0;
	});

	if (it == m_validProjects.end())
	{
		return false;
	}

	it->lastOpened = QDateTime::currentDateTime().toTime_t();
	SaveProjectDescriptions();
	signalProjectDataChanged(&(*it));
	return true;
}

void CProjectManager::SaveProjectDescriptions()
{
	Private_ProjectManager::JsonData toSave;
	toSave.projects = m_validProjects;
	toSave.projects.insert(toSave.projects.end(), m_invalidProjects.begin(), m_invalidProjects.end());

	yasli::JSONOArchive oa;
	oa(toSave, "projects");
	oa.save(m_pathToProjectList.c_str());
}