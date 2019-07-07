// Copyright 2001-2019 Crytek GmbH. All rights reserved.
#include "StdAfx.h"
#include "DeletedWorkFilesStorage.h"
#include "PathUtils.h"
#include "QtUtil.h"

#include <QFile>
#include <QTextStream>

namespace Private_DeletedWorkFilesStorage
{

static std::vector<string> s_files;
static bool s_isDirty = false;

string GetFilePath()
{
	return PathUtil::Make(PathUtil::GetUserSandboxFolder(), "deletedWorkFiles.txt");
}

}

void CDeletedWorkFilesStorage::Add(const string& file)
{
	using namespace Private_DeletedWorkFilesStorage;
	auto it = std::lower_bound(s_files.cbegin(), s_files.cend(), file);
	if (it == s_files.cend() || *it != file)
	{
		CryLog("CDeleteWorkFilesStorage::Add file %s", file);
		s_files.insert(it, file);
		s_isDirty = true;
	}
}

void CDeletedWorkFilesStorage::Remove(const string& file)
{
	using namespace Private_DeletedWorkFilesStorage;
	auto it = std::lower_bound(s_files.cbegin(), s_files.cend(), file);
	if (it != s_files.cend() && *it == file)
	{
		CryLog("CDeleteWorkFilesStorage::Remove file %s", file);
		s_files.erase(it);
		s_isDirty = true;
	}
}

void CDeletedWorkFilesStorage::Save()
{
	using namespace Private_DeletedWorkFilesStorage;
	if (!s_isDirty)
	{
		return;
	}

	if (s_files.empty())
	{
		CryLog("CDeleteWorkFilesStorage::Save. List is empty -> delete file");
		QFile::remove(QtUtil::ToQString(GetFilePath()));
		signalSaved();
		return;
	}

	QFile file(QtUtil::ToQString(GetFilePath()));
	file.open(QIODevice::WriteOnly);

	QTextStream stream(&file);
	for (const string& filePath : s_files)
	{
		stream << filePath << '\n';
	}
	file.close();
	CryLog("CDeleteWorkFilesStorage::Save()");

	s_isDirty = false;
	signalSaved();
}

void CDeletedWorkFilesStorage::Load()
{
	using namespace Private_DeletedWorkFilesStorage;

	QFile file(QtUtil::ToQString(GetFilePath()));
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		CryLog("CDeleteWorkFilesStorage::Load. No file found");
		return;
	}

	s_files.clear();

	QTextStream readStream(&file);

	QString line;
	while (readStream.readLineInto(&line))
	{
		s_files.push_back(QtUtil::ToString(line));
	}
	readStream.seek(0);
	CryLog("CDeleteWorkFilesStorage::Load");

	file.close();

	std::sort(s_files.begin(), s_files.end());
	s_isDirty = false;
}

bool CDeletedWorkFilesStorage::Contains(const string& file) const
{
	using namespace Private_DeletedWorkFilesStorage;
	auto it = std::lower_bound(s_files.cbegin(), s_files.cend(), file);
	return it != s_files.cend() && *it == file;
}

int CDeletedWorkFilesStorage::GetSize() const
{
	using namespace Private_DeletedWorkFilesStorage;
	return s_files.size();
}

const string& CDeletedWorkFilesStorage::GetFile(int index) const
{
	using namespace Private_DeletedWorkFilesStorage;
	return s_files[index];
}
