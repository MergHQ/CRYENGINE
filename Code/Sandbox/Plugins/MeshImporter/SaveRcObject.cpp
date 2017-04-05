// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SaveRcObject.h"
#include "CallRcTask.h"
#include "RcCaller.h"
#include "DialogCommon.h"
#include "ImporterUtil.h"

#include <FilePathUtil.h>
#include <ThreadingUtils.h>

#include <QDir>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QTemporaryDir>

void LogPrintf(const char* szFormat, ...);

void SaveRcObject(
	const std::shared_ptr<QTemporaryDir>& pTempDir,
	const FbxMetaData::SMetaData& metaData,
	const string& targetFilePath,
	const FinalizeSave& finalize)
{
	const string filename = PathUtil::GetFileName(targetFilePath);

	const string sourceFilePath = metaData.sourceFilename;

	FbxMetaData::SMetaData metaDataCopy(metaData);
	metaDataCopy.sourceFilename = PathUtil::GetFile(metaDataCopy.sourceFilename);
	
	auto pTempFile = WriteTemporaryFile(pTempDir->path(), metaDataCopy.ToJson(), QtUtil::ToQString(filename));
	if (!pTempFile)
	{
		finalize(false, targetFilePath);
		return;
	}

	const string filePath = QtUtil::ToString(pTempFile->fileName());

	ThreadingUtils::Async([filePath, finalize, targetFilePath, sourceFilePath, pTempDir, pTempFile = std::move(pTempFile), metaDataCopy]()
	{
		CRcCaller rcCaller;
		const string options = string().Format("%s /overwritesourcefile=\"%s\"",
			CRcCaller::OptionOverwriteExtension("fbx"),
			sourceFilePath.c_str());
		rcCaller.SetAdditionalOptions(options);
		const bool bSuccess = rcCaller.Call(filePath);
		if (!bSuccess)
		{
			LogPrintf("%s: Compiling file %s failed.\n", __FUNCTION__, filePath.c_str());
		}
		else
		{
			ThreadingUtils::PostOnMainThread([filePath, targetFilePath, metaDataCopy, pTempDir /* keep temp dir alive */]()
			{
				const string cgfFile = PathUtil::ReplaceExtension(filePath, metaDataCopy.outputFileExt.c_str());
				MeshImporter::RenameAllowOverwrite(cgfFile, targetFilePath);
				MeshImporter::RenameAllowOverwrite(cgfFile + ".cryasset", targetFilePath + ".cryasset");
			});
		}
		if (finalize)
		{
			ThreadingUtils::PostOnMainThread(finalize, bSuccess, targetFilePath);
		}
	});
}
