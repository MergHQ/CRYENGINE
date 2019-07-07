// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "VersionControlPendingChangesWIdget.h"
#include "VersionControlUIHelper.h"
#include "PendingChange.h"
#include "AssetSystem/Loader/AssetLoaderHelpers.h"
#include "AssetSystem/AssetManager.h"
#include "VersionControl/DeletedWorkFilesStorage.h"
#include "VersionControl/VersionControl.h"
#include "VersionControl/AssetsVCSStatusProvider.h"
#include "ProxyModels/AttributeFilterProxyModel.h"
#include <CryString/CryPath.h>
#include <QApplication>
#include <QBoxLayout>
#include <QHeaderView>
#include <QProxyStyle>
#include <QStyledItemDelegate>
#include <QTreeView>

namespace Private_VersionControlPendingChangesTab
{

static QString g_description;

bool IsLayerFile(const char* fileName)
{
	const char* const szExt = PathUtil::GetExt(fileName);
	return strcmp(szExt, "lyr") == 0; 
}

bool IsDeletedWorkFiles(const CVersionControlFileStatus& fs)
{
	return CDeletedWorkFilesStorage::GetInstance().Contains(fs.GetFileName()) 
		&& fs.HasState(CVersionControlFileStatus::eState_DeletedLocally);
}

bool IsWorkFile(const CVersionControlFileStatus& fs)
{
	return CAssetManager::GetInstance()->GetWorkFilesTracker().GetIndexCount(fs.GetFileName()) > 0 || IsDeletedWorkFiles(fs);
}

QIcon GetStatusIconForPendingChange(const CPendingChange* pPendingChange)
{
	if (!CVersionControl::GetInstance().IsOnline())
	{
		return QIcon();
	}
	const int state = CVersionControl::GetInstance().GetFileStatus(pPendingChange->GetMainFile())->GetState();
	return VersionControlUIHelper::GetIconFromStatus(state);
}

//! Model that handles and displays data for pending change list.
class CPendingChangesModel : public QAbstractItemModel
{
public:
	CPendingChangesModel()
	{
		CVersionControl::GetInstance().GetUpdateSignal().Connect(this, &CPendingChangesModel::Update);
		if (CPendingChangeList::GetNumPendingChanges() == 0)
		{
			Update();
		}
	}

	~CPendingChangesModel()
	{
		CVersionControl::GetInstance().GetUpdateSignal().DisconnectObject(this);
	}

	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const
	{
		if (!hasIndex(row, column, parent))
		{
			return QModelIndex();
		}

		return createIndex(row, column, CPendingChangeList::GetPendingChange(row));
	}

	QModelIndex parent(const QModelIndex& child) const override
	{
		return QModelIndex();
	}

	int rowCount(const QModelIndex& parent = QModelIndex()) const override
	{
		return static_cast<int>(CPendingChangeList::GetNumPendingChanges());
	}

	int columnCount(const QModelIndex&) const override
	{
		return 5;
	}

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
	{
		if (!index.isValid())
		{
			return QVariant();
		}

		if (role == Qt::DisplayRole)
		{
			const CPendingChange* pPendingChange = CPendingChangeList::GetPendingChange(index.row());
			switch (index.column())
			{
			case 2:
				return pPendingChange->GetName();
			case 3:
				return pPendingChange->GetTypeName();
			case 4:
				return pPendingChange->GetLocation();
			}
		}
		else if (role == Qt::DecorationRole && index.column() == 1)
		{
			return GetStatusIconForPendingChange(CPendingChangeList::GetPendingChange(index.row()));
		}
		else if (role == Qt::CheckStateRole && index.column() == 0)
		{
			const CPendingChange* pPendingChange = CPendingChangeList::GetPendingChange(index.row());
			return pPendingChange->IsChecked() ? Qt::Checked : Qt::Unchecked;
		}

		return QVariant();
	}

	bool setData(const QModelIndex& index, const QVariant& value, int role) override
	{
		if (!index.isValid())
		{
			return false;
		}

		if (role == Qt::CheckStateRole)
		{
			CPendingChange* pPendingChange = CPendingChangeList::GetPendingChange(index.row());
			pPendingChange->Check(!pPendingChange->IsChecked());
			signalPendingChangeChecked(pPendingChange);
		}

		return true;
	}

	Qt::ItemFlags flags(const QModelIndex& index) const override
	{
		if (!index.isValid())
			return 0;

		const static int flags = Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;

		if (index.column() == 0)
		{
			return flags | Qt::ItemIsUserCheckable;
		}

		return flags;
	}

	QVariant headerData(int section, Qt::Orientation orientation, int role) const override
	{
		if (orientation != Qt::Horizontal)
		{
			return QVariant();
		}

		if (role == Qt::DisplayRole)
		{
			switch (section)
			{
			case 2:
				return "Name";
			case 3:
				return "Type";
			case 4:
				return "Location";
			}
		}
		else if (role == Qt::DecorationRole)
		{
			if (section == 1)
			{
				return CryIcon("icons:VersionControl/icon.ico");
			}
		}

		return QVariant();
	}

	//! Updates the content of pending changes by going through the list of all files in the VCS cache
	//! and filtering out everything but .cryasset and .lyr files.
	void Update()
	{
		const auto& vcs = CVersionControl::GetInstance();
		std::vector<std::shared_ptr<const CVersionControlFileStatus>> fileStatuses;
		fileStatuses.reserve(vcs.GetFilesNum() * 0.05);
		vcs.GetFileStatuses([this](const auto& fs)
		{
			if (fs.IsUntouchedLocally()) return false;
			return AssetLoader::IsMetadataFile(fs.GetFileName()) || IsLayerFile(fs.GetFileName()) || IsWorkFile(fs);
		}, fileStatuses);

		beginResetModel();

		std::vector<string> selectedMainFiles;
		CPendingChangeList::ForEachPendingChange([&selectedMainFiles](CPendingChange& pPendingChange)
		{
			if (pPendingChange.IsChecked())
			{
				selectedMainFiles.push_back(pPendingChange.GetMainFile());
			}
		});
		CPendingChangeList::Clear();

		for (const auto& fs : fileStatuses)
		{
			CPendingChangeList::CreatePendingChangeFor(fs->GetFileName());
		}

		SelectPendingChangesIfPresent(selectedMainFiles);

		endResetModel();
	}

	//! Looks for pending changes that correspond to the given list of files and marks them for submission.
	//! Assumes all pending changes in the list are deselected.
	void SelectPendingChangesIfPresent(const std::vector<string>& selectedMainFiles)
	{
		for (const string& selectedMainFile : selectedMainFiles)
		{
			CPendingChange* pPendingChange = CPendingChangeList::FindPendingChangeFor(selectedMainFile);
			if (pPendingChange)
			{
				pPendingChange->SetChecked(true);
			}
		}
	}

	//! Marks pending changes that correspond to the given main files as selected if they can be submitted.
	//! \param shouldDeselectCurrent specifies if the current selected item need to be cleared.
	void SetSelectedMainFiles(const std::vector<string>& mainFiles, bool shouldDeselectCurrent = true)
	{
		if (shouldDeselectCurrent)
		{
			CPendingChangeList::ForEachPendingChange([](CPendingChange& pPendingChange)
			{
				pPendingChange.SetChecked(false);
			});
		}
		for (const string& mainFile : mainFiles)
		{
			CPendingChange* pPendingChange = CPendingChangeList::FindPendingChangeFor(mainFile);
			if (pPendingChange)
			{
				pPendingChange->Check(true);
			}
		}
	}

	void SetSelectedAllUnderFolders(const std::vector<string>& folders, bool shouldDeselectCurrent = true)
	{
		if (shouldDeselectCurrent)
		{
			CPendingChangeList::ForEachPendingChange([](CPendingChange& pPendingChange)
			{
				pPendingChange.SetChecked(false);
			});
		}
		for (const string& folder : folders)
		{
			for (int i = 0; i < CPendingChangeList::GetNumPendingChanges(); ++i)
			{
				CPendingChange* pPendingChange = CPendingChangeList::GetPendingChange(i);
				if (pPendingChange && pPendingChange->GetMainFile().compareNoCase(0, folder.size(), folder) == 0)
				{
					pPendingChange->Check(true);
				}
			}
		}
	}
	
	std::vector<CPendingChange*> GetSelectedPendingChanges() const
	{
		std::vector<CPendingChange*> result;
		result.reserve(CPendingChangeList::GetNumPendingChanges() * 0.5);
		CPendingChangeList::ForEachPendingChange([&result](CPendingChange& pPendingChange)
		{
			if (pPendingChange.IsChecked())
			{
				result.push_back(&pPendingChange);
			}
		});
		return result;
	}

	CCrySignal<void(CPendingChange* pPendingChange)> signalPendingChangeChecked;
};

class CStatusIconItemDelegate : public QStyledItemDelegate
{
public:
	CStatusIconItemDelegate(QObject* pParent) 
		: QStyledItemDelegate(pParent) 
	{}

	void paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		const QStyle* style = option.widget ? option.widget->style() : qApp->style();
		style->drawItemPixmap(pPainter, option.rect, Qt::AlignCenter, index.data(Qt::DecorationRole).value<QIcon>().pixmap(option.rect.size()));
	}
};

class CCheckBoxItemDelegate : public QStyledItemDelegate
{
public:
	CCheckBoxItemDelegate(QObject* pParent)
		: QStyledItemDelegate(pParent)
	{}

	void paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		Qt::CheckState state = (Qt::CheckState)index.data(Qt::CheckStateRole).toInt();
		QStyleOptionButton opt;

		opt.state = QStyle::State_Enabled; 
		switch (state) 
		{
		case Qt::Unchecked:
			opt.state |= QStyle::State_Off;
			break;
		case Qt::Checked:
			opt.state |= QStyle::State_On;
			break;
		}

		opt.rect = QApplication::style()->subElementRect(QStyle::SE_CheckBoxIndicator, &opt, NULL);
		const int x = option.rect.center().x() - opt.rect.width() / 2;
		const int y = option.rect.center().y() - opt.rect.height() / 2;
		opt.rect.moveTo(x, y);

		const QStyle* pStyle = option.widget ? option.widget->style() : qApp->style();
		pStyle->drawControl(QStyle::CE_CheckBox, &opt, pPainter);
	}

	bool editorEvent(QEvent* pEvent, QAbstractItemModel* pModel, const QStyleOptionViewItem& option, const QModelIndex& index)
	{
		if (pEvent->type() == QEvent::MouseButtonRelease)
		{
			Qt::CheckState state = (Qt::CheckState)index.data(Qt::CheckStateRole).toInt();
			switch (state)
			{
			case Qt::Unchecked:
				state = Qt::Checked;
				break;
			case Qt::Checked:
				state = Qt::Unchecked;
				break;
			}

			pModel->setData(index, state, Qt::CheckStateRole);
			return true;
		}
		return QAbstractItemDelegate::editorEvent(pEvent, pModel, option, index);
	}
};

class CHeaderProxyStyle : public QProxyStyle
{
public:
	void drawControl(ControlElement ctrElement, const QStyleOption* pStyleOption, QPainter* pPainter, const QWidget* pWidget) const override
	{
		if (ctrElement != CE_HeaderLabel)
		{
			QProxyStyle::drawControl(ctrElement, pStyleOption, pPainter, pWidget);
			return;
		}

		auto pStyleOptionHeader = static_cast<QStyleOptionHeader*>(const_cast<QStyleOption*>(pStyleOption));

		const QIcon icon = qvariant_cast<QIcon>(pStyleOptionHeader->icon);

		if (icon.isNull()) 
		{
			QProxyStyle::drawControl(ctrElement, pStyleOption, pPainter, pWidget);
			return;
		}

		static const QSize iconSize = QSize(16, 16);

		const QRect rect = pStyleOptionHeader->rect;

		const QPixmap iconPixmap = icon.pixmap(iconSize.width(), iconSize.height());

		const QRect centerRect = QRect(rect.center().x() - iconSize.width() / 2, rect.center().y() - iconSize.height() / 2,
			iconPixmap.width(), iconPixmap.height());

		pPainter->drawPixmap(centerRect, iconPixmap);
	}
};

CPendingChangesModel* GetModel(QTreeView* pTree)
{
	return static_cast<CPendingChangesModel*>(pTree->model());
}

}

void CVersionControlPendingChangesWidget::SelectAssets(const std::vector<CAsset*>& assets, bool shouldDeselectCurrent)
{
	using namespace Private_VersionControlPendingChangesTab;
	std::vector<string> metadataFiles;
	metadataFiles.reserve(assets.size());
	for (CAsset* pAsset : assets)
	{
		metadataFiles.push_back(pAsset->GetMetadataFile());
	}
	GetModel(m_pTree)->SetSelectedMainFiles(metadataFiles, shouldDeselectCurrent);
	signalSelectionChanged();
}

void CVersionControlPendingChangesWidget::SelectFiles(const std::vector<string>& layersFiles, bool shouldDeselectCurrent)
{
	using namespace Private_VersionControlPendingChangesTab;
	GetModel(m_pTree)->SetSelectedMainFiles(layersFiles, shouldDeselectCurrent);
	signalSelectionChanged();
}

void CVersionControlPendingChangesWidget::SelectFolders(const std::vector<string>& folders, bool shouldDeselectCurrent)
{
	using namespace Private_VersionControlPendingChangesTab;
	GetModel(m_pTree)->SetSelectedAllUnderFolders(folders, shouldDeselectCurrent);
	signalSelectionChanged();
}

std::vector<CPendingChange*> CVersionControlPendingChangesWidget::GetSelectedPendingChanges() const
{
	using namespace Private_VersionControlPendingChangesTab;
	return GetModel(m_pTree)->GetSelectedPendingChanges();
}

CVersionControlPendingChangesWidget::CVersionControlPendingChangesWidget(QWidget* pParent)
	: QWidget(pParent)
{
	using namespace Private_VersionControlPendingChangesTab;

	auto pLayout = new QVBoxLayout();
	pLayout->setMargin(0);
	setLayout(pLayout);

	CPendingChangesModel* pModel = new CPendingChangesModel();
	QAttributeFilterProxyModel* pSortModel = new QAttributeFilterProxyModel(QAttributeFilterProxyModel::BaseBehavior, this);
	pSortModel->setSourceModel(pModel);

	pModel->signalPendingChangeChecked.Connect([this](CPendingChange*)
	{
		signalSelectionChanged();
	});

	m_pTree = new QTreeView(this);
	m_pTree->setModel(pSortModel);
	m_pTree->setSelectionMode(QAbstractItemView::NoSelection);
	m_pTree->setSortingEnabled(true);
	m_pTree->sortByColumn(0, Qt::AscendingOrder);
	m_pTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	m_pTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	m_pTree->setBaseSize(460, 280);

	static CHeaderProxyStyle s_headerProxyStyle;
	m_pTree->header()->setStyle(&s_headerProxyStyle);

	QStyledItemDelegate* pCheckBoxItemDelegate = new CCheckBoxItemDelegate(m_pTree);
	m_pTree->setItemDelegateForColumn(0, pCheckBoxItemDelegate);

	QStyledItemDelegate* pStatusIconItemDelegate = new CStatusIconItemDelegate(m_pTree);
	m_pTree->setItemDelegateForColumn(1, pStatusIconItemDelegate);

	pLayout->addWidget(m_pTree);

	CPendingChange::s_signalDataUpdatedIndirectly.Connect([this](const std::vector<CPendingChange*>&)
	{
		m_pTree->reset();
	}, reinterpret_cast<uintptr_t>(this));
}

CVersionControlPendingChangesWidget::~CVersionControlPendingChangesWidget()
{
	using namespace Private_VersionControlPendingChangesTab;
	CVersionControl::GetInstance().GetUpdateSignal().DisconnectObject(this);
	CPendingChange::s_signalDataUpdatedIndirectly.DisconnectObject(this);
}
