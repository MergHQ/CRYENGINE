// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <memory>

class CAsset;

namespace FbxMetaData
{
struct SMetaData;
} // namespace FbxMetaData

class QTemporaryDir;
class QTemporaryFile;

// ==================================================
// Path utilities.
// ==================================================

bool IsPathAbsolute(const string& path);

//! Returns true if \p filePath is contained in the asset directory.
bool IsAssetPath(const string& filePath);

QString AppendPath(const QString& lhp, const QString& rhp);

QString GetAbsoluteGameFolderPath();

// ==================================================
// File utilities.
// ==================================================

bool WriteToFile(const QString& filePath, const QString& content);
bool FileExists(const string& path);

// ==================================================
// Temporary directories and files utilities.
// ==================================================

std::unique_ptr<QTemporaryFile> WriteTemporaryFile(const QString& dirPath, const string& content, QString templateName);

std::unique_ptr<QTemporaryDir> CreateTemporaryDirectory();

// ==================================================
// Assets.
// ==================================================

bool IsAssetMetaDataFile(string filePath);

//! Returns any file of asset that matches one of the file extensions in \p exts.
string GetFileFromAsset(CAsset& asset, const std::vector<string>& exts);

// ==================================================
// File importing.
// ==================================================

//! Copies a file to the asset folder.
class CFileImporter
{
public:
	typedef std::function<bool(const string&)> MayOverwriteFunc;

public:
	bool Import(const string& inputFilePath, const string& outputFilePath);
	bool Import(const string& inputFilePath);

	void SetMayOverwriteFunc(const MayOverwriteFunc& mayOverwrite);

	string GetOutputFilePath() const; //!< Returns path that is relative to asset directory.

	string GetError() const;

private:
	static string ShowDialog(const string& inputFilename);

private:
	MayOverwriteFunc m_mayOverwrite;
	string m_outputFilePath;
	string m_error;
};