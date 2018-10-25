// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include <QVariant>

class EDITOR_COMMON_API CUserData
{
public:
	CUserData(std::vector<string> userDataPaths);
	virtual ~CUserData();
};

namespace UserDataUtil
{
// Get path in user folder for given relative path
QString EDITOR_COMMON_API GetUserPath(const char* relativeFilePath);

// Load user data. Path should be relative to user data folder
QVariant EDITOR_COMMON_API Load(const char* relativeFilePath);

// Save user data. Path should be relative to user data folder
void EDITOR_COMMON_API Save(const char* relativeFilePath, const char* data);
} // namespace UserDataUtil
