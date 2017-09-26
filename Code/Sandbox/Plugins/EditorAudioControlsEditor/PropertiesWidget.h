// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QWidget>

class QPropertyTree;
class QLabel;
class QString;

namespace ACE
{
class CConnectionsWidget;
class CAudioAssetsManager;
class IAudioSystemItem;
class CAudioControl;

class CPropertiesWidget final : public QWidget
{
	Q_OBJECT

public:

	explicit CPropertiesWidget(CAudioAssetsManager* pAssetsManager);
	virtual ~CPropertiesWidget() override;

	void Reload();
	void BackupTreeViewStates();
	void RestoreTreeViewStates();

public slots:

	void SetSelectedControls(std::vector<CAudioControl*> const& selectedControls);

private:

	CAudioAssetsManager*     m_pAssetsManager;
	CConnectionsWidget*      m_pConnectionsWidget;
	QPropertyTree*           m_pPropertyTree;
	QLabel*                  m_pConnectionsLabel;
	std::unique_ptr<QString> m_pUsageHint;
	bool                     m_bSupressUpdates = false;
};
} // namespace ACE
