// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QFrame>

class QPropertyTree;
class QLabel;

namespace ACE
{
class QConnectionsWidget;
class CAudioAssetsManager;
class IAudioSystemItem;

class CInspectorPanel : public QFrame
{
	Q_OBJECT
public:
	explicit CInspectorPanel(CAudioAssetsManager* pAssetsManager);
	~CInspectorPanel();
	void Reload();

public slots:
	void SetSelectedControls(const std::vector<CAudioControl*>& selectedControls);

private:
	void UpdateConnectionData();

	CAudioAssetsManager* m_pAssetsManager;
	QConnectionsWidget*  m_pConnectionList;
	QPropertyTree*       m_pPropertyTree;
	QLabel*              m_pConnectionsLabel;
	bool                 m_bSupressUpdates = false;
};
}
