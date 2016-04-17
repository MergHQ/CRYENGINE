// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "InspectorPanel.h"
#include "QtUtil.h"
#include "QAudioControlEditorIcons.h"
#include <ACETypes.h>
#include <IEditor.h>
#include "AudioControlsEditorPlugin.h"
#include <IAudioSystemItem.h>
#include "Controls/QMenuComboBox.h"

#include <QMessageBox>
#include <QMimeData>
#include <QDropEvent>
#include <QKeyEvent>
#include <QVariant>
#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QSpacerItem>
#include <QTabWidget>
#include <QVBoxLayout>

using namespace QtUtil;

namespace ACE
{
CInspectorPanel::CInspectorPanel(CATLControlsModel* pATLModel)
	: m_pATLModel(pATLModel)
	, m_selectedType(eACEControlType_NumTypes)
	, m_notFoundColor(QColor(255, 128, 128))
	, m_bAllControlsSameType(true)
{
	assert(m_pATLModel);
	resize(300, 490);
	setWindowTitle(tr("Inspector Panel"));
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	QVBoxLayout* pMainLayout = new QVBoxLayout(this);
	pMainLayout->setContentsMargins(0, 0, 0, 0);
	m_pEmptyInspectorLabel = new QLabel(this);
	m_pEmptyInspectorLabel->setAlignment(Qt::AlignCenter);

	pMainLayout->addWidget(m_pEmptyInspectorLabel);

	m_pPropertiesPanel = new QScrollArea(this);

	m_pPropertiesPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_pPropertiesPanel->setFrameShape(QFrame::NoFrame);
	m_pPropertiesPanel->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_pPropertiesPanel->setWidgetResizable(true);
	QWidget* pScrollAreaContent = new QWidget();
	QFormLayout* pControlsLayout = new QFormLayout(pScrollAreaContent);
	pControlsLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	pControlsLayout->setRowWrapPolicy(QFormLayout::WrapAllRows);
	pControlsLayout->setContentsMargins(0, 0, 0, 0);
	QLabel* pName = new QLabel(pScrollAreaContent);
	pName->setMinimumSize(QSize(55, 0));

	pControlsLayout->setWidget(1, QFormLayout::LabelRole, pName);
	m_pNameLineEditor = new QLineEdit(pScrollAreaContent);
	m_pNameLineEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	pControlsLayout->setWidget(1, QFormLayout::FieldRole, m_pNameLineEditor);
	m_pScopeLabel = new QLabel(pScrollAreaContent);
	m_pScopeLabel->setMinimumSize(QSize(55, 0));
	pControlsLayout->setWidget(2, QFormLayout::LabelRole, m_pScopeLabel);
	m_pScopeDropDown = new QMenuComboBox(pScrollAreaContent);
	m_pScopeDropDown->SetMultiSelect(false);
	m_pScopeDropDown->SetCanHaveEmptySelection(false);
	m_pScopeDropDown->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	pControlsLayout->setWidget(2, QFormLayout::FieldRole, m_pScopeDropDown);
	m_pConnectedControlsLabel = new QLabel(pScrollAreaContent);
	pControlsLayout->setWidget(3, QFormLayout::LabelRole, m_pConnectedControlsLabel);
	m_pConnectionList = new ACE::QConnectionsWidget(pScrollAreaContent);
	pControlsLayout->setWidget(3, QFormLayout::FieldRole, m_pConnectionList);
	m_pAutoLoadLabel = new QLabel(pScrollAreaContent);
	m_pAutoLoadLabel->setMinimumSize(QSize(55, 0));
	pControlsLayout->setWidget(4, QFormLayout::LabelRole, m_pAutoLoadLabel);
	m_pAutoLoadCheckBox = new QCheckBox(pScrollAreaContent);
	m_pAutoLoadCheckBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_pAutoLoadCheckBox->setTristate(true);
	pControlsLayout->setWidget(4, QFormLayout::FieldRole, m_pAutoLoadCheckBox);
	m_pConnectedSoundbanksLabel = new QLabel(pScrollAreaContent);
	pControlsLayout->setWidget(5, QFormLayout::LabelRole, m_pConnectedSoundbanksLabel);

	m_pPlatformGroupsTabWidget = new QTabWidget(pScrollAreaContent);
	m_pPlatformGroupsTabWidget->setTabPosition(QTabWidget::North);
	m_pPlatformGroupsTabWidget->setTabShape(QTabWidget::Rounded);
	m_pPlatformGroupsTabWidget->setUsesScrollButtons(false);
	m_pPlatformGroupsTabWidget->setDocumentMode(false);
	m_pPlatformGroupsTabWidget->setTabsClosable(false);
	m_pPlatformGroupsTabWidget->setCurrentIndex(-1);

	pControlsLayout->setWidget(5, QFormLayout::FieldRole, m_pPlatformGroupsTabWidget);

	m_pPlatformsLabel = new QLabel(pScrollAreaContent);

	pControlsLayout->setWidget(6, QFormLayout::LabelRole, m_pPlatformsLabel);

	m_pPlatformsWidget = new QWidget(pScrollAreaContent);
	m_pPlatformsWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	QHBoxLayout* pPlatformsLayout = new QHBoxLayout(m_pPlatformsWidget);
	pPlatformsLayout->setSpacing(9);
	pPlatformsLayout->setContentsMargins(0, 0, 0, 0);

	pControlsLayout->setWidget(6, QFormLayout::FieldRole, m_pPlatformsWidget);

	m_pPropertiesPanel->setWidget(pScrollAreaContent);

	pMainLayout->addWidget(m_pPropertiesPanel);
	pMainLayout->addItem(new QSpacerItem(300, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

	m_pEmptyInspectorLabel->setText(tr("Select a control to explore its properties"));
	pName->setText(tr("Name:"));
	m_pScopeLabel->setText(tr("Scope:"));
	// no Clear() call neccessary, since combo box is created empty
	m_pScopeDropDown->AddItem(tr("Global"));
	m_pConnectedControlsLabel->setText(tr("Connected Controls:"));
	m_pAutoLoadLabel->setText(tr("Auto Load:"));
	m_pAutoLoadCheckBox->setText(QString());
	m_pConnectedSoundbanksLabel->setText(tr("Preloaded Soundbanks:"));
	m_pPlatformsLabel->setText(tr("Platforms:"));

	connect(m_pNameLineEditor, SIGNAL(editingFinished()), this, SLOT(FinishedEditingName()));
	connect(m_pAutoLoadCheckBox, SIGNAL(clicked(bool)), this, SLOT(SetAutoLoadForCurrentControl(bool)));
	m_pScopeDropDown->signalCurrentIndexChanged.Connect(std::function<void(int)>([=](int index)
		{
			const QString itemText = m_pScopeDropDown->GetItem(index);
			SetControlScope(itemText);
	  }));

	// data validators
	m_pNameLineEditor->setValidator(new QRegExpValidator(QRegExp("^[a-zA-Z0-9_]*$"), m_pNameLineEditor));

	uint size = m_pATLModel->GetPlatformCount();
	for (uint i = 0; i < size; ++i)
	{
		QLabel* pLabel = new QLabel(m_pPlatformsWidget);
		pLabel->setText(QtUtil::ToQString(m_pATLModel->GetPlatformAt(i)));
		pLabel->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignVCenter);
		pLabel->setIndent(3);
		// TODO: these comboboxes show icons only, switch to QMenuComboBox when it supports icons
		QComboBox* pPlatformComboBox = new QComboBox(this);
		if (pPlatformComboBox)
		{
			m_platforms.push_back(pPlatformComboBox);
			QVBoxLayout* pLayout = new QVBoxLayout();
			pLayout->setSpacing(0);
			pLayout->setContentsMargins(-1, 0, 0, -1);
			pLayout->addWidget(pLabel);
			pLayout->addWidget(pPlatformComboBox);
			pPlatformsLayout->addLayout(pLayout);
			connect(pPlatformComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(SetControlPlatforms()));
		}
	}
	pPlatformsLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

	size = m_pATLModel->GetConnectionGroupCount();
	for (uint i = 0; i < size; ++i)
	{
		QConnectionsWidget* pConnectionWidget = new QConnectionsWidget(nullptr);
		pConnectionWidget->Init(m_pATLModel->GetConnectionGroupAt(i));
		pConnectionWidget->layout()->setContentsMargins(3, 3, 3, 3);
		pConnectionWidget->setMinimumSize(QSize(0, 300));
		pConnectionWidget->setMaximumSize(QSize(16777215, 300));
		m_connectionLists.push_back(pConnectionWidget);
		m_pPlatformGroupsTabWidget->addTab(pConnectionWidget, GetGroupIcon(i), QString());

		const size_t nPlatformCount = m_platforms.size();
		for (size_t j = 0; j < nPlatformCount; ++j)
		{
			m_platforms[j]->addItem(GetGroupIcon(i), QString());
		}
	}

	m_pATLModel->AddListener(this);
	m_pConnectionList->Init(g_sDefaultGroup);

	Reload();
}

CInspectorPanel::~CInspectorPanel()
{
	m_pATLModel->RemoveListener(this);
}

void CInspectorPanel::Reload()
{
	UpdateScopeData();
	UpdateInspector();
	UpdateConnectionData();
}

void CInspectorPanel::SetSelectedControls(const ControlList& selectedControls)
{
	m_pConnectionList->SetControl(nullptr);
	m_selectedType = eACEControlType_NumTypes;
	m_selectedControls.clear();
	m_bAllControlsSameType = true;
	const size_t size = selectedControls.size();
	for (size_t i = 0; i < size; ++i)
	{
		CATLControl* pControl = m_pATLModel->GetControlByID(selectedControls[i]);
		if (pControl)
		{
			m_selectedControls.push_back(pControl);
			if (m_selectedType == eACEControlType_NumTypes)
			{
				m_selectedType = pControl->GetType();
			}
			else if (m_bAllControlsSameType && (m_selectedType != pControl->GetType()))
			{
				m_bAllControlsSameType = false;
			}
		}
	}

	UpdateInspector();
	UpdateConnectionData();
}

void CInspectorPanel::UpdateConnectionListControl()
{
	if ((m_selectedControls.size() == 1) && m_bAllControlsSameType && m_selectedType != eACEControlType_Switch)
	{
		if (m_selectedType == eACEControlType_Preload)
		{
			m_pConnectedControlsLabel->setHidden(true);
			m_pConnectionList->setHidden(true);
			m_pConnectedSoundbanksLabel->setHidden(false);
			m_pPlatformGroupsTabWidget->setHidden(false);
			m_pPlatformsLabel->setHidden(false);
			m_pPlatformsWidget->setHidden(false);
		}
		else
		{
			m_pConnectedSoundbanksLabel->setHidden(true);
			m_pPlatformGroupsTabWidget->setHidden(true);
			m_pPlatformsLabel->setHidden(true);
			m_pPlatformsWidget->setHidden(true);
			m_pConnectedControlsLabel->setHidden(false);
			m_pConnectionList->setHidden(false);
		}
	}
	else
	{
		m_pConnectedControlsLabel->setHidden(true);
		m_pConnectionList->setHidden(true);
		m_pConnectedSoundbanksLabel->setHidden(true);
		m_pPlatformGroupsTabWidget->setHidden(true);
		m_pPlatformsLabel->setHidden(true);
		m_pPlatformsWidget->setHidden(true);
	}
}

void CInspectorPanel::UpdateScopeControl()
{
	const size_t size = m_selectedControls.size();
	if (m_bAllControlsSameType)
	{
		if (size == 1)
		{
			CATLControl* pControl = m_selectedControls[0];
			if (pControl)
			{
				if (m_selectedType == eACEControlType_State)
				{
					HideScope(true);
				}
				else
				{
					QString scope = QtUtil::ToQString(pControl->GetScope());
					if (scope.isEmpty())
					{
						m_pScopeDropDown->SetChecked(0);
					}
					else
					{
						m_pScopeDropDown->SetChecked(scope);
					}
					HideScope(false);
				}
			}
		}
		else
		{
			bool bSameScope = true;
			string sScope = "";
			for (int i = 0; i < size; ++i)
			{
				CATLControl* pControl = m_selectedControls[i];
				if (pControl)
				{
					string sControlScope = pControl->GetScope();
					if (sControlScope.empty())
					{
						sControlScope = "Global";
					}
					if (!sScope.empty() && sControlScope != sScope)
					{
						bSameScope = false;
						break;
					}
					sScope = sControlScope;
				}
			}
			if (bSameScope)
			{
				m_pScopeDropDown->SetChecked(QtUtil::ToQString(sScope));
			}
			else
			{
				m_pScopeDropDown->SetChecked(0);
			}
		}
	}
	else
	{
		HideScope(true);
	}
}

void CInspectorPanel::UpdateNameControl()
{
	const size_t size = m_selectedControls.size();
	if (m_bAllControlsSameType)
	{
		if (size == 1)
		{
			CATLControl* pControl = m_selectedControls[0];
			if (pControl)
			{
				m_pNameLineEditor->setText(ToQString(pControl->GetName()));
				m_pNameLineEditor->setEnabled(true);
			}
		}
		else
		{
			m_pNameLineEditor->setText(" <" + QString::number(size) + tr(" items selected>"));
			m_pNameLineEditor->setEnabled(false);
		}
	}
	else
	{
		m_pNameLineEditor->setText(" <" + QString::number(size) + tr(" items selected>"));
		m_pNameLineEditor->setEnabled(false);
	}
}

void CInspectorPanel::UpdateInspector()
{
	if (!m_selectedControls.empty())
	{
		m_pPropertiesPanel->setHidden(false);
		m_pEmptyInspectorLabel->setHidden(true);
		UpdateNameControl();
		UpdateScopeControl();
		UpdateAutoLoadControl();
		UpdateConnectionListControl();
	}
	else
	{
		m_pPropertiesPanel->setHidden(true);
		m_pEmptyInspectorLabel->setHidden(false);
	}
}

void CInspectorPanel::UpdateConnectionData()
{
	if ((m_selectedControls.size() == 1) && m_selectedType != eACEControlType_Switch)
	{
		if (m_selectedType == eACEControlType_Preload)
		{
			const size_t size = m_connectionLists.size();
			for (size_t i = 0; i < size; ++i)
			{
				m_connectionLists[i]->SetControl(m_selectedControls[0]);
			}

		}
		else
		{
			m_pConnectionList->SetControl(m_selectedControls[0]);
		}
	}
}

void CInspectorPanel::UpdateAutoLoadControl()
{
	if (!m_selectedControls.empty() && m_bAllControlsSameType && m_selectedType == eACEControlType_Preload)
	{
		HideAutoLoad(false);
		bool bHasAutoLoad = false;
		bool bHasNonAutoLoad = false;
		const size_t size = m_selectedControls.size();
		for (int i = 0; i < size; ++i)
		{
			CATLControl* pControl = m_selectedControls[i];
			if (pControl)
			{
				if (pControl->IsAutoLoad())
				{
					bHasAutoLoad = true;
				}
				else
				{
					bHasNonAutoLoad = true;
				}
			}
		}

		if (bHasAutoLoad && bHasNonAutoLoad)
		{
			m_pAutoLoadCheckBox->setTristate(true);
			m_pAutoLoadCheckBox->setCheckState(Qt::PartiallyChecked);

		}
		else
		{
			if (bHasAutoLoad)
			{
				m_pAutoLoadCheckBox->setChecked(true);
			}
			else
			{
				m_pAutoLoadCheckBox->setChecked(false);
			}
			m_pAutoLoadCheckBox->setTristate(false);
		}
	}
	else
	{
		HideAutoLoad(true);
	}
}

void CInspectorPanel::UpdateScopeData()
{
	m_pScopeDropDown->Clear();
	m_pScopeDropDown->AddItem(tr("Global"));
	for (int j = m_pATLModel->GetScopeCount() - 1; j >= 0; --j)
	{
		SControlScope scope = m_pATLModel->GetScopeAt(j);
		const int index = m_pScopeDropDown->GetItemCount();
		m_pScopeDropDown->AddItem(QString(scope.name));
		if (scope.bOnlyLocal)
		{
			// TODO: Implement when QMenuComboBox supports model based view. does not work in old version using QComboBox either
			// m_pScopeDropDown->setItemData(0, m_notFoundColor, Qt::ForegroundRole);
			m_pScopeDropDown->SetItemToolTip(index, tr("Level not found but it is referenced by some audio controls"));
		}
		else
		{
			m_pScopeDropDown->SetItemToolTip(index, "");
		}
	}
}

void CInspectorPanel::SetControlName(QString sName)
{
	if (m_selectedControls.size() == 1)
	{
		if (!sName.isEmpty())
		{
			CUndo undo("Audio Control Name Changed");
			string newName = QtUtil::ToString(sName);
			CATLControl* pControl = m_selectedControls[0];
			if (pControl && pControl->GetName() != newName)
			{
				if (!m_pATLModel->IsNameValid(newName, pControl->GetType(), pControl->GetScope(), pControl->GetParent()))
				{
					newName = m_pATLModel->GenerateUniqueName(newName, pControl->GetType(), pControl->GetScope(), pControl->GetParent());
				}
				pControl->SetName(newName);
			}
		}
		else
		{
			UpdateNameControl();
		}
	}
}

void CInspectorPanel::SetControlScope(QString scope)
{
	CUndo undo("Audio Control Scope Changed");
	size_t size = m_selectedControls.size();
	for (size_t i = 0; i < size; ++i)
	{
		CATLControl* pControl = m_selectedControls[i];
		if (pControl)
		{
			QString currentScope = QtUtil::ToQString(pControl->GetScope());
			if (currentScope != scope && (scope != tr("Global") || currentScope != ""))
			{
				if (scope == tr("Global"))
				{
					pControl->SetScope("");
				}
				else
				{
					pControl->SetScope(QtUtil::ToString(scope));
				}
			}
		}
	}
}

void CInspectorPanel::SetAutoLoadForCurrentControl(bool bAutoLoad)
{
	CUndo undo("Audio Control Auto-Load Property Changed");
	size_t size = m_selectedControls.size();
	for (size_t i = 0; i < size; ++i)
	{
		CATLControl* pControl = m_selectedControls[i];
		if (pControl)
		{
			pControl->SetAutoLoad(bAutoLoad);
		}
	}
}

void CInspectorPanel::SetControlPlatforms()
{
	if (m_selectedControls.size() == 1)
	{
		CATLControl* pControl = m_selectedControls[0];
		if (pControl)
		{
			const size_t size = m_platforms.size();
			for (size_t i = 0; i < size; ++i)
			{
				pControl->SetGroupForPlatform(m_pATLModel->GetPlatformAt(i), m_platforms[i]->currentIndex());
			}
		}
	}
}

void CInspectorPanel::FinishedEditingName()
{
	SetControlName(m_pNameLineEditor->text());
}

void CInspectorPanel::OnControlModified(ACE::CATLControl* pControl)
{
	size_t size = m_selectedControls.size();
	for (size_t i = 0; i < size; ++i)
	{
		if (m_selectedControls[i] == pControl)
		{
			UpdateInspector();
			UpdateConnectionData();
			break;
		}
	}
}

void CInspectorPanel::HideScope(bool bHide)
{
	m_pScopeLabel->setHidden(bHide);
	m_pScopeDropDown->setHidden(bHide);
}

void CInspectorPanel::HideAutoLoad(bool bHide)
{
	m_pAutoLoadLabel->setHidden(bHide);
	m_pAutoLoadCheckBox->setHidden(bHide);
}

void CInspectorPanel::HideGroupConnections(bool bHide)
{
	m_pPlatformGroupsTabWidget->setHidden(bHide);
}

}
