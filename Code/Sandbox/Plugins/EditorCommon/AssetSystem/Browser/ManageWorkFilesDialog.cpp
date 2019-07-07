// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "ManageWorkFilesDialog.h"

#include "AssetSystem/Asset.h"
#include "AssetSystem/AssetManager.h"
#include "AssetSystem/AssetImportContext.h"
#include "Controls/QuestionDialog.h"
#include "EditorStyleHelper.h"
#include "FileDialogs/EngineFileDialog.h"
#include "PathUtils.h"
#include "ProxyModels/AttributeFilterProxyModel.h"
#include "QtUtil.h"
#include "VersionControl/VersionControl.h"
#include "VersionControl/VersionControlPathUtils.h"
#include "VersionControl/DeletedWorkFilesStorage.h"

#include <QBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTreeView>
#include <QHeaderView>
#include <QFileInfo>
#include <QLabel>

namespace Private_DefineWorkFilesWindow
{

constexpr int DELETE_ROW_ROLE = Qt::UserRole + 1;

bool IsValidWorkFile(const string& file)
{
	auto pManager = CAssetManager::GetInstance();
	return pManager->FindAssetForFile(file) == nullptr && pManager->GetSourceFilesTracker().GetIndexCount(file) == 0;
}

class CWorkFilesModel : public QAbstractItemModel
{
public:
	explicit CWorkFilesModel(CAsset* pAsset)
		: m_pAsset(pAsset)
	{
		m_workFiles = pAsset->GetWorkFiles();
		m_isAdded.resize(m_workFiles.size(), 0);
	}

	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const
	{
		if (!hasIndex(row, column, parent))
		{
			return QModelIndex();
		}

		return createIndex(row, column, row);
	}

	QModelIndex parent(const QModelIndex& child) const override
	{
		return QModelIndex();
	}

	int rowCount(const QModelIndex& parent = QModelIndex()) const override
	{
		return static_cast<int>(m_workFiles.size());
	}

	int columnCount(const QModelIndex&) const override
	{
		return 3;
	}

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
	{
		if (!index.isValid())
		{
			return QVariant();
		}

		if (role == Qt::DisplayRole)
		{
			switch (index.column())
			{
			case 0:
				return QtUtil::ToQString(PathUtil::GetFile(m_workFiles[index.row()]));
			case 1:
				return QtUtil::ToQString(PathUtil::GetDirectory(m_workFiles[index.row()]));
			case 2:
				return CAssetManager::GetInstance()->GetWorkFilesTracker().GetIndexCount(m_workFiles[index.row()]) + m_isAdded[index.row()];
			}
		}

		return QVariant();
	}

	bool setData(const QModelIndex& index, const QVariant& value, int role) override
	{
		if (!index.isValid())
		{
			return false;
		}

		if (role == DELETE_ROW_ROLE)
		{
			DeleteWorkFile(index.row());
		}

		return true;
	}

	Qt::ItemFlags flags(const QModelIndex& index) const override
	{
		if (!index.isValid())
			return 0;

		const static int flags = Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;

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
			case 0:
				return "Name";
			case 1:
				return "Path";
			case 2:
				return "Usage";
			}
		}

		return QVariant();
	}

	void AddWorkFiles(std::vector<string> workFiles)
	{
		ValidateFiles(workFiles);
		if (workFiles.empty())
		{
			return;
		}
		beginResetModel();
		const int originalSize = m_isAdded.size();
		for (const string& workFile : workFiles)
		{
			auto it = std::find(m_workFiles.cbegin(), m_workFiles.cend(), workFile);
			if (it == m_workFiles.cend())
			{
				m_workFiles.push_back(workFile);
				m_isAdded.push_back(1);
			}
		}
		const int numAdded = m_workFiles.size() - originalSize;
		for (int i = 0; i < numAdded; ++i)
		{
			auto it = std::find(m_pAsset->GetWorkFiles().cbegin(), m_pAsset->GetWorkFiles().cend(), workFiles[i]);
			if (it != m_pAsset->GetWorkFiles().cend())
			{
				m_isAdded[originalSize + i] = 0;
			}
		}
		endResetModel();
	}

	void DeleteWorkFile(int row)
	{
		beginResetModel();
		m_workFiles.erase(m_workFiles.begin() + row);
		m_isAdded.erase(m_isAdded.begin() + row);
		endResetModel();
	}

	void SaveAsset()
	{
		if (m_workFiles == m_pAsset->GetWorkFiles())
		{
			return;
		}
		
		QFileInfo info(QtUtil::ToQString(PathUtil::Make(PathUtil::GetGameProjectAssetsRelativePath(), m_pAsset->GetMetadataFile())));
		if (!info.isWritable() && CQuestionDialog::SQuestion(tr("Saving read-only asset"),
			tr("The asset that you want to save is read-only. Do you want to override it?")) != QDialogButtonBox::Yes)
		{
			return;
		}

		CAssetManager::GetInstance()->GetWorkFilesTracker().SetIndices(m_workFiles, *m_pAsset);

		for (const string& oldWorkFile : m_pAsset->GetWorkFiles())
		{
			if (CAssetManager::GetInstance()->GetWorkFilesTracker().GetIndexCount(oldWorkFile) == 0)
			{
				CDeletedWorkFilesStorage::GetInstance().Add(oldWorkFile);
			}
		}

		CAssetImportContext ctx;
		CEditableAsset editableAsset = ctx.CreateEditableAsset(*m_pAsset);
		editableAsset.SetWorkFiles(m_workFiles);
		if (editableAsset.WriteToFile() && CVersionControl::GetInstance().IsOnline())
		{
			AddNewFilesToVCS();
		}
	}

private:
	void AddNewFilesToVCS()
	{
		std::vector<string> filesToAdd;
		for (int i = 0; i < m_isAdded.size(); ++i)
		{
			if (m_isAdded[i])
			{
				auto fs = CVersionControl::GetInstance().GetFileStatus(m_workFiles[i]);
				if (!fs || fs->HasState(CVersionControlFileStatus::eState_NotTracked))
				{
					filesToAdd.push_back(m_workFiles[i]);
					CDeletedWorkFilesStorage::GetInstance().Remove(m_workFiles[i]);
				}
			}
		}
		VersionControlPathUtils::MatchCaseAndRemoveUnmatched(filesToAdd);
		if (!filesToAdd.empty())
		{
			CVersionControl::GetInstance().AddFiles(std::move(filesToAdd));
		}
	}

	void ValidateFiles(std::vector<string>& workFiles)
	{
		auto it = std::partition(workFiles.begin(), workFiles.end(), [](const string& file)
		{
			return IsValidWorkFile(file);
		});
		if (it != workFiles.end())
		{
			auto delIt = it;
			do
			{
				CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING
					, "File %s can't be added as work file because it is a part of some asset", *it);
			} while (++it != workFiles.end());
			workFiles.erase(delIt, workFiles.end());
		}
	}

	CAsset* m_pAsset;
	std::vector<string> m_workFiles;
	std::vector<int>    m_isAdded;
};

CWorkFilesModel* GetModel(QAbstractItemView* pView)
{
	QAttributeFilterProxyModel* pSortModel = static_cast<QAttributeFilterProxyModel*>(pView->model());
	return static_cast<CWorkFilesModel*>(pSortModel->sourceModel());
}

class CSelfDeletableItemDelegate : public QStyledItemDelegate
{
public:
	CSelfDeletableItemDelegate(QAbstractItemView* pParent)
		: QStyledItemDelegate(pParent)
		, m_pView(pParent)
	{}

	void paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		QStyledItemDelegate::paint(pPainter, option, index);

		const bool isOver = option.state & QStyle::State_MouseOver;
		if (!isOver)
		{
			return;
		}

		static const QSize iconSize = QSize(24, 24);

		const QPixmap iconPixmap = QIcon("icons:General/Row_Delete.ico").pixmap(iconSize);
		const QRect centerRect = QRect(option.rect.right() - iconSize.width() - 2,
			option.rect.top() + option.rect.height() / 2 - iconSize.height() / 2, iconPixmap.width(), iconPixmap.height());

		if (IsDeleteHovered())
		{
			pPainter->drawPixmap(centerRect, iconPixmap);
		}
		else
		{
			QPixmap normal = QPixmap(iconSize);
			normal.fill(Qt::transparent);
			QPainter p(&normal);
			p.fillRect(normal.rect(), GetStyleHelper()->iconTint());
			p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
			p.drawPixmap(QPoint(0, 0), iconPixmap);
			pPainter->drawPixmap(centerRect.topLeft(), normal);
		}
	}

	bool editorEvent(QEvent* pEvent, QAbstractItemModel* pModel, const QStyleOptionViewItem& option, const QModelIndex& index)
	{
		if (pEvent->type() == QEvent::MouseButtonRelease)
		{
			if (IsDeleteHovered())
			{
				pModel->setData(index, 0, DELETE_ROW_ROLE);
			}
			return true;
		}
		return QAbstractItemDelegate::editorEvent(pEvent, pModel, option, index);
	}

	void SetDeleteHovered(bool isHovered) { m_isDeleteHovered = isHovered; }

	bool IsDeleteHovered() const { return m_isDeleteHovered; }

private:
	QAbstractItemView* m_pView;
	bool m_isDeleteHovered{ false };
};

class CRowSelfDeletableTreeView : public QTreeView
{

public:
	CRowSelfDeletableTreeView(QWidget* pParent, int lastColumnIndex = 0)
		: QTreeView(pParent)
	{
		m_itemDelegate = new CSelfDeletableItemDelegate(this);
		setItemDelegateForColumn(lastColumnIndex, m_itemDelegate);

		setMouseTracking(true);
	}

	void mouseMoveEvent(QMouseEvent* pEvent) override
	{
		const QModelIndex& index = indexAt(pEvent->pos());
		if (index.isValid())
		{
			bool isOverDelete = pEvent->pos().x() > width() - 42;
			if (m_itemDelegate->IsDeleteHovered() != isOverDelete)
			{
				m_itemDelegate->SetDeleteHovered(isOverDelete);
				update(index);
			}
		}
	}

private:
	CSelfDeletableItemDelegate* m_itemDelegate{ nullptr };
};

bool OpenSelectFilesDialog(std::vector<string>& outputFiles, const string& filter, const string& initialDir
	, const string& baseDir)
{
	CEngineFileDialog::OpenParams dialogParams(CEngineFileDialog::OpenMultipleFiles);
	dialogParams.initialDir = QtUtil::ToQString(initialDir);
	dialogParams.extensionFilters = CExtensionFilter::Parse(filter);
	dialogParams.baseDirectory = baseDir;
	CEngineFileDialog fileDialog(dialogParams);
	if (fileDialog.exec() == QDialog::Accepted)
	{
		auto files = fileDialog.GetSelectedFiles();
		outputFiles.clear();
		outputFiles.reserve(files.size());
		for (const auto& file : files)
		{
			outputFiles.push_back(QtUtil::ToString(file));
		}
		return true;
	}
	return false;
}

}

void CManageWorkFilesDialog::ShowWindow(CAsset* pAsset)
{
	CManageWorkFilesDialog popup(pAsset);
	popup.Execute();
}

CManageWorkFilesDialog::CManageWorkFilesDialog(CAsset* pAsset, QWidget* pParent /*= nullptr*/)
	: CEditorDialog(QStringLiteral("CVersionControlSubmissionPopup"), pParent)
	, m_pAsset(pAsset)
{
	using namespace Private_DefineWorkFilesWindow;

	SetTitle(tr(QString("Manage Work Files (%1)").arg(pAsset->GetName().c_str()).toLocal8Bit().data()));
	auto pLayout = new QVBoxLayout();
	pLayout->setMargin(4);
	setLayout(pLayout);

	auto pAddFileButton = new QPushButton(tr("Add File(s)..."));
	QObject::connect(pAddFileButton, &QPushButton::clicked, this, &CManageWorkFilesDialog::OnAddWorkFile);
	pAddFileButton->setFixedHeight(36);
	pLayout->addWidget(pAddFileButton);

	auto pModel = new CWorkFilesModel(pAsset);
	auto pSortModel = new QAttributeFilterProxyModel(QAttributeFilterProxyModel::BaseBehavior, this);
	pSortModel->setSourceModel(pModel);

	m_pTree = new CRowSelfDeletableTreeView(this, 2);
	m_pTree->setModel(pSortModel);
	m_pTree->setSelectionMode(QAbstractItemView::NoSelection);
	m_pTree->setSortingEnabled(true);
	m_pTree->sortByColumn(0, Qt::AscendingOrder);
	m_pTree->setBaseSize(460, 280);
	m_pTree->setMinimumWidth(240);
	m_pTree->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
	m_pTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

	pLayout->addWidget(m_pTree);

	auto pCancelButton = new QPushButton(tr("Cancel"));
	m_pSaveButton = new QPushButton(tr("Save"));
	QObject::connect(pCancelButton, &QPushButton::clicked, this, [this] { close(); });
	QObject::connect(m_pSaveButton, &QPushButton::clicked, this, &CManageWorkFilesDialog::OnSave);
	pCancelButton->setFixedWidth(120);
	m_pSaveButton->setFixedWidth(190);

	m_pWarningWidget = new QWidget(this);
	auto pWarningLayout = new QHBoxLayout();

	auto pLabel = new QLabel(this);
	pLabel->setPixmap(QPixmap("icons:General/Warning.ico"));
	pWarningLayout->addWidget(pLabel);

	m_pWarningText = new QLabel(this);
	pWarningLayout->addWidget(m_pWarningText);

	m_pWarningWidget->setLayout(pWarningLayout);
	m_pWarningWidget->hide();

	auto pButtonLayout = new QHBoxLayout();
	pButtonLayout->setMargin(0);
	pButtonLayout->addWidget(m_pWarningWidget);
	pButtonLayout->addStretch();
	pButtonLayout->addWidget(pCancelButton);
	pButtonLayout->addWidget(m_pSaveButton);
	pLayout->addLayout(pButtonLayout);

	CheckAssetsStatus();

	setLayout(pLayout);
}

void CManageWorkFilesDialog::OnSave()
{
	using namespace Private_DefineWorkFilesWindow;
	GetModel(m_pTree)->SaveAsset();
	close();
}

void CManageWorkFilesDialog::OnAddWorkFile()
{
	using namespace Private_DefineWorkFilesWindow;

	std::vector<string> files;
	if (OpenSelectFilesDialog(files, "All Files (*.*)|*.*|", m_pAsset->GetFolder(), PathUtil::GetGameProjectAssetsPath()))
	{
		// given files are already relative to assets root
		GetModel(m_pTree)->AddWorkFiles(std::move(files));
	}
}

void CManageWorkFilesDialog::CheckAssetsStatus()
{
	if (CVersionControl::GetInstance().IsOnline())
	{
		const auto fs = CVersionControl::GetInstance().GetFileStatus(m_pAsset->GetMetadataFile());
		if (fs)
		{
			if (!fs->HasState(CVersionControlFileStatus::eState_AddedLocally | CVersionControlFileStatus::eState_CheckedOutLocally))
			{
				m_pWarningText->setText("The asset is not checked out");
				m_pWarningWidget->show();
				m_pSaveButton->setDisabled(true);
				return;
			}
		}
	}
	QFileInfo info(QtUtil::ToQString(PathUtil::Make(PathUtil::GetGameProjectAssetsRelativePath(), m_pAsset->GetMetadataFile())));
	if (!info.isWritable())
	{
		m_pWarningText->setText(tr("The asset is read-only"));
		m_pWarningWidget->show();
	}
}
