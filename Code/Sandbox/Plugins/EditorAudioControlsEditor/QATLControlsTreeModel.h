// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <QStandardItemModel>
#include "ATLControlsModel.h"

class QStandardItem;

namespace ACE
{
class CATLControl;

class QATLTreeModel : public QStandardItemModel, public IATLControlModelListener
{

	friend class CAudioControlsLoader;

public:
	QATLTreeModel();
	~QATLTreeModel();
	void           Initialize(CATLControlsModel* pControlsModel);
	QStandardItem* GetItemFromControlID(CID nID);

	CATLControl*   CreateControl(EACEControlType eControlType, const string& sName, CATLControl* pParent = nullptr);
	QStandardItem* AddControl(CATLControl* pControl, QStandardItem* pParent, int nRow = 0);

	QStandardItem* CreateFolder(QStandardItem* pParent, const string& sName, int nRow = 0);

	void           RemoveItem(QModelIndex index);
	void           RemoveItems(QModelIndexList indexList);

	void           SetItemAsDirty(QStandardItem* pItem);
	bool           IsValidParent(const QModelIndex& parent, const EACEControlType controlType) const;

private:
	void           DeleteInternalData(QModelIndex root);
	CATLControl*   GetControlFromIndex(QModelIndex index);
	QStandardItem* ControlsRootItem();

	// ------------- QStandardItemModel ----------------
	virtual bool canDropMimeData(const QMimeData* pData, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;
	virtual bool dropMimeData(const QMimeData* pData, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
	// -------------------------------------------------------

	// ------------- IATLControlModelListener ----------------
	virtual void OnControlAdded(CATLControl* pControl) override;
	virtual void OnControlModified(CATLControl* pControl) override;
	virtual void OnControlRemoved(CATLControl* pControl) override;
	virtual void OnConnectionAdded(CATLControl* pControl, IAudioSystemItem* pMiddlewareControl) override;
	virtual void OnConnectionRemoved(CATLControl* pControl, IAudioSystemItem* pMiddlewareControl) override;
	// -------------------------------------------------------

	CATLControlsModel* m_pControlsModel;
};
}
