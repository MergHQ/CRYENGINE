// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include <QWidget>

//! This tab displays version control system's settings.
class EDITOR_COMMON_API CVersionControlSettingsTab : public QWidget
{
	Q_OBJECT
public:

	CVersionControlSettingsTab(QWidget* pParent = nullptr);

};