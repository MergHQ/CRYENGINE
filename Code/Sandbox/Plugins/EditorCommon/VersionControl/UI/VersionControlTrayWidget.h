// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "QToolButton"

class EDITOR_COMMON_API CVersionControlTrayWidget : public QToolButton
{
	Q_OBJECT
public:
	explicit CVersionControlTrayWidget(QWidget* pParent = nullptr);

protected:
	void OnClicked(bool bChecked);
};