// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "VersionControlPendingChangesTab.h"
#include "AssetSystem/Loader/AssetLoaderHelpers.h"
#include "VersionControl/VersionControl.h"
#include "VersionControl/AssetFilesProvider.h"
#include "QtUtil.h"
#include "ProxyModels/AttributeFilterProxyModel.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QStackedWidget>

namespace Private_VersionControlPendingChangesTab
{

QString FileStatusToString(const CVersionControlFileStatus& fileStatus)
{
	if (fileStatus.HasState(CVersionControlFileStatus::eState_AddedLocally))
	{
		return "added locally";
	}
	else if (fileStatus.HasState(CVersionControlFileStatus::eState_AddedRemotely))
	{
		return "added remotely";
	}
	else if (fileStatus.HasState(CVersionControlFileStatus::eState_CheckedOutLocally))
	{
		return "checked out locally";
	}
	else if (fileStatus.HasState(CVersionControlFileStatus::eState_CheckedOutRemotely))
	{
		if (fileStatus.HasState(CVersionControlFileStatus::eState_UpdatedRemotely))
		{
			return "updated and checked out remotely";
		}
		else
		{
			return "checked out remotely";
		}
	}
	else if (fileStatus.HasState(CVersionControlFileStatus::eState_UpdatedRemotely))
	{
		return "updated remotely";
	}
	return "untouched";
}

static std::vector<CAsset*> g_assets;
static std::vector<CAsset*> g_selectedAssets;
static QString				g_description;

class CPendingChangesModel : public QAbstractItemModel
{
public:
	CPendingChangesModel()
	{
		CVersionControl::GetInstance().GetUpdateSignal().Connect(this, &CPendingChangesModel::Update);
		Update();
	}

	~CPendingChangesModel()
	{
		CVersionControl::GetInstance().GetUpdateSignal().DisconnectObject(this);
	}

	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const
	{
		if (!hasIndex(row, column, parent))
			return QModelIndex();

		return createIndex(row, column, g_assets[row]);
	}

	QModelIndex parent(const QModelIndex& child) const override
	{
		return QModelIndex();
	}

	int rowCount(const QModelIndex& parent = QModelIndex()) const override
	{
		return static_cast<int>(g_assets.size());
	}

	int columnCount(const QModelIndex&) const override
	{
		return 3;
	}

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
	{
		if (!index.isValid())
			return QVariant();

		if (role == Qt::DisplayRole || role == Qt::EditRole)
		{
			CAsset* pAsset = g_assets[index.row()];
			switch (index.column())
			{
			case 0:
				return pAsset->GetName().c_str();
			case 1:
				return pAsset->GetType()->GetUiTypeName();
			case 2:
				return pAsset->GetFolder().c_str();
			}
		}
		else if (role == Qt::CheckStateRole)
		{
			if (index.column() != 0) return QVariant();
			auto pAsset = g_assets[index.row()];
			auto it = std::find(g_selectedAssets.cbegin(), g_selectedAssets.cend(), pAsset);
			if (it != g_selectedAssets.cend())
			{
				return Qt::Checked;
			}
			return Qt::Unchecked;
		}

		return QVariant();
	}

	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override
	{
		if (!index.isValid()) return false;

		if (role == Qt::CheckStateRole)
		{
			auto pAsset = g_assets[index.row()];
			auto it = std::find(g_selectedAssets.cbegin(), g_selectedAssets.cend(), pAsset);
			if (it != g_selectedAssets.cend())
			{
				g_selectedAssets.erase(it);
			}
			else
			{
				g_selectedAssets.push_back(pAsset);
			}
		}

		return true;
	}

	Qt::ItemFlags flags(const QModelIndex& index) const override
	{
		if (!index.isValid())
			return 0;

		return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren | Qt::ItemIsUserCheckable;
	}

	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
	{
		if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
			return QVariant();

		switch (section)
		{
		case 0:
			return "Name";
		case 1:
			return "Type";
		default:
			return "Location";
		}

		return QVariant();
	}

	void Update()
	{
		const auto& vcs = CVersionControl::GetInstance();
		std::vector<std::shared_ptr<const CVersionControlFileStatus>> fileStatuses;
		fileStatuses.reserve(vcs.GetFilesNum() * 0.05);
		vcs.GetFileStatuses([this](const auto& fileStatus)
		{
			if (!AssetLoader::IsMetadataFile(fileStatus.GetFileName())) return false;
			return !fileStatus.IsUntouchedLocally();
		}, fileStatuses);

		beginResetModel();
		g_assets.clear();

		for (const auto& fs : fileStatuses)
		{
			// TODO: see if it's possible to handle situation when asset is marked for edit but then manually 
			// moved or renamed. There will be 2 files of the same asset but with different names (modified and added).
			CAsset* pAsset = GetIEditor()->GetAssetManager()->FindAssetForMetadata(fs->GetFileName());
			if (pAsset && strcmp(pAsset->GetType()->GetTypeName(), "Level") != 0)
			{
				g_assets.push_back(pAsset);
			}
		}

		DeselectMissingAssets();

		endResetModel();
	}

	void DeselectMissingAssets()
	{
		if (g_assets.empty())
		{
			g_selectedAssets.clear();
			return;
		}
		auto it = std::remove_if(g_selectedAssets.begin(), g_selectedAssets.end(), [this](CAsset* pSelectedAsset)
		{
			return std::find(g_assets.cbegin(), g_assets.cend(), pSelectedAsset) == g_assets.cend();
		});
		if (!g_selectedAssets.empty() && it != g_selectedAssets.end())
		{
			g_selectedAssets.erase(it);
		}
	}

	void SetSelectedAssets(std::vector<CAsset*> assets)
	{
		g_selectedAssets = std::move(assets);
	}

	const std::vector<CAsset*>& GetSelectedAssets() const { return g_selectedAssets; }
};

CPendingChangesModel* GetModel(QTreeView* pTree)
{
	return static_cast<CPendingChangesModel*>(pTree->model());
}

}

void CVersionControlPendingChangesTab::SelectAssets(std::vector<CAsset*> assets)
{
	using namespace Private_VersionControlPendingChangesTab;
	GetModel(m_pTree)->SetSelectedAssets(std::move(assets));
	GetModel(m_pTree)->DeselectMissingAssets();
}

CVersionControlPendingChangesTab::CVersionControlPendingChangesTab(QWidget* pParent /*= nullptr*/)
{
	using namespace Private_VersionControlPendingChangesTab;

	auto pLayout = new QVBoxLayout();
	setLayout(pLayout);

	CPendingChangesModel* pModel = new CPendingChangesModel();
	QAttributeFilterProxyModel* pSortModel = new QAttributeFilterProxyModel(QAttributeFilterProxyModel::BaseBehavior, this);
	pSortModel->setSourceModel(pModel);

	m_pTree = new QTreeView(this);
	m_pTree->setModel(pSortModel);
	m_pTree->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_pTree->setSelectionMode(QAbstractItemView::SingleSelection);
	m_pTree->setSortingEnabled(true);
	m_pTree->sortByColumn(0, Qt::AscendingOrder);

	pLayout->addWidget(m_pTree);

	m_textEdit = new QPlainTextEdit(this);
	m_textEdit->setPlaceholderText(tr("Enter submit description"));
	m_textEdit->setBackgroundVisible(true);
	m_textEdit->setFixedHeight(60);
	m_textEdit->setPlainText(g_description);
	pLayout->addWidget(m_textEdit);

	auto pSubmitButton = new QPushButton(tr("Submit"));
	auto pConfirmButton = new QPushButton(tr("Confirm"));
	auto pCancelButton = new QPushButton(tr("Cancel"));
	QObject::connect(pSubmitButton, &QPushButton::clicked, this, [this] { OnSubmit(); });
	QObject::connect(pConfirmButton, &QPushButton::clicked, this, [this] { OnConfirmSubmit(); });
	QObject::connect(pCancelButton, &QPushButton::clicked, this, [this] { OnCancelSubmit(); });

	auto pConfirmLayout = new QHBoxLayout();
	pConfirmLayout->setContentsMargins(0, 0, 0, 0);
	pConfirmLayout->addWidget(pConfirmButton);
	pConfirmLayout->addWidget(pCancelButton);

	auto pConfirmWidget = new QWidget();
	pConfirmWidget->setLayout(pConfirmLayout);

	m_pStackedWidget = new QStackedWidget();
	m_pStackedWidget->setMaximumHeight(22);
	m_pStackedWidget->addWidget(pSubmitButton);
	m_pStackedWidget->addWidget(pConfirmWidget);

	pLayout->addWidget(m_pStackedWidget);

	CVersionControl::GetInstance().GetUpdateSignal().Connect(this, &CVersionControlPendingChangesTab::OnCancelSubmit);
}

CVersionControlPendingChangesTab::~CVersionControlPendingChangesTab()
{
	CVersionControl::GetInstance().GetUpdateSignal().DisconnectObject(this);
	Private_VersionControlPendingChangesTab::g_description = m_textEdit->toPlainText();
}

void CVersionControlPendingChangesTab::OnSubmit()
{
	using namespace Private_VersionControlPendingChangesTab;
	if (GetModel(m_pTree)->GetSelectedAssets().empty())
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "No assets selected for submission");
		return;
	}
	if (m_textEdit->toPlainText().isEmpty())
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Submit description can not be empty");
		return;
	}
	m_pStackedWidget->setCurrentIndex(1);
}

void CVersionControlPendingChangesTab::OnConfirmSubmit()
{
	using namespace Private_VersionControlPendingChangesTab;
	const auto& files = CAssetFilesProvider::GetForAssets(GetModel(m_pTree)->GetSelectedAssets());
	CVersionControl::GetInstance().SubmitFiles(files, QtUtil::ToString(m_textEdit->toPlainText()), false
		, [this](const auto& result)
	{
		if (result.IsSuccess())
		{
			m_textEdit->clear();
			g_description = "";
		}

	});
	m_pStackedWidget->setCurrentIndex(0);
}

void CVersionControlPendingChangesTab::OnCancelSubmit()
{
	m_pStackedWidget->setCurrentIndex(0);
}
