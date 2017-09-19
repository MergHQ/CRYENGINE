// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QFrame>

class QPropertyTree;
class QLabel;
class QString;

namespace ACE
{
class CConnectionsWidget;
class CAudioAssetsManager;
class IAudioSystemItem;
class CAudioControl;

class CPropertiesWidget final : public QFrame
{
	Q_OBJECT
public:

	explicit CPropertiesWidget(CAudioAssetsManager* pAssetsManager);
	virtual ~CPropertiesWidget() override;

	void Reload();
	void BackupTreeViewStates();
	void RestoreTreeViewStates();

public slots:

	void SetSelectedControls(const std::vector<CAudioControl*>& selectedControls);

private:

	CAudioAssetsManager*     m_pAssetsManager;
	CConnectionsWidget*      m_pConnectionList;
	QPropertyTree*           m_pPropertyTree;
	QLabel*                  m_pConnectionsLabel;
	std::unique_ptr<QString> m_pUsageHint;
	bool                     m_bSupressUpdates = false;
};
} // namespace ACE
