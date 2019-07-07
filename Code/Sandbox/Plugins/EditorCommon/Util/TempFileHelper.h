// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EditorCommonAPI.h"

//
// A helper for creating a temp file to write to, then copying that over the destination
// file only if it changes (to avoid requiring the user to check out source controlled
// file unnecessarily)
//
class EDITOR_COMMON_API CTempFileHelper
{
public:
	explicit CTempFileHelper(const char* szFileName);
	~CTempFileHelper();

	// Gets the path to the temp file that should be written to
	const string& GetTempFilePath() { return m_tempFileName; }

	// After the temp file has been written and closed, this should be called to update
	// the destination file.
	// If backup is true a backup file will be created if the file has changed.
	bool UpdateFile(bool backup);

private:
	string m_fileName;
	string m_tempFileName;
};
