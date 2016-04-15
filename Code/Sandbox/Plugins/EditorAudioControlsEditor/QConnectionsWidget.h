// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "AudioControl.h"

#include <QWidget>
#include <QListWidget>
#include <QColor>

class QVBoxLayout;
class QFrame;
class QPropertyTree;
class QTreeView;

namespace ACE
{
class CATLControl;
class QConnectionModel;

class QConnectionsWidget : public QWidget
{
public:
	QConnectionsWidget(QWidget* pParent = nullptr, const string& group = "");
	void SetControl(CATLControl* pControl);
	void Init(const string& group);

private:
	bool eventFilter(QObject* pObject, QEvent* pEvent) override;
	void RemoveSelectedConnection();

	string            m_group;
	CATLControl*      m_pControl;
	QFrame*           m_pConnectionPropertiesFrame;
	QPropertyTree*    m_pConnectionProperties;
	QConnectionModel* m_pConnectionModel;
	QTreeView*        m_pConnectionsView;
};
}
