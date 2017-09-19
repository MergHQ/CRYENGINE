// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "QtUtil.h"
#include "MiddlewareDataWidget.h"
#include "IAudioSystemEditor.h"
#include "AudioControlsEditorPlugin.h"
#include "MiddlewareDataModel.h"
#include "ImplementationManager.h"
#include "AdvancedTreeView.h"

#include <CryIcon.h>
#include <QSearchBox.h>

// Qt
#include <QtUtil.h>
#include <QCheckBox>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QFontMetrics>

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

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
CMiddlewareDataWidget::CMiddlewareDataWidget()
	: m_pModelProxy(new CMiddlewareDataFilterProxyModel(this))
	, m_pModel(new CMiddlewareDataModel())
	, m_pImplNameLabel(nullptr)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	m_pModelProxy->setDynamicSortFilter(true);
	m_pModelProxy->setSourceModel(m_pModel);

	QVBoxLayout* pMainLayout = new QVBoxLayout(this);
	pMainLayout->setContentsMargins(0, 0, 0, 0);

	m_pImplNameLabel = new CElidedLabel("");
	IAudioSystemEditor* pAudioImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();

	if (pAudioImpl)
	{
		m_pImplNameLabel->SetLabelText(QtUtil::ToQString(pAudioImpl->GetName()));
	}

	QHBoxLayout* pTopBarLayout = new QHBoxLayout();

	QSearchBox* pSearchBox = new QSearchBox();
	pSearchBox->setToolTip(tr("Show only middleware controls with this name"));
	connect(pSearchBox, &QSearchBox::textChanged, [&](QString const& filter)
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
				m_pModelProxy->setFilterFixedString(filter);
				m_pTreeView->collapseAll();
				RestoreTreeViewStates();
			}
			else if (!m_filter.isEmpty() && !filter.isEmpty())
			{
				m_pModelProxy->setFilterFixedString(filter);
				m_pTreeView->expandAll();
			}

			m_filter = filter;
		}

		m_pModelProxy->setFilterFixedString(filter);
	});
	
	QCheckBox* pHideAssignedCheckbox = new QCheckBox();
	pHideAssignedCheckbox->setText(tr("Hide Assigned"));
	pHideAssignedCheckbox->setToolTip(tr("Hide or show assigned middleware controls"));
	connect(pHideAssignedCheckbox, &QCheckBox::clicked, [&](bool bHide)
	{
		if (bHide)
		{
			m_pTreeView->BackupExpanded();
			m_pModelProxy->SetHideConnected(bHide);
		}
		else
		{
			m_pModelProxy->SetHideConnected(bHide);
			m_pTreeView->RestoreExpanded();
		}
	});

	pTopBarLayout->addWidget(pSearchBox);
	pTopBarLayout->addWidget(pHideAssignedCheckbox);

	m_pTreeView = new CAdvancedTreeView();
	m_pTreeView->header()->setVisible(false);
	m_pTreeView->setDragEnabled(true);
	m_pTreeView->setDragDropMode(QAbstractItemView::DragOnly);
	m_pTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_pTreeView->setSortingEnabled(true);
	m_pTreeView->sortByColumn(0, Qt::AscendingOrder);
	m_pTreeView->setModel(m_pModelProxy);
	m_pTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_pTreeView, &CAdvancedTreeView::customContextMenuRequested, this, &CMiddlewareDataWidget::OnContextMenu);
	connect(m_pTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, m_pTreeView, &CAdvancedTreeView::OnSelectionChanged);

	pMainLayout->addWidget(m_pImplNameLabel);
	pMainLayout->addLayout(pTopBarLayout);
	pMainLayout->addWidget(m_pTreeView);

	// Update the middleware name label.
	// Note the 'this' ptr being passed as a context variable so that Qt can disconnect this lambda when the object is destroyed (ie. the ACE is closed).
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
void CMiddlewareDataWidget::SetAllowedControls(EItemType type, bool bAllowed)
{
	const ACE::IAudioSystemEditor* pAudioSystemEditorImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();

	if (pAudioSystemEditorImpl)
	{
		m_allowedATLTypes[type] = bAllowed;
		uint mask = 0;

		for (int i = 0; i < EItemType::eItemType_NumTypes; ++i)
		{
			if (m_allowedATLTypes[i])
			{
				mask |= pAudioSystemEditorImpl->GetCompatibleTypes((EItemType)i);
			}
		}

		m_pModelProxy->SetAllowedControlsMask(mask);
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

	pContextMenu->exec(m_pTreeView->mapToGlobal(pos));
}

//////////////////////////////////////////////////////////////////////////
void CMiddlewareDataWidget::Reset()
{
	m_pModel->Reset();
	m_pModelProxy->invalidate();
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
