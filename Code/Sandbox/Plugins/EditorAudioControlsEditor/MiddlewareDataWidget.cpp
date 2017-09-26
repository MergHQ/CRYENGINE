// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MiddlewareDataWidget.h"

#include "IAudioSystemEditor.h"
#include "AudioControlsEditorPlugin.h"
#include "MiddlewareDataModel.h"
#include "ImplementationManager.h"
#include "AudioTreeView.h"

#include <CryIcon.h>
#include <QSearchBox.h>
#include <QtUtil.h>

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QToolButton>
#include <QVBoxLayout>

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
class CElidedLabel final : public QLabel
{
public:

	CElidedLabel::CElidedLabel(QString const& text)
	{
		setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
		SetLabelText(text);
	}

	void SetLabelText(QString const& text)
	{
		m_originalText = text;
		ElideText();
	}

private:

	// QWidget
	virtual void resizeEvent(QResizeEvent *) override
	{
		ElideText();
	}
	// ~QWidget

	void ElideText()
	{
		QFontMetrics const metrics(font());
		QString const elidedText = metrics.elidedText(m_originalText, Qt::ElideRight, size().width());
		setText(elidedText);
	}

	QString m_originalText;
};

//////////////////////////////////////////////////////////////////////////
CMiddlewareDataWidget::CMiddlewareDataWidget()
	: m_pFilterProxyModel(new CMiddlewareDataFilterProxyModel(this))
	, m_pAssetsModel(new CMiddlewareDataModel())
	, m_pImplNameLabel(nullptr)
{
	

	m_pFilterProxyModel->setDynamicSortFilter(true);
	m_pFilterProxyModel->setSourceModel(m_pAssetsModel);

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	QVBoxLayout* pMainLayout = new QVBoxLayout(this);
	pMainLayout->setContentsMargins(0, 0, 0, 0);

	m_pImplNameLabel = new CElidedLabel("");
	IAudioSystemEditor* pAudioImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();

	if (pAudioImpl)
	{
		m_pImplNameLabel->SetLabelText(QtUtil::ToQString(pAudioImpl->GetName()));
	}

	pMainLayout->addWidget(m_pImplNameLabel);

	InitFilterWidgets(pMainLayout);

	m_pTreeView = new CAudioTreeView();
	m_pTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_pTreeView->setDragEnabled(true);
	m_pTreeView->setDragDropMode(QAbstractItemView::DragOnly);
	m_pTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_pTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
	m_pTreeView->setModel(m_pFilterProxyModel);
	m_pTreeView->sortByColumn(0, Qt::AscendingOrder);
	pMainLayout->addWidget(m_pTreeView);
	
	QObject::connect(m_pTreeView, &CAudioTreeView::customContextMenuRequested, this, &CMiddlewareDataWidget::OnContextMenu);
	QObject::connect(m_pTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, m_pTreeView, &CAudioTreeView::OnSelectionChanged);
	
	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationChanged.Connect([&]()
	{
		IAudioSystemEditor* pAudioImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();

		if (pAudioImpl)
		{
			m_pImplNameLabel->SetLabelText(QtUtil::ToQString(pAudioImpl->GetName()));
		}
	}, reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
CMiddlewareDataWidget::~CMiddlewareDataWidget()
{
	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationChanged.DisconnectById(reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
void CMiddlewareDataWidget::InitFilterWidgets(QVBoxLayout* pMainLayout)
{
	QHBoxLayout* pFilterLayout = new QHBoxLayout();

	QSearchBox* pSearchBox = new QSearchBox();
	QObject::connect(pSearchBox, &QSearchBox::textChanged, [&](QString const& filter)
	{
		if (m_filter != filter)
		{
			if (m_filter.isEmpty() && !filter.isEmpty())
			{
				BackupTreeViewStates();
				m_pTreeView->expandAll();
			}
			else if (!m_filter.isEmpty() && filter.isEmpty())
			{
				m_pFilterProxyModel->setFilterFixedString(filter);
				m_pTreeView->collapseAll();
				RestoreTreeViewStates();
			}
			else if (!m_filter.isEmpty() && !filter.isEmpty())
			{
				m_pFilterProxyModel->setFilterFixedString(filter);
				m_pTreeView->expandAll();
			}

			m_filter = filter;
		}

		m_pFilterProxyModel->setFilterFixedString(filter);
	});

	pFilterLayout->addWidget(pSearchBox);

	m_pHideAssignedButton = new QToolButton();
	m_pHideAssignedButton->setIcon(CryIcon("icons:General/Visibility_True.ico"));
	m_pHideAssignedButton->setToolTip(tr("Hide assigned middleware data"));
	m_pHideAssignedButton->setCheckable(true);
	m_pHideAssignedButton->setMaximumSize(QSize(20, 20));
	QObject::connect(m_pHideAssignedButton, &QToolButton::toggled, [&](bool const bChecked)
	{
		if (bChecked)
		{
			BackupTreeViewStates();
			m_pFilterProxyModel->SetHideConnected(bChecked);
			m_pHideAssignedButton->setIcon(CryIcon("icons:General/Visibility_False.ico"));
			m_pHideAssignedButton->setToolTip(tr("Show all middleware data"));
		}
		else
		{
			m_pFilterProxyModel->SetHideConnected(bChecked);
			RestoreTreeViewStates();
			m_pHideAssignedButton->setIcon(CryIcon("icons:General/Visibility_True.ico"));
			m_pHideAssignedButton->setToolTip(tr("Hide assigned middleware data"));
		}
	});

	pFilterLayout->addWidget(m_pHideAssignedButton);

	pMainLayout->addLayout(pFilterLayout);
}

//////////////////////////////////////////////////////////////////////////
void CMiddlewareDataWidget::SetAllowedControls(EItemType const type, bool const bAllowed)
{
	const IAudioSystemEditor* pAudioSystemEditorImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();

	if (pAudioSystemEditorImpl)
	{
		m_allowedATLTypes[static_cast<int>(type)] = bAllowed;
		uint mask = 0;
		int const numTypes = static_cast<int>(EItemType::NumTypes);

		for (int i = 0; i < numTypes; ++i)
		{
			if (m_allowedATLTypes[i])
			{
				mask |= pAudioSystemEditorImpl->GetCompatibleTypes((EItemType)i);
			}
		}

		m_pFilterProxyModel->SetAllowedControlsMask(mask);
	}
}

//////////////////////////////////////////////////////////////////////////
void CMiddlewareDataWidget::OnContextMenu(QPoint const& pos) const
{
	QMenu* pContextMenu = new QMenu();
	auto const selection = m_pTreeView->selectionModel()->selectedRows();

	if (!selection.isEmpty())
	{
		pContextMenu->addAction(tr("Expand Selection"), [&]() { m_pTreeView->ExpandSelection(m_pTreeView->GetSelectedIndexes()); });
		pContextMenu->addAction(tr("Collapse Selection"), [&]() { m_pTreeView->CollapseSelection(m_pTreeView->GetSelectedIndexes()); });
		pContextMenu->addSeparator();
	}

	pContextMenu->addAction(tr("Expand All"), [&]() { m_pTreeView->expandAll(); });
	pContextMenu->addAction(tr("Collapse All"), [&]() { m_pTreeView->collapseAll(); });

	pContextMenu->exec(QCursor::pos());
}

//////////////////////////////////////////////////////////////////////////
void CMiddlewareDataWidget::Reset()
{
	m_pAssetsModel->Reset();
	m_pFilterProxyModel->invalidate();
}

//////////////////////////////////////////////////////////////////////////
void CMiddlewareDataWidget::BackupTreeViewStates()
{
	m_pTreeView->BackupExpanded();
	m_pTreeView->BackupSelection();
}

//////////////////////////////////////////////////////////////////////////
void CMiddlewareDataWidget::RestoreTreeViewStates()
{
	m_pTreeView->RestoreExpanded();
	m_pTreeView->RestoreSelection();
}
} // namespace ACE
