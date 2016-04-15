// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QFrame>
#include <QMenu>
#include <QListWidget>

#include "ATLControlsModel.h"
#include "AudioControl.h"
#include <IAudioSystemEditor.h>
#include "QConnectionsWidget.h"

class QMenuComboBox;

class QVBoxLayout;
class QLabel;
class QScrollArea;
class QLineEdit;
class QCheckBox;
class QTabWidget;
class QComboBox;

namespace ACE
{
class CATLControl;

class CInspectorPanel : public QFrame, public IATLControlModelListener
{
	Q_OBJECT
public:
	explicit CInspectorPanel(CATLControlsModel* pATLModel);
	~CInspectorPanel();
	void Reload();

public slots:
	void SetSelectedControls(const ControlList& selectedControls);
	void UpdateInspector();

private slots:
	void SetControlName(QString name);
	void SetControlScope(QString scope);
	void SetAutoLoadForCurrentControl(bool bAutoLoad);
	void SetControlPlatforms();
	void FinishedEditingName();

private:
	void UpdateNameControl();
	void UpdateScopeControl();
	void UpdateAutoLoadControl();
	void UpdateConnectionListControl();
	void UpdateConnectionData();
	void UpdateScopeData();

	void HideScope(bool bHide);
	void HideAutoLoad(bool bHide);
	void HideGroupConnections(bool bHide);

	//////////////////////////////////////////////////////////
	// IAudioSystemEditor implementation
	/////////////////////////////////////////////////////////
	virtual void OnControlModified(ACE::CATLControl* pControl) override;
	//////////////////////////////////////////////////////////

	CATLControlsModel*               m_pATLModel;

	EACEControlType                  m_selectedType;
	std::vector<CATLControl*>        m_selectedControls;
	bool                             m_bAllControlsSameType;

	QColor                           m_notFoundColor;

	std::vector<QConnectionsWidget*> m_connectionLists;
	std::vector<QComboBox*>          m_platforms;

	QLabel*                          m_pEmptyInspectorLabel;
	QScrollArea*                     m_pPropertiesPanel;
	QLineEdit*                       m_pNameLineEditor;
	QLabel*                          m_pScopeLabel;
	QMenuComboBox*                   m_pScopeDropDown;
	QLabel*                          m_pConnectedControlsLabel;
	ACE::QConnectionsWidget*         m_pConnectionList;
	QLabel*                          m_pAutoLoadLabel;
	QCheckBox*                       m_pAutoLoadCheckBox;
	QLabel*                          m_pConnectedSoundbanksLabel;
	QTabWidget*                      m_pPlatformGroupsTabWidget;
	QLabel*                          m_pPlatformsLabel;
	QWidget*                         m_pPlatformsWidget;
};
}
