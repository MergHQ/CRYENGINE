// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/FileImportInfo.h"
#include <QAbstractItemModel>
#include <QDir>

namespace ACE
{
class CFileImporterModel final : public QAbstractItemModel
{
	Q_OBJECT

public:

	enum class EColumns : CryAudio::EnumFlagsType
	{
		Source,
		Target,
		Import,
		Count, };

	CFileImporterModel() = delete;
	CFileImporterModel(CFileImporterModel const&) = delete;
	CFileImporterModel(CFileImporterModel&&) = delete;
	CFileImporterModel& operator=(CFileImporterModel const&) = delete;
	CFileImporterModel& operator=(CFileImporterModel&&) = delete;

	explicit CFileImporterModel(FileImportInfos& fileImportInfos, QObject* const pParent);
	virtual ~CFileImporterModel() override = default;

	static QString const s_newAction;
	static QString const s_replaceAction;
	static QString const s_unsupportedAction;
	static QString const s_sameFileAction;

	void             Reset();
	SFileImportInfo& ItemFromIndex(QModelIndex const& index);

signals:

	void SignalActionChanged(Qt::CheckState const isChecked);

protected:
	// QAbstractItemModel
	virtual int             rowCount(QModelIndex const& parent) const override;
	virtual int             columnCount(QModelIndex const& parent) const override;
	virtual QVariant        data(QModelIndex const& index, int role) const override;
	virtual bool            setData(QModelIndex const& index, QVariant const& value, int role) override;
	virtual QVariant        headerData(int section, Qt::Orientation orientation, int role) const override;
	virtual Qt::ItemFlags   flags(QModelIndex const& index) const override;
	virtual QModelIndex     index(int row, int column, QModelIndex const& parent = QModelIndex()) const override;
	virtual QModelIndex     parent(QModelIndex const& index) const override;
	virtual bool            canDropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent) const override;
	virtual bool            dropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent) override;
	virtual Qt::DropActions supportedDragActions() const override;
	// ~QAbstractItemModel

private:

	FileImportInfos& m_fileImportInfos;
	QDir const       m_gameFolder;
};
} // namespace ACE
