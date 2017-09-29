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

	CAudioAssetsManager* const m_pAssetsManager;
	CConnectionsWidget* const  m_pConnectionsWidget;
	QPropertyTree* const       m_pPropertyTree;
	QLabel*                    m_pConnectionsLabel;
	std::unique_ptr<QString>   m_pUsageHint;
	bool                       m_supressUpdates = false;
};
} // namespace ACE
