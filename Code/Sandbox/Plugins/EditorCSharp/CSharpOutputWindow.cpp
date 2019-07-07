// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include <StdAfx.h>
#include <cctype>
#include <CrySystem/ICryPluginManager.h>
#include <CrySystem/IProjectManager.h>
#include "CSharpOutputWindow.h"
#include "CSharpEditorPlugin.h"
#include "CSharpOutputModel.h"
#include "EditorStyleHelper.h"

#include "QtViewPane.h"

#include "QAdvancedTreeView.h"

#include <QWidget>
#include <QVariant>
#include <QHeaderView>
#include <QFilteringPanel.h>
#include <QAbstractItemModel>
#include <QMenuBar>

#include "QAdvancedItemDelegate.h"

REGISTER_VIEWPANE_FACTORY_AND_MENU(CCSharpOutputWindow, "C# Output", "Tools", false, "Advanced");

CCSharpOutputWindow::CCSharpOutputWindow(QWidget* const pParent)
	: CDockableEditor(pParent)
	, m_pModel(new CCSharpOutputModel())
{
	m_pFilter = new QAttributeFilterProxyModel(QAttributeFilterProxyModel::BaseBehavior);
	m_pFilter->setSourceModel(m_pModel);
	m_pFilter->setFilterKeyColumn(2);

	m_pDetailsView = new QAdvancedTreeView(QAdvancedTreeView::UseItemModelAttribute);
	m_pDetailsView->setModel(m_pFilter);
	m_pDetailsView->setSortingEnabled(true);
	m_pDetailsView->setUniformRowHeights(true);
	m_pDetailsView->header()->setStretchLastSection(true);
	m_pDetailsView->header()->setMinimumSectionSize(20);

	// Setting Description and FileName to resize to contents
	m_pDetailsView->header()->resizeSection(2, 400);
	m_pDetailsView->header()->resizeSection(3, 150);

	m_pDetailsView->sortByColumn(CCSharpOutputModel::eColumn_ErrorText, Qt::AscendingOrder);
	m_pDetailsView->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);

	connect(m_pDetailsView, &QAdvancedTreeView::doubleClicked, this, &CCSharpOutputWindow::OnDoubleClick);

	m_pFilteringPanel = new QFilteringPanel("CSharpOutputWindow", m_pFilter);
	m_pFilteringPanel->SetContent(m_pDetailsView);
	m_pFilteringPanel->GetSearchBox()->EnableContinuousSearch(true);

	QVBoxLayout* pLayout = new QVBoxLayout();
	pLayout->setContentsMargins(0, 0, 0, 0);
	pLayout->addWidget(m_pFilteringPanel);
	setLayout(pLayout);
	SetContent(pLayout);

	CCSharpEditorPlugin* pPlugin = CCSharpEditorPlugin::GetInstance();
	assert(pPlugin);
	if (pPlugin != nullptr)
	{
		pPlugin->RegisterMessageListener(this);
		OnMessagesUpdated();
	}
}

CCSharpOutputWindow::~CCSharpOutputWindow()
{
	CCSharpEditorPlugin* pPlugin = CCSharpEditorPlugin::GetInstance();
	if (pPlugin != nullptr)
	{
		pPlugin->UnregisterMessageListener(this);
	}
	delete m_pFilter;
	delete m_pModel;
	disconnect(m_pDetailsView, &QAdvancedTreeView::doubleClicked, this, &CCSharpOutputWindow::OnDoubleClick);
}

QAdvancedTreeView* CCSharpOutputWindow::GetView() const
{
	return m_pDetailsView;
}

void CCSharpOutputWindow::OnDoubleClick(const QModelIndex& index)
{
	CCSharpEditorPlugin* pPlugin = CCSharpEditorPlugin::GetInstance();
	assert(pPlugin);
	if (pPlugin == nullptr)
	{
		return;
	}
	const SCSharpCompilerError* pError = gEnv->pMonoRuntime->GetCompileErrorAt(index.row());
	if (pError != nullptr)
	{
		const Cry::IProjectManager* pProjectManager = gEnv->pSystem->GetIProjectManager();
		if (pProjectManager != nullptr)
		{
			pPlugin->OpenCSharpFile(pError->m_fileName, pError->m_line);
		}
	}
}

void CCSharpOutputWindow::OnMessagesUpdated()
{
	m_pModel->ResetModel();
}

QVariantMap CCSharpOutputWindow::GetLayout() const
{
	QVariantMap& state = CDockableEditor::GetLayout();
	state.insert("view", m_pDetailsView->GetState());
	return state;
}

void CCSharpOutputWindow::SetLayout(const QVariantMap& state)
{
	CDockableEditor::SetLayout(state);

	QVariant viewState = state.value("view");
	if (viewState.isValid() && viewState.type() == QVariant::Map)
	{
		m_pDetailsView->SetState(viewState.value<QVariantMap>());
	}
}
