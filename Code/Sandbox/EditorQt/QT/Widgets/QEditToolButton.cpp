// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "QEditToolButton.h"
#include "IEditorImpl.h"
#include "LogFile.h"

#include <CryIcon.h>

#include <IEditorClassFactory.h>
#include <LevelEditor/LevelEditorSharedState.h>
#include <RecursionLoopGuard.h>

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>


QEditToolButton::QEditToolButton(QWidget* parent)
	: QToolButton(parent)
	, m_toolClass(nullptr)
	, m_bNeedDocument(false)
	, m_ignoreClick(false)
{
	setCheckable(true);
	connect(this, SIGNAL(toggled(bool)), this, SLOT(OnClicked(bool)));
	GetIEditorImpl()->GetLevelEditorSharedState()->signalEditToolChanged.Connect(this, &QEditToolButton::DetermineCheckedState);
}

QEditToolButton::~QEditToolButton()
{
	GetIEditorImpl()->GetLevelEditorSharedState()->signalEditToolChanged.DisconnectObject(this);
}

void QEditToolButton::SetToolName(const string& sEditToolName)
{
	IClassDesc* pClass = GetIEditorImpl()->GetClassFactory()->FindClass(sEditToolName);
	if (!pClass)
	{
		Warning("Editor Tool %s not registered.", (const char*)sEditToolName);
		return;
	}
	if (pClass->SystemClassID() != ESYSTEM_CLASS_EDITTOOL)
	{
		Warning("Class name %s is not a valid Edit Tool class.", (const char*)sEditToolName);
		return;
	}
	CRuntimeClass* pRtClass = pClass->GetRuntimeClass();
	if (!pRtClass || !pRtClass->IsDerivedFrom(RUNTIME_CLASS(CEditTool)))
	{
		Warning("Class name %s is not a valid Edit Tool class.", (const char*)sEditToolName);
		return;
	}
	m_toolClass = pRtClass;
}

void QEditToolButton::SetToolClass(CRuntimeClass* toolClass)
{
	m_toolClass = toolClass;

	// Now we can determine if we can be checked or not, since we have a valid tool class
	DetermineCheckedState();
}


void QEditToolButton::DetermineCheckedState()
{
	RECURSION_GUARD(m_ignoreClick);

	if (!m_toolClass)
	{
		if (isChecked())
			setChecked(false);
		return;
	}

	// Check tool state.
	CEditTool* tool = GetIEditorImpl()->GetLevelEditorSharedState()->GetEditTool();
	CRuntimeClass* toolClass = 0;
	if (tool)
		toolClass = tool->GetRuntimeClass();

	if (toolClass != m_toolClass)
	{
		if (isChecked())
			setChecked(false);
	}
	else
	{
		if (!isChecked())
			setChecked(true);
	}
}

void QEditToolButton::OnClicked(bool bChecked)
{
	RECURSION_GUARD(m_ignoreClick);

	if (!m_toolClass || m_bNeedDocument && !GetIEditorImpl()->IsDocumentReady())
	{
		if (isChecked())
			setChecked(false);
		return;
	}

	CEditTool* tool = GetIEditorImpl()->GetLevelEditorSharedState()->GetEditTool();
	if (!bChecked)
	{
		if (tool && tool->GetRuntimeClass() == m_toolClass)
		{
			GetIEditorImpl()->GetLevelEditorSharedState()->SetEditTool(nullptr);
		}
		return;
	}
	else
	{
		CEditTool* pNewTool = (CEditTool*)m_toolClass->CreateObject();
		if (!pNewTool)
			return;

		// Must be last function, can delete this.
		GetIEditorImpl()->GetLevelEditorSharedState()->SetEditTool(pNewTool);
	}
}

//////////////////////////////////////////////////////////////////////////
QEditToolButtonPanel::QEditToolButtonPanel(LayoutType layoutType, QWidget* pParent)
	: QWidget(pParent)
	, m_layoutType(layoutType)
{
	QLayout* pLayout = nullptr;
	switch (m_layoutType)
	{
	case LayoutType::Grid:
		pLayout = new QGridLayout(this);
		break;

	case LayoutType::Vertical:
		pLayout = new QVBoxLayout(this);
		break;

	case LayoutType::Horizontal:
		pLayout = new QHBoxLayout(this);
		break;
	}

	pLayout->setSpacing(3);
	pLayout->setContentsMargins(3, 4, 4, 6);
	pLayout->setSizeConstraint(QLayout::SetNoConstraint);
}

QEditToolButtonPanel::~QEditToolButtonPanel()
{
	ClearButtons();
}

void QEditToolButtonPanel::AddButton(const SButtonInfo& button)
{
	SButton b;
	b.info = button;
	m_buttons.push_back(b);

	b.pButton = new QEditToolButton(this);
	b.pButton->setText(button.name.c_str());
	b.pButton->setToolTip(button.toolTip.c_str());
	b.pButton->setIcon(CryIcon(button.icon.c_str()));
	b.pButton->setIconSize(QSize(24, 24));
	b.pButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	b.pButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
	b.pButton->SetNeedDocument(b.info.bNeedDocument);

	if (m_layoutType == LayoutType::Grid)
	{
		int index = m_buttons.size() - 1;
		QGridLayout* grid = (QGridLayout*)layout();
		int column = index % 2;
		int row = index / 2;
		grid->addWidget(b.pButton, row, column);
	}
	else
	{
		layout()->addWidget(b.pButton);
	}

	if (b.info.pToolClass)
	{
		b.pButton->SetToolClass(b.info.pToolClass);
	}
	else if (!b.info.toolClassName.empty())
	{
		b.pButton->SetToolName(b.info.toolClassName);
	}
}

void QEditToolButtonPanel::AddButton(string name, string toolClass)
{
	SButtonInfo bi;
	bi.name = name;
	bi.toolClassName = toolClass;
	AddButton(bi);
}

void QEditToolButtonPanel::AddButton(string name, CRuntimeClass* pToolClass)
{
	SButtonInfo bi;
	bi.name = name;
	bi.pToolClass = pToolClass;
	AddButton(bi);
}

void QEditToolButtonPanel::ClearButtons()
{
	for (int i = 0; i < m_buttons.size(); i++)
	{
		delete m_buttons[i].pButton;
		m_buttons[i].pButton = 0;
	}
	m_buttons.clear();
}

void QEditToolButtonPanel::UncheckAll()
{
	for (int i = 0; i < m_buttons.size(); i++)
	{
		m_buttons[i].pButton->setChecked(false);
	}
}

void QEditToolButtonPanel::EnableButton(string buttonName, bool enable)
{
	for (int i = 0; i < m_buttons.size(); i++)
	{
		SButton& b = m_buttons[i];

		if (strcmp(b.info.name, buttonName) == 0 && b.pButton)
			b.pButton->setEnabled(enable);
	}
}
