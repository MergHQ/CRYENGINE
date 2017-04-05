// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QString>
#include <QTemporaryDir>

#include <functional>

namespace FbxMetaData
{

struct SMetaData;

} // namespace FbMetaData

typedef std::function<void(bool bSuccess, string filePath)> FinalizeSave;

//! Compiles file described by \p metaData and saves output as \p targetFilePath.
//! Writes intermediate files to a temporary directory \pTempDir to avoid torn writes.
//! Files will be overwritten unconditionally.
//! Copies asset meta-data (.cryasset) too, if it exists.
//!
//! \param pTempDir Temporary directory.
//! \param metaData Scene meta-data.
//! \param targetFilePath Absolute file path of target data file.
//! \param finalize Completion callback.
void SaveRcObject(
	const std::shared_ptr<QTemporaryDir>& pTempDir,
	const FbxMetaData::SMetaData& metaData,
	const string& targetFilePath,
	const FinalizeSave& finalize = FinalizeSave());