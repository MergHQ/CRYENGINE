// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include <QToolButton>

class QPopupWidget;

//! This is a widget (icon) that is displayed in the top-right corner of the editor.
//! Clicking it brings up main version control system window.
class EDITOR_COMMON_API CVersionControlTrayWidget : public QToolButton
{
	Q_OBJECT
public:
	explicit CVersionControlTrayWidget(QWidget* pParent = nullptr);

	~CVersionControlTrayWidget();

private:
	void OnClicked(bool bChecked);

	QPopupWidget* m_pPopUpMenu{ nullptr };
};