// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "ManageWorkFilesDialog.h"

#include "AssetSystem/Asset.h"
#include "AssetSystem/AssetImportContext.h"
#include "EditorStyleHelper.h"
#include "FileDialogs/EngineFileDialog.h"
#include "PathUtils.h"
#include "ProxyModels/AttributeFilterProxyModel.h"
#include "QtUtil.h"

#include <QBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTreeView>
#include <QHeaderView>

namespace Private_DefineWorkFilesWindow
{

constexpr int DELETE_ROW_ROLE = Qt::UserRole + 1;

class CWorkFilesModel : public QAbstractItemModel
{
public:
	explicit CWorkFilesModel(CAsset* pAsset)
		: m_pAsset(pAsset)
	{
		m_workFiles = pAsset->GetWorkFiles();
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
		return 2;
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
			}
		}

		return QVariant();
	}

	void AddWorkFiles(const std::vector<string>& workFiles)
	{
		beginResetModel();
		m_workFiles.insert(m_workFiles.end(), workFiles.cbegin(), workFiles.cend());
		endResetModel();
	}

	void DeleteWorkFile(int row)
	{
		beginResetModel();
		m_workFiles.erase(m_workFiles.begin() + row);
		endResetModel();
	}

	void SaveAsset()
	{
		CAssetImportContext ctx;
		CEditableAsset editableAsset = ctx.CreateEditableAsset(*m_pAsset);
		editableAsset.SetWorkFiles(m_workFiles);
		editableAsset.WriteToFile();
	}

private:
	CAsset* m_pAsset;
	std::vector<string> m_workFiles;
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

	CWorkFilesModel* pModel = new CWorkFilesModel(pAsset);
	QAttributeFilterProxyModel* pSortModel = new QAttributeFilterProxyModel(QAttributeFilterProxyModel::BaseBehavior, this);
	pSortModel->setSourceModel(pModel);

	m_pTree = new CRowSelfDeletableTreeView(this, 1);
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
	auto pSaveButton = new QPushButton(tr("Save"));
	QObject::connect(pCancelButton, &QPushButton::clicked, this, [this] { close(); });
	QObject::connect(pSaveButton, &QPushButton::clicked, this, &CManageWorkFilesDialog::OnSave);
	pCancelButton->setFixedWidth(120);
	pSaveButton->setFixedWidth(190);

	auto pButtonLayout = new QHBoxLayout();
	pButtonLayout->setMargin(0);
	pButtonLayout->addStretch();
	pButtonLayout->addWidget(pCancelButton);
	pButtonLayout->addWidget(pSaveButton);
	pLayout->addLayout(pButtonLayout);

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
		GetModel(m_pTree)->AddWorkFiles(files);
	}
}
