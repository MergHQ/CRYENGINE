// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Dialogs/PythonScriptsPanel.h"
#include "IEditorImpl.h"

#include <QSearchBox.h>

#include <FileSystem/FileSystem_Enumerator.h>
#include <FileSystem/FileTreeModel.h>
#include <FileSystem/FileSortProxyModel.h>
#include <Util/EditorUtils.h>

#include <QBoxLayout>
#include <QDir>
#include <QHeaderView>
#include <QPushButton>
#include <QAdvancedTreeView.h>

REGISTER_VIEWPANE_FACTORY_AND_MENU(CPythonScriptsPanel, "Python Scripts", "Editor", false, "Advanced");

CPythonScriptsPanel::CPythonScriptsPanel()
{
	const auto pLayout = new QVBoxLayout;
	pLayout->setContentsMargins(5, 5, 5, 5);
	SetContent(pLayout);

	auto filesystemEnumerator = GetIEditorImpl()->GetFileSystemEnumerator();

	FileSystem::SFileFilter fileFilter;
	fileFilter.skipEmptyDirectories = true;
	fileFilter.fileExtensions << QStringLiteral("py");

	auto fileTreeModel = new CFileTreeModel(*filesystemEnumerator, fileFilter, this);

	auto sortModel = new CFileSortProxyModel(this);
	sortModel->setSourceModel(fileTreeModel);

	QWidget* pSearchBoxContainer = new QWidget();
	pSearchBoxContainer->setObjectName("SearchBoxContainer");

	QHBoxLayout* pSearchBoxLayout = new QHBoxLayout();
	pSearchBoxLayout->setAlignment(Qt::AlignTop);
	pSearchBoxLayout->setMargin(0);
	pSearchBoxLayout->setSpacing(0);

	const auto pSearchBox = new QSearchBox();
	auto searchFunction = [=](const QString& text)
	{
		auto newFilter = fileFilter;
		if (!text.isEmpty())
		{
			newFilter.fileFilterCallbacks <<[text](const FileSystem::SnapshotPtr&, const FileSystem::FilePtr& file)
				  {
				  return file->keyName.contains(text, Qt::CaseInsensitive);
			  };
		}
		fileTreeModel->SetFilter(newFilter);
	};
	pSearchBox->SetSearchFunction(std::function<void(const QString&)>(searchFunction));

	pSearchBox->EnableContinuousSearch(true);

	pSearchBoxLayout->addWidget(pSearchBox);
	pSearchBoxContainer->setLayout(pSearchBoxLayout);

	pLayout->addWidget(pSearchBoxContainer);

	m_pTree = new QAdvancedTreeView();
	m_pTree->setModel(sortModel);
	m_pTree->setSelectionMode(QTreeView::ExtendedSelection);
	m_pTree->setUniformRowHeights(true);
	m_pTree->setAllColumnsShowFocus(true);

	m_pTree->setHeaderHidden(true);
	m_pTree->SetColumnVisible(CFileTreeModel::eColumn_LastModified, false);
	m_pTree->SetColumnVisible(CFileTreeModel::eColumn_Type, false);
	m_pTree->SetColumnVisible(CFileTreeModel::eColumn_Size, false);
	m_pTree->SetColumnVisible(CFileTreeModel::eColumn_Archive, false);

	pLayout->addWidget(m_pTree);

	pSearchBox->SetAutoExpandOnSearch(m_pTree);

	const auto pExecuteButton = new QPushButton();
	pExecuteButton->setText(tr("Execute"));
	connect(pExecuteButton, &QPushButton::pressed, this, &CPythonScriptsPanel::ExecuteScripts);
	pLayout->addWidget(pExecuteButton);

	// Hide all columns but the first (filename)
	const auto pHeader = m_pTree->header();
	if (pHeader)
	{
		pHeader->setSectionResizeMode(0, QHeaderView::Stretch);
		const size_t sections = pHeader->count();
		for (size_t i = 1; i < sections; ++i)
		{
			pHeader->hideSection(i);
		}
	}
}

CPythonScriptsPanel::~CPythonScriptsPanel()
{
}

void CPythonScriptsPanel::ExecuteScripts() const
{
	const auto pSelectionModel = m_pTree->selectionModel();
	if (pSelectionModel)
	{
		const auto selection = pSelectionModel->selectedRows();
		for (const auto& index : selection)
		{
			QString scriptPath = index.data(CFileTreeModel::eRole_EnginePath).toString();
			if (!index.model()->hasChildren(index))
				GetIEditorImpl()->ExecuteCommand("general.run_file '%s'", scriptPath.toUtf8().data());
		}
	}
}
