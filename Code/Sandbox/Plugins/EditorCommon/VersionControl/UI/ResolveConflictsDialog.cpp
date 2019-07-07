// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "VersionControl/UI/ResolveConflictsDialog.h"
#include "QtUtil.h"
#include <QVBoxLayout>
#include <QTreeView>
#include <QAbstractTableModel>
#include <QApplication>
#include <QPushButton>

namespace Private_ResolveConflict
{

class QResolveConflictsModel : public QAbstractTableModel
{
public:
	QResolveConflictsModel(QWidget* parent, const QStringList& conflictFiles, std::vector<SVersionControlFileConflictStatus>& resolvedFiles);
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex& index, const QVariant& value, int role) override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
	std::vector<SVersionControlFileConflictStatus>& m_resolvedFiles;
};

QResolveConflictsModel::QResolveConflictsModel(QWidget* parent, const QStringList& conflictFiles, std::vector<SVersionControlFileConflictStatus>& resolvedFiles)
	: QAbstractTableModel(parent)
	, m_resolvedFiles(resolvedFiles)
{
	for (const QString& file : conflictFiles)
	{
		m_resolvedFiles.push_back(std::make_pair(QtUtil::ToString(file), EConflictResolution::None));
	}

	dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
}

Qt::ItemFlags QResolveConflictsModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags flags = QAbstractTableModel::flags(index);
	if (index.isValid() && (index.column() == 1 || index.column() == 2))
	{
		flags |= Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
	}

	return flags;
}

int QResolveConflictsModel::rowCount(const QModelIndex& parent) const
{
	if (!parent.isValid())
	{
		return static_cast<int>(m_resolvedFiles.size());
	}
	return 0;
}

int QResolveConflictsModel::columnCount(const QModelIndex& parent) const
{
	if (!parent.isValid())
	{
		return 3;
	}
	return 0;
}

QVariant QResolveConflictsModel::data(const QModelIndex& index, int role) const
{
	int row = index.row();
	int col = index.column();

	if (m_resolvedFiles.size() >= row)
	{
		switch (role)
		{
		case Qt::DisplayRole:
			if (col == 0)
			{
				return QtUtil::ToQString(m_resolvedFiles[row].first);
			}
			break;

		case Qt::CheckStateRole:
			if (col == 1 && m_resolvedFiles[row].second == EConflictResolution::Ours)
			{
				return Qt::Checked;
			}
			else if (col == 2 && m_resolvedFiles[row].second == EConflictResolution::Their)
			{
				return Qt::Checked;
			}
			else if (col != 0)
			{
				return Qt::Unchecked;
			}
		}
	}
	return QVariant();
}

bool QResolveConflictsModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	int row = index.row();
	int col = index.column();

	if (m_resolvedFiles.size() >= row)
	{
		//const Qt::CheckState checkState = static_cast<Qt::CheckState> (value.toInt());
		//const bool isChecked = checkState == Qt::Checked;

		if (col != 0)
		{
			QModelIndex leftIndex = index.model()->index(index.row(), 1, index.parent());
			QModelIndex rightIndex = index.model()->index(index.row(), 2, index.parent());
			m_resolvedFiles[row].second = col == 1 ? EConflictResolution::Ours : EConflictResolution::Their;
			dataChanged(leftIndex, rightIndex);
			return true;
		}
	}
	return false;
}

QVariant QResolveConflictsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal)
	{
		if (role == Qt::DisplayRole)
		{
			switch (section)
			{
			case 0:
				return QString("Files");
			case 1:
				return QString("Use Local File");
			case 2:
				return QString("Use Server File");
			}
		}
	}

	return QVariant();
}

}

CResolveConflictsDialog::CResolveConflictsDialog(const QStringList& conflictFiles, QWidget* parent)
	: QDialog(parent ? parent : QApplication::activeWindow())
{
	using namespace Private_ResolveConflict;
	QVBoxLayout* layout = new QVBoxLayout();
	layout->setContentsMargins(1, 1, 1, 1);
	setLayout(layout);

	QResolveConflictsModel* pResolveFilesModel = new QResolveConflictsModel(this, conflictFiles, m_conflictFiles);
	m_pFileListView = new QTreeView(this);

	m_pFileListView->setModel(pResolveFilesModel);
	layout->addWidget(m_pFileListView);

	QPushButton* button = new QPushButton("Resolve");
	layout->addWidget(button);
	connect(button, SIGNAL(clicked()), this, SLOT(onButtonClicked()));
}

void CResolveConflictsDialog::onButtonClicked()
{
	resolve(m_conflictFiles);
	close();
}
