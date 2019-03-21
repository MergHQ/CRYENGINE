// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include <StdAfx.h>
#include "FileUtils.h"
#include "UserDataUtil.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>

#include <QtUtil.h>
#include <CrySystem/ISystem.h>

namespace UserDataUtil
{
const unsigned currentVersion = 1;

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
		return FileUtils::CopyDirectory(QtUtil::ToString(lastVersionPath).c_str(), QtUtil::ToString(currentVersionPath).c_str());
	}

	return true;
}

string GetUserPath(const char* szRelativeFilePath)
{
	string userPath;
	userPath.Format("%s/%d/%s", QtUtil::ToString(QtUtil::GetAppDataFolder()), currentVersion, szRelativeFilePath);
	return userPath;
}

QVariant Load(const char* szRelativeFilePath)
{
	const string filePath(GetUserPath(szRelativeFilePath));
	QFile file(filePath.c_str());
	if (!file.open(QIODevice::ReadOnly))
	{
		string msg;
		msg.Format("Failed to open path: %s", filePath.c_str());
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_COMMENT, msg);
		return QVariant();
	}

	QJsonDocument doc(QJsonDocument::fromJson(file.readAll()));

	return doc.toVariant();
}

bool Save(const char* szRelativeFilePath, const char* data)
{
	const string filePath(GetUserPath(szRelativeFilePath));
	// Remove filename from path
	QDir dir(QFileInfo(filePath.c_str()).absolutePath());
	// Make sure the directory exists
	dir.mkpath(dir.absolutePath());

	QFile file(filePath.c_str());
	if (!file.open(QIODevice::WriteOnly))
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "Failed to open path: %s", filePath.c_str());
		return false;
	}

	return file.write(data) > 0;
}
} // namespace UserDataUtil

CUserData::CUserData(const std::vector<string>& userDataPaths)
{
	// Automatically migrate any relevant data to new version folder
	for (auto& userDataPath : userDataPaths)
	{
		UserDataUtil::Migrate(userDataPath.c_str());
	}
}
