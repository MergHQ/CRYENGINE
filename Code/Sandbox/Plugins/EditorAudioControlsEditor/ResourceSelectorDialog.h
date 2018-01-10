// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Controls/EditorDialog.h>
#include <SystemTypes.h>

class QSearchBox;
class QDialogButtonBox;
class CMountingProxyModel;
class QAbstractItemModel;
class QModelIndex;

namespace ACE
{
class CTreeView;
class CSystemAsset;
class CSystemAssetsManager;
class CResourceSourceModel;
class CResourceLibraryModel;
class CResourceFilterProxyModel;

class CResourceSelectorDialog final : public CEditorDialog
{
	Q_OBJECT

public:

	CResourceSelectorDialog(ESystemItemType const type, Scope const scope, QWidget* const pParent);
	~CResourceSelectorDialog();

	char const*   ChooseItem(char const* currentValue);

	// QDialog
	virtual QSize sizeHint() const override;
	// ~QDialog

private slots:

	void OnUpdateSelectedControl();
	void OnStopTrigger();
	void OnItemDoubleClicked(QModelIndex const& modelIndex);
	void OnContextMenu(QPoint const& pos);

private:

	QModelIndex         FindItem(string const& sControlName);
	QAbstractItemModel* CreateLibraryModelFromIndex(QModelIndex const& sourceIndex);
	void                DeleteModels();

	// QDialog
	virtual bool        eventFilter(QObject* pObject, QEvent* pEvent) override;
	// ~QDialog

	ESystemItemType const               m_type;
	Scope const                         m_scope;
	bool                                m_selectionIsValid = false;

	CSystemAssetsManager* const         m_pAssetsManager;
	QSearchBox* const                   m_pSearchBox;
	CTreeView* const                    m_pTreeView;
	QDialogButtonBox* const             m_pDialogButtons;
	CResourceFilterProxyModel* const    m_pFilterProxyModel;
	CMountingProxyModel*                m_pMountingProxyModel;
	CResourceSourceModel* const         m_pSourceModel;
	std::vector<CResourceLibraryModel*> m_libraryModels;

	static string                       s_previousControlName;
	static ESystemItemType              s_previousControlType;
};
} // namespace ACE
