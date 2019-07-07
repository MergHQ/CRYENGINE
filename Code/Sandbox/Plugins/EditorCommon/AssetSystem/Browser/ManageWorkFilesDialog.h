// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "QWidget"
#include "EditorCommonAPI.h"
#include "Controls/EditorDialog.h"

class CAsset;
class QLabel;
class QPushButton;
class QTreeView;

class EDITOR_COMMON_API CManageWorkFilesDialog : public CEditorDialog
{
	Q_OBJECT
public:
	static void ShowWindow(CAsset* pAsset);

	CManageWorkFilesDialog(CAsset* pAsset, QWidget* pParent = nullptr);

private:
	void OnSave();
	void OnAddWorkFile();

	void CheckAssetsStatus();

	CAsset*      m_pAsset{ nullptr };
	QTreeView*   m_pTree{ nullptr };
	QPushButton* m_pSaveButton{ nullptr };
	QWidget*     m_pWarningWidget{ nullptr };
	QLabel*      m_pWarningText{ nullptr };
};
