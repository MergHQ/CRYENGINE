// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Controls/EditorDialog.h>
#include <ACETypes.h>

class QAdvancedTreeView;
class QDeepFilterProxyModel;
class QDialogButtonBox;
class CMountingProxyModel;
class QAbstractItemModel;
class QModelIndex;

namespace ACE
{
class CAudioAssetsManager;
class CResourceControlModel;
class CResourceLibraryModel;
class CResourceFilterProxyModel;

class CResourceSelectorDialog final : public CEditorDialog
{
	Q_OBJECT

public:

	CResourceSelectorDialog(QWidget* pParent, EItemType const eType);
	~CResourceSelectorDialog();

private slots:

	void UpdateSelectedControl();
	void SetTextFilter(QString const&filter);
	void StopTrigger();
	void ItemDoubleClicked(QModelIndex const& modelIndex);
	void OnContextMenu(QPoint const& pos);

public:

	void        SetScope(Scope const scope);
	const char* ChooseItem(const char* currentValue);
	QSize       sizeHint() const override;

private:

	QModelIndex         FindItem(string const& sControlName);
	void                ApplyFilter();
	bool                ApplyFilter(QModelIndex const& parent);
	bool                IsValid(QModelIndex const& index);
	QAbstractItemModel* CreateLibraryModelFromIndex(QModelIndex const& sourceIndex);

	// QDialog
	virtual bool eventFilter(QObject* pObject, QEvent* pEvent) override;
	// ~QDialog

	// Filtering
	QString                             m_sFilter;
	EItemType                           m_eType;
	Scope                               m_scope;
	bool                                m_bSelectionIsValid = false;

	static string                       s_previousControlName;
	static EItemType                    s_previousControlType;
	QAdvancedTreeView*                  m_pControlsTree;
	QDialogButtonBox*                   m_pDialogButtons;

	CAudioAssetsManager*                m_pAssetsManager;
	CResourceFilterProxyModel*          m_pFilterProxyModel;
	CResourceControlModel*              m_pAssetsModel;
	CMountingProxyModel*                m_pMountingProxyModel;
	std::vector<CResourceLibraryModel*> m_libraryModels;
};
} // namespace ACE
