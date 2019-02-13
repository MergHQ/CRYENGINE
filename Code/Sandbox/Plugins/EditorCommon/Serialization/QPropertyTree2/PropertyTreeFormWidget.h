// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include <QLayout>
#include <QRect>

#include "PropertyTreeModel.h"
#include "PropertyTree2.h"

namespace PropertyTree2
{
//This widget container is in charge of laying out the widgets in the tree and drawing the extra elements
//This also handles interaction with the tree's UI elements (expanding/collapsing, slider...)
class CFormWidget : public QWidget
{
	Q_OBJECT

	//Styling properties
	Q_PROPERTY(int nesting READ GetNesting)
	Q_PROPERTY(int spacing READ GetSpacing WRITE SetSpacing DESIGNABLE true)
	Q_PROPERTY(int minRowHeight READ GetMinRowHeight WRITE SetMinRowHeight DESIGNABLE true)
	Q_PROPERTY(int iconSize READ GetIconSize WRITE SetIconSize DESIGNABLE true)
	Q_PROPERTY(int nestingIndent READ GetNestingIndent WRITE SetNestingIndent DESIGNABLE true)
	Q_PROPERTY(int groupBorderWidth READ GetGroupBorderWidth WRITE SetGroupBorderWidth DESIGNABLE true)
	Q_PROPERTY(QColor groupBorderColor READ GetGroupBorderColor WRITE SetGroupBorderColor DESIGNABLE true)
	Q_PROPERTY(QRect margins READ GetMargins WRITE SetMargins DESIGNABLE true)

public:
	CFormWidget(QPropertyTree2* pParentTree, const CRowModel* pRow, int nesting);
	~CFormWidget();

	//Styling
	int              GetNesting() const       { return m_nesting; }

	int              GetNestingIndent() const { return m_nestingIndent; }
	void             SetNestingIndent(int value);

	int              GetSpacing() const { return m_spacing; }
	void             SetSpacing(int value);

	int              GetMinRowHeight() const { return m_minRowHeight; }
	void             SetMinRowHeight(int value);

	int              GetIconSize() const { return m_iconSize; }
	void             SetIconSize(int value);

	int              GetGroupBorderWidth() const { return m_groupBorderWidth; }
	void             SetGroupBorderWidth(int value);

	QColor           GetGroupBorderColor() const { return m_groupBorderColor; }
	void             SetGroupBorderColor(const QColor& color);

	const CRowModel* GetRowModel() const { return m_pRowModel; }
	void             OnActiveRowChanged(const CRowModel* pNewActiveRow);

	QRect            GetMargins() const;
	void             SetMargins(const QRect& margins);

	void             OnSplitterPosChanged();

	//! Expands a row in this form's scope. Returns the formWidget child of the row
	CFormWidget* ExpandRow(const CRowModel* pRow, bool expand);

	//! Recursively expands to the specified row. Returns the formWidget that contains the row
	CFormWidget* ExpandToRow(const CRowModel* pRow);

	//! Scrolls the parent scroll area to make this row visible
	void ScrollToRow(const CRowModel* pRow);

	//Updates the tree if the model is marked dirty
	void UpdateTree();

private:
	bool  eventFilter(QObject* pWatched, QEvent* pEvent) override;
	void  paintEvent(QPaintEvent* pEvent) override;
	void  resizeEvent(QResizeEvent* event) override;
	bool  event(QEvent* pEvent) override;
	void  mousePressEvent(QMouseEvent* pEvent) override;
	void  mouseReleaseEvent(QMouseEvent* pEvent) override;
	void  mouseMoveEvent(QMouseEvent* pEvent) override;
	void  enterEvent(QEvent* pEvent) override;
	void  leaveEvent(QEvent* pEvent) override;
	void  contextMenuEvent(QContextMenuEvent* pEvent) override;
	void  dragEnterEvent(QDragEnterEvent* pEvent) override;
	void  dragMoveEvent(QDragMoveEvent* pEvent) override;
	void  dragLeaveEvent(QDragLeaveEvent* pEvent) override;
	void  dropEvent(QDropEvent* pEvent) override;
	void  keyPressEvent(QKeyEvent* pEvent) override;

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;
	bool  focusNextPrevChild(bool next) override;

	void  SetupWidgets();
	void  ReleaseWidgets();

	void  UpdateMinimumSize();
	void  DoLayout(const QSize& rect);
	void  DoFullLayoutUpdate();

	void  OnCopy();
	void  OnPaste();

	void  RecursiveInstallEventFilter(QWidget* pWidget);

	struct FormRow
	{
		FormRow(const CRowModel* pRowModel);
		FormRow(FormRow&& other);
		FormRow(const FormRow& other) = delete;
		~FormRow();

		FormRow& operator=(FormRow&& other);
		FormRow& operator=(const FormRow& other) = delete;

		_smart_ptr<const CRowModel> m_pModel;
		CFormWidget*                m_pChildContainer;
		QWidget*                    m_pWidget; //Widget that is actually displayed on this row, may differ from model's widget due to inlining behavior

		//Cached rowRect for paint event (paint labels, hover highlight and so on)
		QRect                rowRect;

		bool                 IsExpanded() const;
		QWidget*             GetQWidget();
		IPropertyTreeWidget* GetPropertyTreeWidget();
		bool                 HasChildren() const;
	};

	int      GetSplitterPosition() const;
	FormRow* ModelToFormRow(const CRowModel* pRow);
	FormRow* RowAtPos(const QPoint& position);
	int      InsertionIndexAtPos(const QPoint& position, int tolerancePx = 5);
	int      RowIndex(const FormRow* pRow);

	void     UpdateActiveRow(const QPoint& cursorPos);
	void     SetActiveRow(FormRow* pRow);
	QRect    GetExpandIconRect(const FormRow& row) const;
	QRect    GetDragHandleIconRect(const FormRow& row) const;

	void     ToggleExpand(FormRow& row, bool skipLayout = false);

	//Invariants
	_smart_ptr<const CRowModel> m_pRowModel;
	QPropertyTree2*             m_pParentTree;
	const int                   m_nesting;

	//Internals
	std::vector<FormRow> m_rows;
	FormRow*             m_pActiveRow;
	QPoint               m_lastCursorPos; //Last position tested for hovering. Used to avoid unnecessary computing.
	QPoint               m_mouseDownPos;
	bool                 m_allowReorderChildren;
	int                  m_dragInsertIndex; //-2 = not dragging at all, -1 = dragging but invalid index, >0 = valid index

	//Style props
	int    m_spacing;
	int    m_minRowHeight;
	int    m_iconSize;
	int    m_nestingIndent;
	int    m_groupBorderWidth;
	QColor m_groupBorderColor;
};

class CInlineWidgetBox : public QWidget
{
	Q_OBJECT
public:
	CInlineWidgetBox(CFormWidget* pParent);

	void AddWidget(QWidget* pWidget, QPropertyTree2* pPropertyTree);
	void ReleaseWidgets(QPropertyTree2* pPropertyTree);
};
}
