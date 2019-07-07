// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "TemplateManager.h"

#include "ProjectManagement/Utils.h"

#include <FileUtils.h>
#include <QtUtil.h>

#include <CrySerialization/yasli/JSONIArchive.h>
#include <CrySystem/IProjectManager.h>

#include <QDirIterator>
#include <map>

namespace Private_TemplateManager
{

//Entry inside CryEngine.json
struct SEngineDescription
{
	void Serialize(Serialization::IArchive& ar)
	{
		ar(version, "info");
		ar(uri, "uri");
	}

	SCryEngineVersion version;
	string            uri;
};

// Get paths for current instance of the CryEngine
bool GetCryEngineTemplatesRoot(QString& engineRoot, QString& templatesRoot)
{
	engineRoot = "";
	templatesRoot = "";

	const SCryEngineVersion currEngineVersion = GetCurrentCryEngineVersion();
	if (!currEngineVersion.IsValid())
	{
		CRY_ASSERT_MESSAGE(0, "Cannot determinate CryEngine version");
		return false;
	}

	string config = GetCryEngineProgramDataFolder() + "cryengine.json";
	yasli::JSONIArchive ia;
	ia.load(config);

	std::map<string, SEngineDescription> engines;
	ia(engines);

	// Try to find exact match
	for (const auto& engine : engines)
	{
		if (currEngineVersion.Equal(engine.second.version, true))
		{
			QFileInfo info(engine.second.uri.c_str());
			QDir rootDir = info.absoluteDir();

			QString templatesFolder = QDir::cleanPath(rootDir.path() + "/Templates");
			if (FileUtils::FolderExists(templatesFolder.toStdString().c_str()))
			{
				engineRoot = QDir::cleanPath(rootDir.path());
				templatesRoot = templatesFolder;
				return true;
			}
		}
	}

	// Try to find not exact match (engine-dev + 5.5 <=> engine-5.5 + 5.5)
	for (const auto& engine : engines)
	{
		if (currEngineVersion.Equal(engine.second.version, false))
		{
			QFileInfo info(engine.second.uri.c_str());
			QDir rootDir = info.absoluteDir();

			QString templatesFolder = QDir::cleanPath(rootDir.path() + "/Templates");
			if (FileUtils::FolderExists(templatesFolder.toStdString().c_str()))
			{
				engineRoot = QDir::cleanPath(rootDir.path());
				templatesRoot = templatesFolder;
				return true;
			}
		}
	}

	CRY_ASSERT_MESSAGE(0, "Can not find templates root");
	return false;
}

string AdjustLanguageName(const QString& folder)
{
	QFileInfo fileInfo(folder);
	string language = fileInfo.baseName().toStdString().c_str();
	if (language.CompareNoCase("cs") == 0)
	{
		return "C#";
	}

	if (language.CompareNoCase("cpp") == 0)
	{
		return "C++";
	}

	if (language.CompareNoCase("schematyc") == 0)
	{
		return "Schematyc";
	}

	// Not modified
	return language;
}

void FindProjectsInLanguageFolder(const QString& folder, const int engineRootSize, std::vector<STemplateDescription>& templates)
{
	const string language = AdjustLanguageName(folder);
	if (language == "Schematyc")
	{
		// Schematyc is not supported by Sandbox
		return;
	}

	QFileInfo fileInfo(folder + '/');
	QDirIterator iterator(fileInfo.absolutePath(), QStringList() << "*.cryproject", QDir::Files, QDirIterator::Subdirectories);
	while (iterator.hasNext())
	{
		iterator.next();
		const auto fi = iterator.fileInfo();

		STemplateDescription descr;
		descr.language = language;
		descr.templateFolder = QtUtil::ToString(fi.dir().absolutePath());
		descr.cryprojFileName = QtUtil::ToString(fi.fileName());

		// Project name (from template)
		{
			yasli::JSONIArchive ia;
			ia.load(fi.absoluteFilePath().toStdString().c_str());

			Cry::SProject proj;
			ia(proj);
			descr.templateName = proj.name;

			if (descr.templateName.empty())
			{
				CRY_ASSERT_MESSAGE(descr.templateName.empty(), "Failed to get template name");
				descr.templateName = QtUtil::ToString(fi.dir().dirName());
			}
		}

		descr.templatePath = QtUtil::ToString(fi.absolutePath()).substr(engineRootSize);

		descr.icon = GetProjectThumbnail(descr.templateFolder, "icons:General/Template_Default.ico");
		templates.push_back(descr);
	}
}

std::vector<STemplateDescription> EnumerateProjectTemplates()
{
	QString engineRoot;
	QString templatesRoot;
	if (!GetCryEngineTemplatesRoot(engineRoot, templatesRoot))
	{
		return {};
	}

	std::vector<STemplateDescription> templates;

	QDir dir(templatesRoot);
	QFileInfoList list = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
	for (int i = 0; i < list.size(); ++i)
	{
		const QFileInfo& fileInfo = list.at(i);
		const QString subFolder = fileInfo.absoluteFilePath();

		FindProjectsInLanguageFolder(subFolder, engineRoot.size() + 1, templates);
	}

	return templates;
}

} // namespace Private_TemplateManager

CTemplateManager::CTemplateManager()
	: m_templates(Private_TemplateManager::EnumerateProjectTemplates())
{
}

const std::vector<STemplateDescription>& CTemplateManager::GetTemplates() const
{
	return m_templates;
}