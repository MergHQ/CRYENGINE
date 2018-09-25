// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include <StdAfx.h>
#include "UserDataUtil.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>

#include <QtUtil.h>

namespace UserDataUtil
{
const unsigned currentVersion = 1;

bool Migrate(const QString& from, const QString& to)
{
	QFileInfo fileInfo(from);
	if (fileInfo.isDir())
	{
		QDir fromDir(from);
		if (!fromDir.exists())
			return false;

		QString newDir = to + "/" + fromDir.dirName();
		QDir toDir(newDir);
		toDir.mkpath(toDir.absolutePath());

		QFileInfoList infoList = fromDir.entryInfoList(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
		for (const QFileInfo& fileInfo : infoList)
		{
			if (!Migrate(from + "/" + fileInfo.fileName(), newDir))
				return false;
		}
	}
	else
	{
		QString destination(to + "/" + fileInfo.fileName());

		// Don't replace any existing files
		if (QFile::exists(destination))
			return true;

		// Make sure the folder exists in the versioned user data
		QFileInfo fileInfo(destination);
		QDir toDir(fileInfo.absolutePath());
		toDir.mkpath(fileInfo.absolutePath());

		return QFile::copy(from, destination);
	}

	return true;
}

bool Migrate(const char* szRelativeFilePath)
{
	unsigned lastVersion = currentVersion;

	QString userDataRootPath(QtUtil::GetAppDataFolder() + "/");
	const QString currentVersionPath(userDataRootPath + QString::number(currentVersion));

	// Get latest file version
	while (!QFileInfo::exists(userDataRootPath + QString::number(lastVersion) + "/" + szRelativeFilePath) && --lastVersion)
		;

	// If there's a version mismatch, we must try to migrate data
	if (lastVersion != currentVersion)
	{
		// If last version is a valid version, then copy it's directory into current version for migration
		QString lastVersionPath;
		if (lastVersion)
		{
			lastVersionPath = userDataRootPath + QString::number(lastVersion) + "/" + szRelativeFilePath;
		}
		else
		{
			lastVersionPath = userDataRootPath + szRelativeFilePath;
		}

		// If directory was copied successfully, then return the current version path
		return Migrate(lastVersionPath, currentVersionPath);
	}

	return true;
}

QString GetUserPath(const char* szRelativeFilePath)
{
	return QtUtil::GetAppDataFolder() + "/" + QString::number(currentVersion) + "/" + szRelativeFilePath;
}

QVariant Load(const char* szRelativeFilePath)
{
	const QString filePath(GetUserPath(szRelativeFilePath));
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly))
	{
		QString msg = "Failed to open path: " + filePath;
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_COMMENT, QtUtil::ToConstCharPtr(msg));
		return QVariant();
	}

	QJsonDocument doc(QJsonDocument::fromJson(file.readAll()));

	return doc.toVariant();
}

void Save(const char* szRelativeFilePath, const char* data)
{
	QString filePath(GetUserPath(szRelativeFilePath));
	// Remove filename from path
	QDir dir(QFileInfo(filePath).absolutePath());
	// Make sure the directory exists
	dir.mkpath(dir.absolutePath());

	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly))
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "Failed to open path: %s", QtUtil::ToConstCharPtr(filePath));
		return;
	}

	file.write(data);
}
} // namespace UserDataUtil

CUserData::CUserData(std::vector<string> userDataPaths)
{
	// Automatically migrate any relevant data to new version folder
	for (auto& userDataPath : userDataPaths)
	{
		UserDataUtil::Migrate(userDataPath.c_str());
	}
}

CUserData::~CUserData()
{

}
