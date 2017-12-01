// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ProxyModels/AttributeFilterProxyModel.h>
#include <SystemTypes.h>

class CItemModelAttribute;
class QMimeData;

namespace ACE
{
class CImplItem;
class CSystemControl;
class CSystemLibrary;
class CSystemFolder;
class CSystemAsset;
class CSystemAssetsManager;

namespace SystemModelUtils
{
enum class EColumns
{
	Notification,
	Type,
	Placeholder,
	NoConnection,
	NoControl,
	Scope,
	Name,
	Count,
};

enum class ERoles
{
	ItemType = Qt::UserRole + 1,
	Name,
	InternalPointer,
	Id,
};

CItemModelAttribute* GetAttributeForColumn(EColumns const column);
QVariant             GetHeaderData(int const section, Qt::Orientation const orientation, int const role);

void                 GetAssetsFromIndexesSeparated(QModelIndexList const& list, std::vector<CSystemLibrary*>& outLibraries, std::vector<CSystemFolder*>& outFolders, std::vector<CSystemControl*>& outControls);
void                 GetAssetsFromIndexesCombined(QModelIndexList const& list, std::vector<CSystemAsset*>& outAssets);
CSystemAsset*        GetAssetFromIndex(QModelIndex const& index, int const column);
void                 DecodeImplMimeData(const QMimeData* pData, std::vector<CImplItem*>& outItems);
} // namespace SystemModelUtils

class CSystemSourceModel : public QAbstractItemModel
{
public:

	CSystemSourceModel(CSystemAssetsManager* const pAssetsManager, QObject* const pParent);
	virtual ~CSystemSourceModel() override;

	void DisconnectSignals();

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
	virtual Qt::DropActions supportedDropActions() const override;
	virtual QStringList     mimeTypes() const override;
	// ~QAbstractItemModel

private:

	void ConnectSignals();

	CSystemAssetsManager* const m_pAssetsManager;
	bool                        m_ignoreLibraryUpdates;
};

class CSystemLibraryModel : public QAbstractItemModel
{
public:

	CSystemLibraryModel(CSystemAssetsManager* const pAssetsManager, CSystemLibrary* const pLibrary, QObject* const pParent);
	virtual ~CSystemLibraryModel() override;

	void DisconnectSignals();

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
	virtual QMimeData*      mimeData(QModelIndexList const& indexes) const override;
	virtual Qt::DropActions supportedDropActions() const override;
	virtual QStringList     mimeTypes() const override;
	// ~QAbstractItemModel

private:

	void        ConnectSignals();
	QModelIndex IndexFromItem(CSystemAsset const* pItem) const;

	CSystemAssetsManager* const m_pAssetsManager;
	CSystemLibrary* const       m_pLibrary;
};

class CSystemFilterProxyModel final : public QAttributeFilterProxyModel
{
public:

	CSystemFilterProxyModel(QObject* const pParent);

protected:

	// QAttributeFilterProxyModel
	virtual bool rowMatchesFilter(int sourceRow, QModelIndex const& sourcePparent) const override;
	// ~QAttributeFilterProxyModel

	// QSortFilterProxyModel
	virtual bool lessThan(QModelIndex const& left, QModelIndex const& right) const override;
	// ~QSortFilterProxyModel
};
} // namespace ACE
