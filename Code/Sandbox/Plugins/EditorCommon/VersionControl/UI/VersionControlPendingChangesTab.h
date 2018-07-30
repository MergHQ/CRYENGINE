// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "QWidget"

class QPlainTextEdit;
class QStackedWidget;
class QTreeView;

class EDITOR_COMMON_API CVersionControlPendingChangesTab : public QWidget
{
	Q_OBJECT
public:
	explicit CVersionControlPendingChangesTab(QWidget* pParent = nullptr);

	~CVersionControlPendingChangesTab();

	void SelectAssets(std::vector<CAsset*> assets);

private:
	void OnSubmit();
	void OnConfirmSubmit();
	void OnCancelSubmit();

	QTreeView*      m_pTree{ nullptr };
	QPlainTextEdit* m_textEdit{ nullptr };
	QStackedWidget* m_pStackedWidget{ nullptr };
};