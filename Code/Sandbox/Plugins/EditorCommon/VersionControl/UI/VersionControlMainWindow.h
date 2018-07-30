// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "QTabWidget"

class CVersionControlPendingChangesTab;
class CVersionControlHistoryTab;
class CVersionControlSettingsTab;

class EDITOR_COMMON_API CVersionControlMainWindow : public CDockableEditor
{
	Q_OBJECT
public:
	CVersionControlMainWindow(QWidget* pParent = nullptr);

	void SelectAssets(std::vector<CAsset*> assets);

	virtual const char* GetEditorName() const override { return "Version Control System"; }

private:
	CVersionControlPendingChangesTab* m_pChangelistsTab{ nullptr };
	CVersionControlHistoryTab*        m_pHistoryTab{ nullptr };
	CVersionControlSettingsTab*       m_pSettingsTab{ nullptr };
};