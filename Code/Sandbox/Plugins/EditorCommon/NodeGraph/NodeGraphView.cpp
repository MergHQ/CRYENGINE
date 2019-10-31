// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "NodeGraphView.h"

#include "AbstractNodeGraphViewModel.h"
#include "AbstractNodeGraphViewModelItem.h"
#include "NodeGraphViewBackground.h"
#include "ItemCollection.h"

#include "AbstractNodeItem.h"

#include "Commands/QCommandAction.h"
#include "NodeWidget.h"
#include "PinWidget.h"
#include "ConnectionWidget.h"
#include "GroupWidget.h"
#include "CommentWidget.h"
#include "GroupWidgetStyle.h"
#include "AbstractNodeContentWidget.h"
#include "NodeGraphItemPropertiesWidget.h"
#include "NodeGraphUndo.h"
#include "NodeGraphViewStyle.h"
#include "QTrackingTooltip.h"

#include "Controls/DictionaryWidget.h"

#include "QtUtil.h"

#include <IEditor.h>
#include <CryMath/Cry_Math.h>
#include <Commands/ICommandManager.h>
#include <EditorFramework/Events.h>
#include <EditorFramework/BroadcastManager.h>
#include <EditorFramework/InspectorLegacy.h>
#include <CrySerialization/IArchive.h>
#include <CrySerialization/IArchiveHost.h>
#include <CrySerialization/STL.h>
#include <CrySandbox/CrySignal.h>

#include <QCollapsibleFrame.h>
#include <QApplication>
#include <QScrollBar>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QMimeData>
#include <QTextEdit>

namespace CryGraphEditor
{

class CTextEdit : public QTextEdit
{
public:
	CCrySignal<void()> SignalEditingFinished;

public:
	CTextEdit(const CTextWidget& textWidget)
		: m_textWidget(textWidget)
	{
	}

protected:
	virtual void keyPressEvent(QKeyEvent* pEvent) override
	{
		switch (pEvent->key())
		{
		case Qt::Key_Return:
			m_textWidget.SignalTextChanged(toPlainText());
		case Qt::Key_Escape:
			SignalEditingFinished();
			break;
		default:
			QTextEdit::keyPressEvent(pEvent);
		}
	}

	virtual void focusOutEvent(QFocusEvent* pEvent) override
	{
		SignalEditingFinished();
		QTextEdit::focusOutEvent(pEvent);
	}

	virtual void wheelEvent(QWheelEvent* pEvent) override
	{
	}

private:
	const CTextWidget& m_textWidget;

};

CNodeGraphView::CNodeGraphView()
	: QGraphicsView(static_cast<QWidget*>(nullptr))
	, m_pModel(nullptr)
	, m_pStyle(nullptr)
	, m_pContentEditPopup(nullptr)
	, m_pNewConnectionLine(nullptr)
	, m_pResizingBox(nullptr)
	, m_pSelectionBox(nullptr)
	, m_pendingItemPlacementTimer(this)
	, m_operation(eOperation_None)
	, m_action(eAction_None)
	, m_isRecodringUndo(false)
	, m_isTooltipActive(false)
	, m_pNewConnectionBeginPin(nullptr)
	, m_pNewConnectionPossibleTargetPin(nullptr)
	, m_currentScaleFactor(1.0)
	, m_currentScalePercent(100)
{
	setResizeAnchor(AnchorViewCenter);
	setInteractive(true);
	setTransformationAnchor(AnchorUnderMouse);
	setDragMode(QGraphicsView::NoDrag);
	setAlignment(Qt::AlignLeft | Qt::AlignTop);
	setBackgroundBrush(palette().brush(QPalette::Background));
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setContextMenuPolicy(Qt::ContextMenuPolicy::PreventContextMenu);

	m_pScene = new      QGraphicsScene();
	m_pBackground = new CNodeGraphViewBackground();
	m_pScene->addItem(m_pBackground);
	setScene(m_pScene);

	const float workspaceSize = 1024.0f * 128.0f;
	QRectF rect(-workspaceSize, -workspaceSize, workspaceSize * 2.0f, workspaceSize * 2.0f);
	m_pBackground->setGeometry(rect);

	m_pContextMenuContent = new CDictionaryWidget();
	m_pContextMenu = new        QPopupWidget("NodeSearchMenu", m_pContextMenuContent);

	QObject::connect(m_pContextMenuContent, &CDictionaryWidget::OnEntryClicked, this, &CNodeGraphView::OnContextMenuAddNode);
	QObject::connect(m_pContextMenuContent, &CDictionaryWidget::OnHide, this, &CNodeGraphView::OnContextMenuAbort);

	m_pTooltip.reset(new QTrackingTooltip());
	m_pTooltip->SetAutoHide(false);

	// TODO: Implement a proper wrapper around CNodeGraphView, which is inherited from CEditorWidget.
	addAction((QAction*)GetIEditor()->GetICommandManager()->GetCommandAction("general.delete"));
	addAction((QAction*)GetIEditor()->GetICommandManager()->GetCommandAction("general.copy"));
	addAction((QAction*)GetIEditor()->GetICommandManager()->GetCommandAction("general.paste"));
	// ~TODO

	QObject::connect(&m_pendingItemPlacementTimer, &QTimer::timeout, this, &CNodeGraphView::RunPendingItemPlacement);
}

CNodeGraphView::~CNodeGraphView()
{
	DeselectAllItems();

	if (m_pNewConnectionLine)
	{
		m_pScene->removeItem(m_pNewConnectionLine);
		delete m_pNewConnectionLine;
	}

	delete m_pContextMenu;
}

void CNodeGraphView::OnNodeMouseEvent(QGraphicsItem* pSender, SNodeMouseEventArgs& args)
{
	CNodeWidget* pNodeWidget = CNodeGraphViewGraphicsWidget::Cast<CNodeWidget>(pSender);
	if (pNodeWidget == nullptr)
		return;

	switch (args.GetReason())
	{
	case EMouseEventReason::ButtonPress:
		{
			if (args.GetButton() == Qt::MouseButton::LeftButton)
			{
				AbortAction();
				const bool shiftPressed = (args.GetModifiers() & Qt::ShiftModifier) != 0;
				if (pNodeWidget->IsSelected())
				{
					if (shiftPressed)
						DeselectWidget(*pNodeWidget);
				}
				else
				{
					if (!shiftPressed)
						DeselectAllItems();
					SelectWidget(*pNodeWidget);
				}
			}
			else if (args.GetButton() == Qt::MouseButton::RightButton)
			{

			}
		}
		break;
	case EMouseEventReason::ButtonRelease:
		{
			if (args.GetButton() == Qt::MouseButton::LeftButton)
			{
				if (m_action == eAction_ItemMovement || m_action == eAction_ConnectionCreation)
					AbortAction();

				PlaceSelection();
			}
			else if (args.GetButton() == Qt::MouseButton::RightButton)
			{
				AbortAction();

				if (pNodeWidget->IsSelected() && m_selectedWidgets.size() > 1)
				{
					ShowSelectionContextMenu(args.GetScreenPos());
				}
				else
				{
					DeselectAllItems();
					SelectWidget(*pNodeWidget);

					ShowNodeContextMenu(pNodeWidget, args.GetScreenPos());
				}
			}
		}
		break;
	case EMouseEventReason::ButtonPressAndMove:
		{
			if (args.GetButtons() == Qt::MouseButton::LeftButton)
			{
				if (m_action == eAction_ItemMovement || m_action == eAction_None)
				{
					SetAction(eAction_ItemMovement);

					const QPointF delta = args.GetScenePos() - args.GetLastScenePos();
					if (!m_isRecodringUndo)
					{
						m_isRecodringUndo = true;
						GetIEditor()->GetIUndoManager()->Begin();
					}

					MoveSelection(delta);
				}
			}
		}
		break;
	case EMouseEventReason::HoverEnter:
		{
			ShowItemToolTip(pNodeWidget);
		}
		break;
	case EMouseEventReason::HoverMove:
		{
			pNodeWidget->SetHighlighted(true);

			ShowItemToolTip(pNodeWidget);
		}
		break;
	case EMouseEventReason::HoverLeave:
		{
			pNodeWidget->SetHighlighted(false);

			ShowItemToolTip(nullptr);
		}
		break;
	default:
		break;
	}
}

void CNodeGraphView::OnPinMouseEvent(QGraphicsItem* pSender, SPinMouseEventArgs& args)
{
	CPinWidget* pPinWidget = CNodeGraphViewGraphicsWidget::Cast<CPinWidget>(pSender);
	if (pPinWidget == nullptr)
		return;

	switch (args.GetReason())
	{
	case EMouseEventReason::ButtonPress:
		{
			if (m_action == eAction_None)
			{
				if (args.GetButton() == Qt::MouseButton::LeftButton)
				{
					AbortAction();
					BeginCreateConnection(pPinWidget);
				}
			}
		}
		break;
	case EMouseEventReason::ButtonRelease:
		{
			if (m_action == eAction_ConnectionCreation)
			{
				if (args.GetButton() == Qt::MouseButton::LeftButton)
				{
					EndCreateConnection(pPinWidget);
				}
				else
				{
					EndCreateConnection(nullptr);
				}
			}
			else if (m_action == eAction_None)
			{
				if (args.GetButton() == Qt::MouseButton::LeftButton)
				{
					AbortAction();
					BeginCreateConnection(pPinWidget);
				}
			}
		}
		break;
	case EMouseEventReason::ButtonPressAndMove:
		{
			if (m_action == eAction_ConnectionCreation)
			{
				CRY_ASSERT(m_selectedWidgets.size() == 1);
				CPinWidget* pSourcePin = CNodeGraphViewGraphicsWidget::Cast<CPinWidget>(*m_selectedWidgets.begin());
				CRY_ASSERT(pSourcePin);

				if (pSourcePin->GetItem().CanConnect(&pPinWidget->GetItem()))
				{
					pPinWidget->SetHighlighted(true);
					break;
				}
			}
		}
		break;
	case EMouseEventReason::HoverEnter:
		{
			if (m_action == eAction_None)
			{
				pPinWidget->SetHighlighted(true);
				pPinWidget->GetNodeWidget().SetHighlighted(true);
			}

			ShowItemToolTip(pPinWidget);
		}
		break;
	case EMouseEventReason::HoverMove:
		{
			// TODO: Temp solution to fix highlighting when moving mouse from pin icon to the node text.
			if (m_action == eAction_None)
			{
				pPinWidget->SetHighlighted(true);
				pPinWidget->GetNodeWidget().SetHighlighted(true);
			}
			// ~TODO

			ShowItemToolTip(pPinWidget);
		}
		break;
	case EMouseEventReason::HoverLeave:
		{
			pPinWidget->SetHighlighted(false);
			pPinWidget->GetNodeWidget().SetHighlighted(false);

			ShowItemToolTip(nullptr);
		}
		break;
	default:
		break;
	}
}

void CNodeGraphView::OnConnectionMouseEvent(QGraphicsItem* pSender, SConnectionMouseEventArgs& args)
{
	CConnectionWidget* pConnectionWidget = CNodeGraphViewGraphicsWidget::Cast<CConnectionWidget>(pSender);
	if (pConnectionWidget == nullptr)
		return;

	switch (args.GetReason())
	{
	case EMouseEventReason::ButtonPress:
		{
			if (args.GetButton() == Qt::MouseButton::LeftButton)
			{
				if (args.IsInsideShape())
				{
					if (m_action == eAction_None)
					{
						AbortAction();
						const bool shiftPressed = (args.GetModifiers() & Qt::ShiftModifier) != 0;
						if (pConnectionWidget->IsSelected())
						{
							if (shiftPressed)
								DeselectWidget(*pConnectionWidget);
						}
						else
						{
							if (!shiftPressed)
								DeselectAllItems();
							SelectWidget(*pConnectionWidget);
						}
					}
				}
			}
			else if (args.GetButton() == Qt::MouseButton::RightButton)
			{

			}
		}
		break;
	case EMouseEventReason::ButtonRelease:
		{
			if (args.GetButton() == Qt::MouseButton::LeftButton)
			{
				if (m_action == eAction_ItemMovement || m_action == eAction_ConnectionCreation)
					AbortAction();
			}
			else if (args.GetButton() == Qt::MouseButton::RightButton)
			{
				if (pConnectionWidget->IsSelected())
				{
					ShowSelectionContextMenu(args.GetScreenPos());
				}
			}
		}
		break;
	case EMouseEventReason::ButtonPressAndMove:
		{
			if (args.GetButtons() == Qt::MouseButton::LeftButton)
			{
				if (m_action == eAction_ItemMovement || m_action == eAction_None)
				{
					SetAction(eAction_ItemMovement);

					const QPointF delta = args.GetScenePos() - args.GetLastScenePos();
					if (!m_isRecodringUndo)
					{
						m_isRecodringUndo = true;
						GetIEditor()->GetIUndoManager()->Begin();
					}

					MoveSelection(delta);
				}
			}
		}
		break;
	case EMouseEventReason::HoverMove:
		{
			if (args.IsInsideShape())
				pConnectionWidget->SetHighlighted(true);
			else
				pConnectionWidget->SetHighlighted(false);
		}
		break;
	case EMouseEventReason::HoverLeave:
		{
			pConnectionWidget->SetHighlighted(false);
		}
		break;
	default:
		break;
	}
}

void CNodeGraphView::OnGroupMouseEvent(QGraphicsItem* pSender, SGroupMouseEventArgs& args)
{
	CGroupWidget* pGroupWidget = CNodeGraphViewGraphicsWidget::Cast<CGroupWidget>(pSender);
	if (pGroupWidget == nullptr)
		return;

	switch (args.GetReason())
	{
	case EMouseEventReason::ButtonPress:
		{
			if (args.GetButton() == Qt::MouseButton::LeftButton)
			{
				if (m_action == eAction_ItemBoxSelection)
				{
					if ((args.GetModifiers() & Qt::ShiftModifier) == 0)
						DeselectAllItems();
				}
				else
				{
					pGroupWidget->UpdateAABB();

					const bool shiftPressed = (args.GetModifiers() & Qt::ShiftModifier) != 0;
					if (pGroupWidget->IsSelected())
					{
						if (shiftPressed)
							DeselectWidget(*pGroupWidget);
					}
					else
					{
						if (!shiftPressed)
							DeselectAllItems();
						SelectWidget(*pGroupWidget);
					}
				}
			}
		}
		break;
	case EMouseEventReason::ButtonPressAndMove:
		{
			if (args.GetButtons() & Qt::MouseButton::LeftButton)
			{
				if (m_action == eAction_ItemBoxSelection)
				{
					CRY_ASSERT(m_pSelectionBox);
					if (m_pSelectionBox)
					{
						GraphViewWidgetSet oldItems;
						oldItems.swap(m_pSelectionBox->m_items);

						m_pSelectionBox->UpdateContainingSceneItems();
						for (CNodeGraphViewGraphicsWidget* pItem : m_pSelectionBox->GetItems())
						{
							if (oldItems.erase(pItem) == 0)
								pItem->SetHighlighted(true);
						}

						for (CNodeGraphViewGraphicsWidget* pItem : oldItems)
						{
							pItem->SetHighlighted(false);
						}
					}
				}
				else if (m_action == eAction_ItemMovement || m_action == eAction_None)
				{
					SetAction(eAction_ItemMovement);

					const QPointF delta = args.GetScenePos() - args.GetLastScenePos();
					if (!m_isRecodringUndo)
					{
						m_isRecodringUndo = true;
						GetIEditor()->GetIUndoManager()->Begin();
					}

					MoveSelection(delta);
				}
			}
		}
		break;
	case EMouseEventReason::ButtonRelease:
		{
			if (args.GetButton() == Qt::MouseButton::LeftButton)
			{
				if (m_action == eAction_ItemBoxSelection)
				{
					if ((args.GetModifiers() & Qt::ShiftModifier) == 0)
						DeselectAllItems();

					CRY_ASSERT(m_pSelectionBox);
					if (m_pSelectionBox)
					{
						GraphViewWidgetSet widgets;
						widgets.swap(m_pSelectionBox->m_items);
						SelectWidgets(widgets);
					}

					AbortAction();
				}
			}
		}
		break;
	case EMouseEventReason::HoverEnter:
		{
		}
		break;
	case EMouseEventReason::HoverMove:
		{
			if (m_action == eAction_None)
			{
				m_pResizingBox = pGroupWidget;
				SelectMouseCursor(args.GetScenePos());
			}
		}
		break;
	case EMouseEventReason::HoverLeave:
		{
			if (m_pResizingBox)
			{
				m_pResizingBox = nullptr;
				SelectMouseCursor(QPointF());
			}
		}
		break;
	default:
		break;
	}
}

void CNodeGraphView::OnCommentMouseEvent(QGraphicsItem* pSender, SGroupMouseEventArgs& args)
{
	CCommentWidget* pComment = CNodeGraphViewGraphicsWidget::Cast<CCommentWidget>(pSender);
	if (pComment == nullptr)
		return;

	switch (args.GetReason())
	{
	case EMouseEventReason::ButtonPress:
		{
			if (args.GetButton() == Qt::MouseButton::LeftButton)
			{
				const bool shiftPressed = (args.GetModifiers() & Qt::ShiftModifier) != 0;
				if (pComment->IsSelected())
				{
					if (shiftPressed)
						DeselectWidget(*pComment);
				}
				else
				{
					if (!shiftPressed)
						DeselectAllItems();

					SelectWidget(*pComment);
				}
			}
		}
		break;
	case EMouseEventReason::ButtonPressAndMove:
		{
			if (args.GetButtons() & Qt::MouseButton::LeftButton)
			{
				if (m_action == eAction_ItemMovement || m_action == eAction_None)
				{
					SetAction(eAction_ItemMovement);

					const QPointF delta = args.GetScenePos() - args.GetLastScenePos();
					if (!m_isRecodringUndo)
					{
						m_isRecodringUndo = true;
						GetIEditor()->GetIUndoManager()->Begin();
					}

					MoveSelection(delta);
				}
			}
		}
		break;
	case EMouseEventReason::ButtonRelease:
		{
			if (args.GetButton() == Qt::MouseButton::LeftButton)
			{
				if (args.GetButton() == Qt::MouseButton::LeftButton)
				{
					if (m_action == eAction_ItemMovement || m_action == eAction_ConnectionCreation)
						AbortAction();

					PlaceSelection();
				}
			}
		}
		break;
	case EMouseEventReason::HoverMove:
		{

		}
		break;
	case EMouseEventReason::HoverLeave:
		{

		}
		break;
	default:
		break;
	}
}

void CNodeGraphView::SetPosition(QPoint sceenPos)
{
	centerOn(sceenPos.x(), sceenPos.y());
}

QPoint CNodeGraphView::GetPosition() const
{
	const QPointF pos = mapToScene(viewport()->geometry()).boundingRect().center();
	return QPoint(pos.x(), pos.y());
}

void CNodeGraphView::SetZoom(int32 percent)
{
	if (percent <= 100 && percent >= 10)
	{
		const qreal targetScale = (qreal)percent / 100.0;
		const qreal scaleFactor = targetScale / m_currentScaleFactor;

		m_currentScaleFactor *= scaleFactor;
		m_currentScalePercent = percent;

		scale(scaleFactor, scaleFactor);
	}
}

void CNodeGraphView::FitSceneInView()
{
	// Do not take into account the background item.
	m_pScene->removeItem(m_pBackground);

	const auto rc = m_pScene->itemsBoundingRect();
	MAKE_SURE(rc.isValid(), return );

	const auto sx = viewport()->width() / rc.width();
	const auto sy = viewport()->height() / rc.height();
	const int32 percent(100 * std::min(sx, sy));
	const int32 percentClamped(std::max(10, std::min(percent, 100)));
	SetZoom(percentClamped);
	centerOn(rc.center());

	m_pScene->addItem(m_pBackground);
}

void CNodeGraphView::SetModel(CNodeGraphViewModel* pModel)
{
	m_pContextMenuContent->RemoveAllDictionaries();

	DeselectAllItems();
	if (m_pModel)
	{
		QObject::disconnect(m_pModel, 0, this, 0);
	}

	m_pModel = pModel;
	if (m_pModel)
	{
		ReloadItems();

		//const QString editorName = QObject::tr(pModel->GetRuntimeContext().GetTypeName());
		m_copyPasteMimeFormat = QString("text/plain");

		QObject::connect(m_pModel, &CNodeGraphViewModel::SignalInvalidated, this, &CNodeGraphView::ReloadItems);

		QObject::connect(m_pModel, &CNodeGraphViewModel::SignalCreateNode, this, &CNodeGraphView::OnCreateNode);
		QObject::connect(m_pModel, &CNodeGraphViewModel::SignalCreateGroup, this, &CNodeGraphView::OnCreateGroup);
		QObject::connect(m_pModel, &CNodeGraphViewModel::SignalCreateComment, this, &CNodeGraphView::OnCreateComment);
		QObject::connect(m_pModel, &CNodeGraphViewModel::SignalCreateConnection, this, &CNodeGraphView::OnCreateConnection);

		QObject::connect(m_pModel, &CNodeGraphViewModel::SignalRemoveNode, this, &CNodeGraphView::OnRemoveNode);
		QObject::connect(m_pModel, &CNodeGraphViewModel::SignalRemoveGroup, this, &CNodeGraphView::OnRemoveGroup);
		QObject::connect(m_pModel, &CNodeGraphViewModel::SignalRemoveComment, this, &CNodeGraphView::OnRemoveComment);
		QObject::connect(m_pModel, &CNodeGraphViewModel::SignalRemoveConnection, this, &CNodeGraphView::OnRemoveConnection);

		QObject::connect(m_pModel, &CNodeGraphViewModel::SignalRemoveCustomItem, this, &CNodeGraphView::OnRemoveCustomItem);
	}
	else
	{
		ClearItems();
	}

	update();
}

void CNodeGraphView::SetStyle(const CNodeGraphViewStyle* pStyle)
{
	m_pStyle = pStyle;
	if (m_pStyle)
	{
		m_pStyle->ensurePolished();
		m_pBackground->SetStyle(m_pStyle);
	}
}

void CNodeGraphView::customEvent(QEvent* pEvent)
{
	if (pEvent->type() != SandboxEvent::Command)
	{
		QGraphicsView::customEvent(pEvent);
		return;
	}

	CommandEvent* pCommandEvent = static_cast<CommandEvent*>(pEvent);
	const string& command = pCommandEvent->GetCommand();

	if (command == "general.delete")
	{
		pEvent->setAccepted(OnDeleteEvent());
	}
	else if (command == "general.copy")
	{
		pEvent->setAccepted(OnCopyEvent());
	}
	else if (command == "general.paste")
	{
		pEvent->setAccepted(OnPasteEvent());
	}
	else if (command == "general.undo")
	{
		pEvent->setAccepted(OnUndoEvent());
	}
	else if (command == "general.redo")
	{
		pEvent->setAccepted(OnRedoEvent());
	}
	else
		QGraphicsView::customEvent(pEvent);
}

void CNodeGraphView::DeselectAllItems()
{
	if (m_selectedWidgets.size())
	{
		for (QObject* pObject : m_selectedWidgets)
		{
			if (CNodeGraphViewGraphicsWidget* pItem = qobject_cast<CNodeGraphViewGraphicsWidget*>(pObject))
			{
				pItem->SetSelected(false);
				pItem->ResetZValue();
			}
		}

		m_selectedWidgets.clear();
		m_selectedItems.clear();

		BroadcastSelectionChange();
	}
}

void CNodeGraphView::SelectItem(CAbstractNodeGraphViewModelItem& item)
{
	if (SetItemSelectionState(item, true))
	{
		BroadcastSelectionChange();
	}
}

void CNodeGraphView::DeselectItem(CAbstractNodeGraphViewModelItem& item)
{
	if (SetItemSelectionState(item, false))
	{
		BroadcastSelectionChange();
	}

	CRY_ASSERT(m_selectedItems.find(&item) == m_selectedItems.end());
}

void CNodeGraphView::SelectItems(const GraphItemSet& items)
{
	uint32 numSelectedItems = 0;
	for (CAbstractNodeGraphViewModelItem* pItem : items)
	{
		if (pItem)
		{
			numSelectedItems += SetItemSelectionState(*pItem, true);
		}
	}

	if (numSelectedItems)
	{
		BroadcastSelectionChange();
	}
}

void CNodeGraphView::DeselectItems(const GraphItemSet& items)
{
	uint32 numDeselectedItems = 0;
	for (CAbstractNodeGraphViewModelItem* pItem : items)
	{
		if (pItem)
		{
			numDeselectedItems += SetItemSelectionState(*pItem, false);
		}
	}

	if (numDeselectedItems)
	{
		BroadcastSelectionChange();
	}
}

void CNodeGraphView::SelectItems(const GraphItemIds& itemIds)
{
	if (m_pModel)
	{
		uint32 numSelectedItems = 0;
		for (const QVariant& id : itemIds)
		{
			if (CAbstractNodeItem* pNodeItem = m_pModel->GetNodeItemById(id))
			{
				numSelectedItems += SetItemSelectionState(*pNodeItem, true);
			}
			else if (CAbstractConnectionItem* pConnectionItem = m_pModel->GetConnectionItemById(id))
			{
				numSelectedItems += SetItemSelectionState(*pConnectionItem, true);
			}
		}

		if (numSelectedItems)
		{
			BroadcastSelectionChange();
		}
	}
}

void CNodeGraphView::DeselectItems(const GraphItemIds& itemIds)
{
	if (m_pModel)
	{
		uint32 numDeselectedItems = 0;
		for (const QVariant& id : itemIds)
		{
			if (CAbstractNodeItem* pNodeItem = m_pModel->GetNodeItemById(id))
			{
				numDeselectedItems += SetItemSelectionState(*pNodeItem, false);
			}
			else if (CAbstractConnectionItem* pConnectionItem = m_pModel->GetConnectionItemById(id))
			{
				numDeselectedItems += SetItemSelectionState(*pConnectionItem, false);
			}
		}

		if (numDeselectedItems)
		{
			BroadcastSelectionChange();
		}
	}
}

CNodeWidget* CNodeGraphView::GetNodeWidget(const CAbstractNodeItem& node) const
{
	auto result = m_nodeWidgetByItemInstance.find(&node);
	if (result != m_nodeWidgetByItemInstance.end())
	{
		CNodeWidget* pNodeWidget = result->second;
		const bool isSame = pNodeWidget->GetItem().IsSame(node);
		CRY_ASSERT(isSame);
		if (isSame)
			return pNodeWidget;
	}
	return nullptr;
}

CCommentWidget* CNodeGraphView::GetCommentWidget(const CAbstractCommentItem& comment) const
{
	auto result = m_commentWidgetByItemInstance.find(&comment);
	if (result != m_commentWidgetByItemInstance.end())
	{
		CCommentWidget* pCommentWidget = result->second;
		const bool isSame = pCommentWidget->GetAbstractItem()->IsSame(comment);
		CRY_ASSERT(isSame);
		if (isSame)
			return pCommentWidget;
	}
	return nullptr;
}

CConnectionWidget* CNodeGraphView::GetConnectionWidget(const CAbstractConnectionItem& connection) const
{
	auto result = m_connectionWidgetByItemInstance.find(&connection);
	if (result != m_connectionWidgetByItemInstance.end())
	{
		CConnectionWidget* pConnectionWidget = result->second;
		const bool isSame = pConnectionWidget->GetItem() ? pConnectionWidget->GetItem()->IsSame(connection) : false;
		CRY_ASSERT(isSame);
		if (isSame)
			return pConnectionWidget;
	}
	return nullptr;
}

CPinWidget* CNodeGraphView::GetPinWidget(const CAbstractPinItem& pin) const
{
	CNodeWidget* pNodeWidget = GetNodeWidget(pin.GetNodeItem());
	if (pNodeWidget)
	{
		CAbstractNodeContentWidget* pContentWidget = pNodeWidget->GetContentWidget();
		if (pContentWidget)
		{
			for (CPinWidget* pPinWidget : pContentWidget->GetPinWidgets())
			{
				if (pPinWidget->GetAbstractItem()->IsSame(pin))
				{
					return pPinWidget;
				}
			}
		}
	}
	return nullptr;
}

void CNodeGraphView::LinkItemToGroup(CGroupWidget& group, CNodeGraphViewGraphicsWidget& item)
{
	CAbstractGroupItem* pGroupNode = static_cast<CAbstractGroupItem*>(group.GetAbstractItem());
	CRY_ASSERT_MESSAGE(pGroupNode, "CGroupWidget without group item.");
	if (!pGroupNode)
	{
		return;
	}

	CAbstractNodeGraphViewModelItem* pItemNode = item.GetAbstractItem();
	if ((pItemNode->GetType() != eItemType_Node) && (pItemNode->GetType() != eItemType_Comment))
	{
		return;
	}

	CRY_ASSERT_MESSAGE(pItemNode->GetAcceptsGroup() == false, "Item already attached to group");
	if (!pItemNode->GetAcceptsGroup())
	{
		return;
	}

	m_groupWidgetByItemInstance[pItemNode] = &group;
	group.GetItems().insert(&item);
	pGroupNode->LinkItem(*pItemNode);
}

void CNodeGraphView::UnlinkItemFromGroup(CGroupWidget& group, CNodeGraphViewGraphicsWidget& item)
{
	CAbstractGroupItem* pGroupNode = static_cast<CAbstractGroupItem*>(group.GetAbstractItem());
	CRY_ASSERT_MESSAGE(pGroupNode, "CGroupWidget without group item.");
	if (!pGroupNode)
	{
		return;
	}

	CAbstractNodeGraphViewModelItem* pItemNode = item.GetAbstractItem();

	pGroupNode->UnlinkItem(*pItemNode);
	group.GetItems().erase(&item);
	m_groupWidgetByItemInstance[pItemNode] = nullptr;
}

void CNodeGraphView::SelectWidget(CNodeGraphViewGraphicsWidget& itemWidget)
{
	if (SetWidgetSelectionState(itemWidget, true))
	{
		BroadcastSelectionChange();
	}
}

void CNodeGraphView::DeselectWidget(CNodeGraphViewGraphicsWidget& itemWidget)
{
	if (SetWidgetSelectionState(itemWidget, false))
	{
		BroadcastSelectionChange();
	}

	CRY_ASSERT(m_selectedWidgets.find(&itemWidget) == m_selectedWidgets.end());
	CRY_ASSERT(m_selectedItems.find(itemWidget.GetAbstractItem()) == m_selectedItems.end());
}

void CNodeGraphView::SelectWidgets(const GraphViewWidgetSet& widgets)
{
	uint32 numSelectedItems = 0;
	for (CNodeGraphViewGraphicsWidget* pWidget : widgets)
	{
		if (CAbstractNodeGraphViewModelItem* pItem = pWidget->GetAbstractItem())
		{
			numSelectedItems += SetItemSelectionState(*pItem, true);
		}
	}

	if (numSelectedItems)
	{
		BroadcastSelectionChange();
	}
}

void CNodeGraphView::DeselectWidgets(const GraphViewWidgetSet& widgets)
{
	uint32 numDeselectedItems = 0;
	for (CNodeGraphViewGraphicsWidget* pWidget : widgets)
	{
		if (CAbstractNodeGraphViewModelItem* pItem = pWidget->GetAbstractItem())
		{
			numDeselectedItems += SetItemSelectionState(*pItem, false);
		}
	}

	if (numDeselectedItems)
	{
		BroadcastSelectionChange();
	}
}

bool CNodeGraphView::SetItemSelectionState(CAbstractNodeGraphViewModelItem& item, bool setSelected)
{
	CNodeGraphViewGraphicsWidget* pViewWidget = nullptr;
	if (setSelected)
	{
		const int32 type = item.GetType();
		switch (type)
		{
		case eItemType_Node:
			{
				if (CNodeWidget* pNodeWidget = GetNodeWidget(static_cast<CAbstractNodeItem&>(item)))
					pViewWidget = pNodeWidget;
			}
			break;
		case eItemType_Comment:
			{
				if (CCommentWidget* pCommentWidget = GetCommentWidget(static_cast<CAbstractCommentItem&>(item)))
					pViewWidget = pCommentWidget;
			}
			break;
		case eItemType_Connection:
			{
				if (CConnectionWidget* pConnectionWidget = GetConnectionWidget(static_cast<CAbstractConnectionItem&>(item)))
					pViewWidget = pConnectionWidget;
			}
			break;
		case eItemType_Pin:
			{
				if (CPinWidget* pPinWidget = GetPinWidget(static_cast<CAbstractPinItem&>(item)))
					pViewWidget = pPinWidget;
			}
			break;
		}

		if (type >= eItemType_UserType)
		{
			if (CNodeGraphViewGraphicsWidget* pCustomWidget = GetCustomWidget(item))
				pViewWidget = pCustomWidget;
		}
	}
	else
	{
		for (CNodeGraphViewGraphicsWidget* pSelectedWidget : m_selectedWidgets)
		{
			if (pSelectedWidget->GetAbstractItem() == &item)
			{
				pViewWidget = pSelectedWidget;
			}
		}
	}

	if (pViewWidget)
	{
		return SetWidgetSelectionState(*pViewWidget, setSelected);
	}

	return false;
}

bool CNodeGraphView::SetWidgetSelectionState(CNodeGraphViewGraphicsWidget& itemWidget, bool setSelected)
{
	if (setSelected)
	{
		if (!IsSelected(&itemWidget))
		{
			CAbstractNodeGraphViewModelItem* pGraphItem = itemWidget.GetAbstractItem();
			if (pGraphItem && pGraphItem->GetAcceptsSelection())
			{
				m_selectedWidgets.insert(&itemWidget);
				m_selectedItems.insert(pGraphItem);

				itemWidget.SetSelected(true);
				itemWidget.SetHighlighted(false);

				switch (itemWidget.GetType())
				{
				case eGraphViewWidgetType_NodeWidget:
					itemWidget.setZValue(itemWidget.GetDefaultZValue() + 0.1);
					break;
				case eGraphViewWidgetType_ConnectionWidget:
					itemWidget.setZValue(itemWidget.GetDefaultZValue() + 0.01);
					break;
				case eGraphViewWidgetType_PinWidget:   // Fall through
				default:
					break;
				}

				return true;
			}
		}
	}
	else
	{
		if (itemWidget.IsSelected())
		{
			if (CAbstractNodeGraphViewModelItem* pAbstractItem = itemWidget.GetAbstractItem())
			{
				m_selectedWidgets.erase(&itemWidget);
				m_selectedItems.erase(pAbstractItem);

				itemWidget.SetSelected(false);
				itemWidget.SetHighlighted(false);
				itemWidget.ResetZValue();

				return true;
			}
		}
	}

	return false;
}

QWidget* CNodeGraphView::CreatePropertiesWidget(GraphItemSet& selectedItems)
{
	return new CNodeGraphItemPropertiesWidget(selectedItems);
}

void CNodeGraphView::BroadcastSelectionChange(bool forceClear)
{
	// TODO: This should actually never be called during this operation.
	//			 Add assert as soon as all cases are fixed.
	if (m_operation == eOperation_DeleteSelected)
		return;
	// ~TODO

	if (CBroadcastManager* pBroadcastManager = CBroadcastManager::Get(this))
	{
		if (!forceClear)
		{
			PopulateLegacyInspectorEvent popEvent([this](CInspectorLegacy& inspector)
				{
					if (m_selectedItems.size() > 0)
					{
					  if (QWidget* pPropertiesWidget = CreatePropertiesWidget(m_selectedItems))
					  {
					    QString frameName;
					    if (m_selectedItems.size() == 1)
					    {
					      CAbstractNodeGraphViewModelItem* pFirstItem = *(m_selectedItems.begin());
					      if (CAbstractNodeItem* pNodeItem = pFirstItem->Cast<CAbstractNodeItem>())
					      {
					        frameName = pNodeItem->GetName();
					      }

					      if (frameName.isEmpty())
					      {
					        frameName = QString("%1 Item").arg(m_selectedItems.size());
					      }
					    }
					    else
					    {
					      frameName = QString("%1 Items").arg(m_selectedItems.size());
					    }

					    QCollapsibleFrame* pInspectorWidget = new QCollapsibleFrame(frameName);
					    pInspectorWidget->SetWidget(pPropertiesWidget);
					    inspector.AddWidget(pInspectorWidget);
					  }
					}
			  });

			pBroadcastManager->Broadcast(popEvent);
		}
		else
		{
			ClearLegacyInspectorEvent().Broadcast(pBroadcastManager);
		}
	}
}

void CNodeGraphView::MoveSelection(const QPointF& delta)
{
	if (m_action == eAction_ItemMovement)
	{
		for (CAbstractNodeGraphViewModelItem* pGraphItem : m_selectedItems)
		{
			if (pGraphItem->GetAcceptsMoves())
			{
				pGraphItem->MoveBy(delta.x(), delta.y());
			}
		}
	}
}

void CNodeGraphView::PlaceSelection()
{
	GraphViewWidgetSet pendingSelection;
	for (auto pWidget : m_selectedWidgets)
	{
		CAbstractNodeGraphViewModelItem* item = pWidget->GetAbstractItem();
		if (item->GetType() == eItemType_Node || item->GetType() == eItemType_Comment)
			pendingSelection.insert(pWidget);
	}

	for (auto pWidget : pendingSelection)
	{
		auto groupWidget = m_groupWidgetByItemInstance[pWidget->GetAbstractItem()];
		if (groupWidget)
		{
			UnlinkItemFromGroup(*groupWidget, *pWidget);
		}
	}

	PlaceSelectionToGroup(pendingSelection);
}

void CNodeGraphView::RemoveNodeItem(CAbstractNodeItem& node)
{
	GetIEditor()->GetIUndoManager()->Begin();

	m_pModel->RemoveNode(node);

	stack_string nodeName = QtUtil::ToString(node.GetName());
	stack_string desc;
	desc.Format("Undo remove of node '%s'", nodeName.c_str());

	GetIEditor()->GetIUndoManager()->Accept(desc.c_str());
}

void CNodeGraphView::RemoveConnectionItem(CAbstractConnectionItem& connection)
{
	GetIEditor()->GetIUndoManager()->Begin();

	m_pModel->RemoveConnection(connection);

	GetIEditor()->GetIUndoManager()->Accept("Undo remove of connection");
}

void CNodeGraphView::DeleteSelectedItems()
{
	if (m_selectedWidgets.size())
	{
		GetIEditor()->GetIUndoManager()->Begin();

		stack_string singleItemDesc;
		int32 numDeletedItems = 0;

		// We need to empty the inspector first before we start to remove items.
		BroadcastSelectionChange(true);
		m_operation = eOperation_DeleteSelected;

		GraphItemSet notDeletedItems;
		for (auto itr = m_selectedItems.begin(); itr != m_selectedItems.end(); itr = m_selectedItems.begin())
		{
			CAbstractNodeGraphViewModelItem* pAbstractItem = *itr;

			// Ensure that the items gets removed from the selection set. This also ensures that
			// during removal it doesn't do the standard deselection including broadcast etc.
			SetItemSelectionState(*pAbstractItem, false);
			if (!pAbstractItem->GetAcceptsDeletion())
			{
				notDeletedItems.emplace(pAbstractItem);
				continue;
			}

			const int32 type = pAbstractItem->GetType();
			switch (type)
			{
			case eItemType_Node:
				{
					string nodeName;
					CAbstractNodeItem& nodeItem = *pAbstractItem->Cast<CAbstractNodeItem>();
					if (numDeletedItems == 0)
					{
						nodeName = QtUtil::ToString(nodeItem.GetName());
					}

					if (m_pModel->RemoveNode(nodeItem))
					{
						++numDeletedItems;
					}
					else
					{
						notDeletedItems.emplace(pAbstractItem);
					}

					if (numDeletedItems == 1)
					{
						singleItemDesc.Format("Undo deletion of node '%s'", nodeName.c_str());
					}
				}
				break;
			case eItemType_Comment:
				{
					CAbstractCommentItem& commentItem = *pAbstractItem->Cast<CAbstractCommentItem>();
					if (m_pModel->RemoveComment(commentItem))
					{
						++numDeletedItems;
					}
					else
					{
						notDeletedItems.emplace(pAbstractItem);
					}
				}
				break;
			case eItemType_Connection:
				{
					CAbstractConnectionItem& connectionItem = *pAbstractItem->Cast<CAbstractConnectionItem>();
					if (m_pModel->RemoveConnection(connectionItem))
					{
						++numDeletedItems;
					}
					else
					{
						notDeletedItems.emplace(pAbstractItem);
					}
				}
				break;
			default:
				if (type >= eItemType_UserType)
				{
					if (DeleteCustomItem(*pAbstractItem))
					{
						++numDeletedItems;
					}
					else
					{
						notDeletedItems.emplace(pAbstractItem);
					}

					// TODO: More control over undo description of custom items.
					singleItemDesc = "Undo graph item deletion";
					// ~TODO
				}
				break;
			}
		}

		if (numDeletedItems == 1 && !singleItemDesc.empty())
			GetIEditor()->GetIUndoManager()->Accept(singleItemDesc.c_str());
		else if (numDeletedItems > 0)
			GetIEditor()->GetIUndoManager()->Accept("Undo multi-item deletion");
		else
			GetIEditor()->GetIUndoManager()->Cancel();

		m_operation = eOperation_None;

		if (notDeletedItems.size() > 0)
		{
			SelectItems(notDeletedItems);
		}
	}
}

void CNodeGraphView::OnCreateNode(CAbstractNodeItem& node)
{
	if (CNodeWidget* pNodeWidget = node.CreateWidget(*this))
	{
		m_pScene->addItem(pNodeWidget);
		m_nodeWidgetByItemInstance.emplace(&node, pNodeWidget);
	}

	node.GetEditorData().SignalDataChanged();
}

void CNodeGraphView::OnCreateGroup(CAbstractGroupItem& group)
{
	if (CGroupWidget* pGroupWidget = group.CreateWidget(*this))
	{
		m_pScene->addItem(pGroupWidget);
		m_pScene->addItem(pGroupWidget->GetPendingWidget());
	}

	group.GetEditorData().SignalDataChanged();
}

void CNodeGraphView::OnCreateComment(CAbstractCommentItem& comment)
{
	if (CCommentWidget* pCommentWidget = comment.CreateWidget(*this))
	{
		m_pScene->addItem(pCommentWidget);
		m_commentWidgetByItemInstance.emplace(&comment, pCommentWidget);
	}

	comment.GetEditorData().SignalDataChanged();
}

void CNodeGraphView::OnCreateConnection(CAbstractConnectionItem& connection)
{
	CNodeWidget* pSourceNode = GetNodeWidget(connection.GetSourcePinItem().GetNodeItem());
	CNodeWidget* pTargetNode = GetNodeWidget(connection.GetTargetPinItem().GetNodeItem());

	CRY_ASSERT(pSourceNode && pTargetNode);
	CAbstractNodeContentWidget* pSourceNodeContent = pSourceNode ? pSourceNode->GetContentWidget() : nullptr;
	CAbstractNodeContentWidget* pTargetNodeContent = pTargetNode ? pTargetNode->GetContentWidget() : nullptr;

	CRY_ASSERT(pSourceNodeContent && pTargetNodeContent);
	CConnectionPoint* pSourceConnectionPoint = pSourceNodeContent ? pSourceNodeContent->GetConnectionPoint(connection.GetSourcePinItem()) : nullptr;
	CConnectionPoint* pTargetConnectionPoint = pTargetNodeContent ? pTargetNodeContent->GetConnectionPoint(connection.GetTargetPinItem()) : nullptr;

	CRY_ASSERT(pSourceConnectionPoint && pTargetConnectionPoint);
	if (pSourceConnectionPoint && pTargetConnectionPoint)
	{
		CConnectionWidget* pConnectionWidget = connection.CreateWidget(*this);
		pConnectionWidget->SetSourceConnectionPoint(pSourceConnectionPoint);
		pConnectionWidget->SetTargetConnectionPoint(pTargetConnectionPoint);
		m_pScene->addItem(pConnectionWidget);
		pConnectionWidget->Update();

		m_connectionWidgetByItemInstance.emplace(&connection, pConnectionWidget);
	}
}

void CNodeGraphView::OnRemoveNode(CAbstractNodeItem& node)
{
	CRY_ASSERT_MESSAGE(m_pModel, "No items should exist if m_pModel == nullptr");
	if (m_pModel)
		m_pModel->GetEditorData().RemoveNodeEditorDataById(node.GetId());

	const QPointF nodePos = node.GetPosition();
	const QList<QGraphicsItem*> itemList = m_pScene->items(QRectF(nodePos - QPoint(5, 5), nodePos + QPoint(5, 5)));
	for (QGraphicsItem* pGraphicsItem : itemList)
	{
		if (CNodeWidget* pNodeWidget = CNodeGraphViewGraphicsWidget::Cast<CNodeWidget>(pGraphicsItem))
		{
			if (&pNodeWidget->GetItem() == &node)
			{
				m_nodeWidgetByItemInstance.erase(&node);
				m_pScene->removeItem(pGraphicsItem);
				DeselectWidget(*pNodeWidget);
				pNodeWidget->DeleteLater();
				return;
			}
		}
	}

	return;
}

void CNodeGraphView::OnRemoveGroup(CAbstractGroupItem& group)
{
	CRY_ASSERT_MESSAGE(m_pModel, "No items should exist if m_pModel == nullptr");
	if (m_pModel)
		m_pModel->GetEditorData().RemoveGroupEditorDataById(group.GetId());

	const QPointF groupPos = group.GetPosition();

	const QList<QGraphicsItem*> itemList = m_pScene->items(QRectF(groupPos - QPoint(5, 5), groupPos + QPoint(5, 5)));
	for (QGraphicsItem* pGraphicsItem : itemList)
	{
		if (CGroupWidget* pGroupWidget = CNodeGraphViewGraphicsWidget::Cast<CGroupWidget>(pGraphicsItem))
		{
			if (pGroupWidget->GetAbstractItem() == &group)
			{
				m_pScene->removeItem(pGroupWidget->GetPendingWidget());
				m_pScene->removeItem(pGroupWidget);
				DeselectWidget(*pGroupWidget);
				pGroupWidget->DeleteLater();
				return;
			}
		}
	}

	return;
}

void CNodeGraphView::OnRemoveComment(CAbstractCommentItem& comment)
{
	CRY_ASSERT_MESSAGE(m_pModel, "No items should exist if m_pModel == nullptr");
	if (m_pModel)
		m_pModel->GetEditorData().RemoveCommentEditorDataById(comment.GetId());

	const QPointF commentPos = comment.GetPosition();
	const QList<QGraphicsItem*> itemList = m_pScene->items(QRectF(commentPos - QPoint(5, 5), commentPos + QPoint(5, 5)));
	for (QGraphicsItem* pGraphicsItem : itemList)
	{
		if (CCommentWidget* pCommentWidget = CNodeGraphViewGraphicsWidget::Cast<CCommentWidget>(pGraphicsItem))
		{
			if (pCommentWidget->GetAbstractItem() == &comment)
			{
				m_commentWidgetByItemInstance.erase(&comment);
				m_pScene->removeItem(pCommentWidget);
				DeselectWidget(*pCommentWidget);
				pCommentWidget->DeleteLater();
				return;
			}
		}
	}

	return;
}

void CNodeGraphView::OnRemoveConnection(CAbstractConnectionItem& connection)
{
	// TODO: This isn't really optimal!
	const QPointF sp = connection.GetSourcePinItem().GetNodeItem().GetPosition();
	const QPointF tp = connection.GetTargetPinItem().GetNodeItem().GetPosition();

	const qreal x = std::min(sp.x(), tp.x());
	const qreal y = std::min(sp.y(), tp.y());
	const qreal w = std::max(std::abs(sp.x() - tp.x()), 5.0);
	const qreal h = std::max(std::abs(sp.y() - tp.y()), 5.0);

	const QPointF topLeft = QPointF(x, y);
	const QRectF rect = QRectF(topLeft, topLeft + QPointF(w, h));

	const QList<QGraphicsItem*> itemList = m_pScene->items(rect, Qt::ItemSelectionMode::IntersectsItemBoundingRect);
	for (QGraphicsItem* pGraphicsItem : itemList)
	{
		if (CConnectionWidget* pConnectionWidget = CNodeGraphViewGraphicsWidget::Cast<CConnectionWidget>(pGraphicsItem))
		{
			if (pConnectionWidget->GetItem() == &connection)
			{
				m_connectionWidgetByItemInstance.erase(&connection);

				m_pScene->removeItem(pGraphicsItem);
				DeselectWidget(*pConnectionWidget);
				pConnectionWidget->DeleteLater();
				return;
			}
		}
	}

	CRY_ASSERT_MESSAGE(false, "Item not found!!");
}

void CNodeGraphView::OnContextMenuAddNode(CAbstractDictionaryEntry& entry)
{
	if (m_pModel && entry.GetType() == CAbstractDictionaryEntry::Type_Entry)
	{
		const bool tryToConnect = (m_action == eAction_ConnectionCreationAddNodeMenu);

		GetIEditor()->GetIUndoManager()->Begin();
		CAbstractNodeItem* pNodeItem = m_pModel->CreateNode(entry.GetIdentifier(), m_contextMenuScenePos);
		if (pNodeItem)
		{
			stack_string nodeName = QtUtil::ToString(pNodeItem->GetName());
			stack_string desc;
			desc.Format("Undo node '%s' creation", nodeName.c_str());

			// Note: In the case the view will try to connect the node when it
			if (pNodeItem && tryToConnect)
			{
				CRY_ASSERT(m_pModel);
				if (m_pModel)
				{
					CAbstractConnectionItem* pConnectionItem = nullptr;

					// TODO: Use a virtual function that can be overloaded to let the graph runtime decide if and what to connect.
					CRY_ASSERT(m_pNewConnectionLine);
					if (m_pNewConnectionLine)
					{
						if (m_pNewConnectionLine->GetSourceConnectionPoint() != &m_mouseConnectionPoint)
						{
							CRY_ASSERT(m_selectedWidgets.size() == 1);
							CPinWidget* pSourcePin = CNodeGraphViewGraphicsWidget::Cast<CPinWidget>(*m_selectedWidgets.begin());
							CRY_ASSERT(pSourcePin);
							CAbstractPinItem& sourcePin = pSourcePin->GetItem();

							for (CAbstractPinItem* pTargetPin : pNodeItem->GetPinItems())
							{
								if (pTargetPin->IsInputPin() && pTargetPin->CanConnect(&sourcePin))
								{
									pConnectionItem = m_pModel->CreateConnection(sourcePin, *pTargetPin);
									break;
								}
							}
						}
						else if (m_pNewConnectionLine->GetTargetConnectionPoint() != &m_mouseConnectionPoint)
						{
							CRY_ASSERT(m_selectedWidgets.size() == 1);
							CPinWidget* pTargetPin = CNodeGraphViewGraphicsWidget::Cast<CPinWidget>(*m_selectedWidgets.begin());
							CRY_ASSERT(pTargetPin);
							CAbstractPinItem& targetPin = pTargetPin->GetItem();

							for (CAbstractPinItem* pSourcePin : pNodeItem->GetPinItems())
							{
								if (pSourcePin->IsOutputPin() && pSourcePin->CanConnect(&targetPin))
								{
									pConnectionItem = m_pModel->CreateConnection(*pSourcePin, targetPin);
									break;
								}
							}
						}
					}
				}
			}

			GetIEditor()->GetIUndoManager()->Accept(desc.c_str());

			AbortAction();
			m_pContextMenu->hide();
		}
	}
}

void CNodeGraphView::OnContextMenuAbort()
{
	if (m_action == eAction_ConnectionCreationAddNodeMenu)
	{
		AbortAction();
	}
}

void CNodeGraphView::OnEditingContentPopupFinish()
{
	if (m_action == eAction_ShowContentEditPopup)
	{
		AbortAction();
	}
}

void CNodeGraphView::SetAction(uint32 action, bool abortCurrent)
{
	if (m_action != action)
	{
		if (abortCurrent)
			AbortAction();

		CRY_ASSERT(abortCurrent == false || m_action == eAction_None);

		if (action != eAction_None)
		{
			ShowItemToolTip(nullptr);
		}

		m_action = action;
	}

	switch (m_action)
	{
	case eAction_ItemMovement:
		{
			BeginPendingItemPlacement();
		}
		break;
	}
}

bool CNodeGraphView::AbortAction()
{
	if (m_action == eAction_None)
		return true;

	switch (m_action)
	{
	case eAction_GroupBoxResizing:
		{
			if (m_pResizingBox)
			{
				m_pResizingBox = nullptr;
				SelectMouseCursor(QPointF());
			}
		}
	case eAction_ItemBoxSelection:
		{
			if (m_pSelectionBox)
			{
				m_pSelectionBox->ungrabMouse();

				m_pScene->removeItem(m_pSelectionBox);
				m_pSelectionBox->DeleteLater();
				m_pSelectionBox = nullptr;
			}
		}
		break;
	case eAction_ConnectionCreationAddNodeMenu:
	case eAction_ConnectionCreation:
		EndCreateConnection(nullptr);
		break;
	case eAction_SceneNavigation:
		return false;   // We don't allow to abort scene navigation.
	case eAction_ItemMovement:
		{
			if (m_isRecodringUndo)
			{
				if (m_selectedItems.size() > 1)
				{
					GetIEditor()->GetIUndoManager()->Accept("Undo multi-item movement.");
				}
				else
				{
					// Note: Currently only nodes can be moved.
					if (m_selectedItems.size() == 1)
					{
						if (CAbstractNodeItem* pNodeItem = (*m_selectedItems.begin())->Cast<CAbstractNodeItem>())
						{
							stack_string nodeName = QtUtil::ToString(pNodeItem->GetName());
							stack_string desc;
							desc.Format("Undo '%s' node movement", nodeName.c_str());
							GetIEditor()->GetIUndoManager()->Accept(desc.c_str());
						}
					}
				}
				m_isRecodringUndo = false;
			}

			EndPendingItemPlacement();
		}
		break;
	case eAction_ShowContentEditPopup:
		{
			m_pContentEditPopup->hide();
		}
		break;
	case eAction_None:
	default:
		break;
	}

	m_action = eAction_None;
	return true;
}

void CNodeGraphView::ShowItemToolTip(CNodeGraphViewGraphicsWidget* pItemWidget)
{
	if (m_action == eAction_None && pItemWidget)
	{
		CAbstractNodeGraphViewModelItem* pItem = pItemWidget->GetAbstractItem();
		if (pItem)
		{
			const QString toolTipText = pItem->GetToolTipText();
			if (!toolTipText.isEmpty())
			{
				m_pTooltip->SetText(toolTipText);
				if (!m_isTooltipActive)
				{
					m_isTooltipActive = true;
					m_toolTipTimer.singleShot(500, this, [this]()
						{
							if (m_isTooltipActive)
								QTrackingTooltip::ShowTrackingTooltip(m_pTooltip);
						});
				}

				return;
			}
		}
	}

	m_isTooltipActive = false;
	m_toolTipTimer.stop();
	QTrackingTooltip::HideTooltip(m_pTooltip.data());
	m_pTooltip->SetText("");
}

void CNodeGraphView::ReloadItems()
{
	SetStyle(m_pModel->GetRuntimeContext().GetStyle());

	DeselectAllItems();
	ClearItems();

	for (int32 i = m_pModel->GetNodeItemCount(); i--;)
	{
		if (CAbstractNodeItem* pItem = m_pModel->GetNodeItemByIndex(i))
			AddNodeItem(*pItem);
	}

	for (int32 i = m_pModel->GetCommentItemCount(); i--;)
	{
		if (CAbstractCommentItem* pItem = m_pModel->GetCommentItemByIndex(i))
			AddCommentItem(*pItem);
	}

	for (int32 i = m_pModel->GetConnectionItemCount(); i--;)
	{
		if (CAbstractConnectionItem* pItem = m_pModel->GetConnectionItemByIndex(i))
			AddConnectionItem(*pItem);
	}

	// groups should be restored after all other items restored
	for (int32 i = m_pModel->GetGroupItemCount(); i--;)
	{
		if (CAbstractGroupItem* pItem = m_pModel->GetGroupItemByIndex(i))
			AddGroupItem(*pItem);
	}

	SignalItemsReloaded(*this);

	update();
}

void CNodeGraphView::ClearItems()
{
	for (auto itemWidgetPair : m_connectionWidgetByItemInstance)
	{
		if (!itemWidgetPair.second)
			continue;
		m_pScene->removeItem(itemWidgetPair.second);
		itemWidgetPair.second->DeleteLater();
	}

	for (auto itemWidgetPair : m_nodeWidgetByItemInstance)
	{
		if (!itemWidgetPair.second)
			continue;
		m_pScene->removeItem(itemWidgetPair.second);
		itemWidgetPair.second->DeleteLater();
	}

	for (auto itemWidgetPair : m_groupWidgetByItemInstance)
	{
		if (!itemWidgetPair.second)
			continue;
		m_pScene->removeItem(itemWidgetPair.second);
		itemWidgetPair.second->DeleteLater();
	}

	for (auto itemWidgetPair : m_commentWidgetByItemInstance)
	{
		if (!itemWidgetPair.second)
			continue;
		m_pScene->removeItem(itemWidgetPair.second);
		itemWidgetPair.second->DeleteLater();
	}

	m_pScene->removeItem(m_pBackground);
	m_pScene->clear();

	m_nodeWidgetByItemInstance.clear();
	m_groupWidgetByItemInstance.clear();
	m_commentWidgetByItemInstance.clear();
	m_connectionWidgetByItemInstance.clear();

	m_pScene->addItem(m_pBackground);
}

void CNodeGraphView::AddNodeItem(CAbstractNodeItem& node)
{
	OnCreateNode(node);
}

void CNodeGraphView::AddGroupItem(CAbstractGroupItem& group)
{
	OnCreateGroup(group);
}

void CNodeGraphView::AddCommentItem(CAbstractCommentItem& comment)
{
	OnCreateComment(comment);
}

void CNodeGraphView::AddConnectionItem(CAbstractConnectionItem& connection)
{
	OnCreateConnection(connection);
}

void CNodeGraphView::keyPressEvent(QKeyEvent* pEvent)
{
	QGraphicsView::keyPressEvent(pEvent);
	if (!pEvent->isAccepted())
	{
		const int key = pEvent->key();
		if (key >= Qt::Key_A && key <= Qt::Key_Z && pEvent->modifiers() == 0)
		{
			const QPointF pos = mapToGlobal(viewport()->geometry().center());
			ShowGraphContextMenu(QPointF(pos.x(), pos.y()));
			if (m_pContextMenuContent)
			{
				m_pContextMenuContent->SetFilterText(pEvent->text());
			}
		}
	}
}

void CNodeGraphView::wheelEvent(QWheelEvent* pEvent)
{
	AbortAction();

	const int32 percent = m_currentScalePercent + (pEvent->delta() > 0 ? 10 : -10);
	SetZoom(percent);
}

void CNodeGraphView::mouseMoveEvent(QMouseEvent* pEvent)
{
	if (m_action != eAction_ConnectionCreationAddNodeMenu)
		m_mouseConnectionPoint.SetPosition(mapToScene(pEvent->pos()));

	switch (m_action)
	{
	case eAction_GroupBoxResizing:
		{
			if (m_pResizingBox)
			{
				m_pResizingBox->CalcResize(mapToScene(pEvent->pos()));
			}
			else
			{
				AbortAction();
			}
		}
	case eAction_SceneNavigation:
		{
			QMouseEvent moveEvent(
				QEvent::DragMove, pEvent->localPos(), pEvent->windowPos(), pEvent->screenPos(),
				Qt::LeftButton, pEvent->buttons() | Qt::LeftButton, pEvent->modifiers());
			QGraphicsView::mouseMoveEvent(&moveEvent);
		}
		break;
	case eAction_ConnectionCreation:
		{
			const QPointF scenePos = mapToScene(pEvent->pos());
			m_pNewConnectionPossibleTargetPin = GetPossibleConnectionTarget(scenePos);

			CConnectionPoint* pConnectionPoint = &m_mouseConnectionPoint;
			if (m_pNewConnectionPossibleTargetPin)
			{
				pConnectionPoint = &m_pNewConnectionPossibleTargetPin->GetConnectionPoint();
			}

			if (m_pNewConnectionBeginPin->GetItem().IsOutputPin())
			{
				m_pNewConnectionLine->SetTargetConnectionPoint(pConnectionPoint);
			}
			else
			{
				m_pNewConnectionLine->SetSourceConnectionPoint(pConnectionPoint);
			}
		}
		break;
	case eAction_ItemBoxSelection:
	case eAction_ItemMovement:
	case eAction_None:
	default:
		QGraphicsView::mouseMoveEvent(pEvent);
		break;
	}
}

void CNodeGraphView::mousePressEvent(QMouseEvent* pEvent)
{
	if (m_action == eAction_None)
	{
		m_lastMouseActionPressPos = pEvent->pos();

		if (IsDraggingButton(pEvent->buttons()))
		{
			BeginSceneDragging(pEvent);
			return;
		}
		else if (IsSelectionButton(pEvent->buttons()))
		{
			if (m_pResizingBox)
			{
				SetAction(eAction_GroupBoxResizing);
			}
			else if (itemAt(m_lastMouseActionPressPos) == m_pBackground)
			{
				SetAction(eAction_ItemBoxSelection);
				if (m_pSelectionBox == nullptr)
				{
					m_pSelectionBox = new CGroupWidget(*this);
					m_pSelectionBox->SetColor(QColor(97, 172, 237, 128));
				}

				m_pScene->addItem(m_pSelectionBox);
				m_pSelectionBox->SetAA(mapToScene(m_lastMouseActionPressPos));
				m_pSelectionBox->SetBB(mapToScene(m_lastMouseActionPressPos));
				m_pSelectionBox->grabMouse();
			}
		}
	}

	QGraphicsView::mousePressEvent(pEvent);
}

void CNodeGraphView::mouseReleaseEvent(QMouseEvent* pEvent)
{
	switch (m_action)
	{
	case eAction_SceneNavigation:
		{
			EndSceneDragging(pEvent);
			if (m_lastMouseActionPressPos != pEvent->pos())
				return;

			if (RequestContextMenu(pEvent))
				return;
		}
		break;
	case eAction_ConnectionCreation:
		{
			if (pEvent->button() == Qt::MouseButton::LeftButton)
			{
				// Note: Because the first item will be always the line that get drawn while creating a connection.
				//			 we need to request all items at that position and check the second one.
				if (!m_pNewConnectionPossibleTargetPin)
				{
					QList<QGraphicsItem*> itemsAtPos = items(pEvent->pos());
					if (itemsAtPos.size())
					{
						int32 showContextMenu = 0;
						showContextMenu += (itemsAtPos.size() >= 1 && itemsAtPos[0] == m_pBackground);
						showContextMenu += (itemsAtPos.size() >= 2 && itemsAtPos[0] == m_pNewConnectionLine && itemsAtPos[1] == m_pBackground);

						if (showContextMenu != 0)
						{
							if (m_pNewConnectionLine && m_pNewConnectionLine->GetSourceConnectionPoint() != &m_mouseConnectionPoint)
								ShowGraphContextMenu(pEvent->screenPos());
							else
								AbortAction();

							return;
						}
					}
				}
			}

			QPointF eventPos = mapToScene(pEvent->pos());
			if (m_pNewConnectionPossibleTargetPin)
			{
				eventPos = m_pNewConnectionPossibleTargetPin->GetConnectionPoint().GetPosition();
			}

			// In connection creation we want the underlying item to receive the release event instead
			// of the current mouse grabber item
			QGraphicsItem* pItem = m_pScene->itemAt(eventPos.x(), eventPos.y(), QTransform());
			if (pItem /*&& pItem->type() != QGraphicsProxyWidget::Type*/)
			{
				if (QGraphicsItem* pMouseGrabberItem = m_pScene->mouseGrabberItem())
				{
					pMouseGrabberItem->ungrabMouse();
				}

				pItem->grabMouse();
				QGraphicsView::mouseReleaseEvent(pEvent);
				pItem->ungrabMouse();

				AbortAction();
				return;
			}
		}
		break;
	case eAction_ShowContentEditPopup:
		{
			QGraphicsView::mouseReleaseEvent(pEvent);
			return;
		}
	case eAction_None:
		{
			if (RequestContextMenu(pEvent))
				return;
		}
		break;
	default:
		break;
	}

	QGraphicsView::mouseReleaseEvent(pEvent);
}

bool CNodeGraphView::OnCopyEvent()
{
	if (m_pModel)
	{
		std::unique_ptr<CItemCollection> pClipboardItems(m_pModel->CreateClipboardItemsCollection());
		Serialization::IArchiveHost* pArchiveHost = GetIEditor()->GetSystem()->GetArchiveHost();

		if (pClipboardItems && pArchiveHost)
		{
			QRectF itemsRect;
			for (CNodeGraphViewGraphicsWidget* pViewWidget : m_selectedWidgets)
			{
				CAbstractNodeGraphViewModelItem* pAbstractItem = pViewWidget->GetAbstractItem();
				if (!pAbstractItem || !pAbstractItem->GetAcceptsCopy())
				{
					continue;
				}

				if (CNodeWidget* pNode = CNodeGraphViewGraphicsWidget::Cast<CNodeWidget>(pViewWidget))
				{
					pClipboardItems->AddNode(pNode->GetItem());

					const QRectF nodeRect = pNode->geometry();
					if (itemsRect.isValid())
					{
						if (nodeRect.left() < itemsRect.left())
						{
							itemsRect.setLeft(nodeRect.left());
						}
						if (nodeRect.width() > itemsRect.width())
						{
							itemsRect.setWidth(nodeRect.width());
						}
						if (nodeRect.top() < itemsRect.top())
						{
							itemsRect.setTop(nodeRect.top());
						}
						if (nodeRect.height() > itemsRect.height())
						{
							itemsRect.setHeight(nodeRect.height());
						}
					}
					else
					{
						itemsRect = nodeRect;
					}
				}
				else if (CConnectionWidget* pConnection = CNodeGraphViewGraphicsWidget::Cast<CConnectionWidget>(pViewWidget))
				{
					if (CAbstractConnectionItem* pConnectionItem = pConnection->GetItem())
					{
						pClipboardItems->AddConnection(*pConnectionItem);
					}
				}
				else if (pViewWidget->GetType() >= eGraphViewWidgetType_UserType)
				{
					pClipboardItems->AddCustomItem(*pAbstractItem);
				}
			}

			pClipboardItems->SetItemsRect(itemsRect);

			const QPoint cursorPos = QCursor::pos();
			if (IsScreenPositionPresented(cursorPos))
			{
				const QPointF sceenPos = mapToScene(mapFromGlobal(cursorPos));
				pClipboardItems->SetScenePosition(sceenPos);

				QList<QGraphicsItem*> graphicsItems = m_pScene->items(m_contextMenuScenePos);
				for (QGraphicsItem* pGraphicsItem : graphicsItems)
				{
					CNodeGraphViewGraphicsWidget* pViewWidget = qgraphicsitem_cast<CNodeGraphViewGraphicsWidget*>(pGraphicsItem);
					if (pViewWidget && pViewWidget->GetAbstractItem() && pViewWidget->GetAbstractItem()->GetAcceptsCopy())
					{
						pClipboardItems->SetViewWidget(pViewWidget);
						break;
					}
				}
			}
			else
			{
				pClipboardItems->SetScenePosition(mapToScene(viewport()->geometry()).boundingRect().center());
				if (m_selectedWidgets.size() == 1)
				{
					CNodeGraphViewGraphicsWidget* pViewWidget = *m_selectedWidgets.begin();
					if (pViewWidget && pViewWidget->GetAbstractItem() && pViewWidget->GetAbstractItem()->GetAcceptsCopy())
					{
						pClipboardItems->SetViewWidget(pViewWidget);
					}
				}
			}

			const char* szEditorName = m_pModel->GetRuntimeContext().GetTypeName();
			XmlNodeRef xmlItems = GetIEditor()->GetSystem()->CreateXmlNode(szEditorName);
			pArchiveHost->SaveXmlNode(xmlItems, Serialization::SStruct(*pClipboardItems));

			const QString xmlText = xmlItems->getXML().c_str();

			QMimeData* pMimeData = new QMimeData();

			pMimeData->setData(m_copyPasteMimeFormat, xmlText.toUtf8());
			QClipboard* pClipboard = QApplication::clipboard();
			pClipboard->setMimeData(pMimeData);

			return true;
		}
	}

	return false;
}

bool CNodeGraphView::OnPasteEvent()
{
	if (m_pModel)
	{
		CItemCollection* pClipboardItems = m_pModel->CreateClipboardItemsCollection();
		Serialization::IArchiveHost* pArchiveHost = GetIEditor()->GetSystem()->GetArchiveHost();

		if (pClipboardItems && pArchiveHost)
		{
			QPointer<QClipboard> pClipboard = QApplication::clipboard();
			const QMimeData* pMimeData = pClipboard->mimeData();

			QByteArray byteArray = pMimeData->data(m_copyPasteMimeFormat);
			if (byteArray.length() > 0)
			{
				const XmlNodeRef xmlItems = GetIEditor()->GetSystem()->LoadXmlFromBuffer(byteArray.data(), byteArray.length(), true);
				const string szEditorName = m_pModel->GetRuntimeContext().GetTypeName();
				if (xmlItems && xmlItems->getTag() == szEditorName)
				{
					const QPoint cursorPos = QCursor::pos();
					const bool isScreenPositionPresented = IsScreenPositionPresented(cursorPos);

					if (isScreenPositionPresented)
					{
						const QPointF sceenPos = mapToScene(mapFromGlobal(cursorPos));
						pClipboardItems->SetScenePosition(sceenPos);
					}
					else
					{
						const QPointF sceenPos = mapToScene(viewport()->geometry()).boundingRect().center();
						pClipboardItems->SetScenePosition(sceenPos);
					}

					if (m_selectedWidgets.size() == 1)
					{
						pClipboardItems->SetViewWidget(*m_selectedWidgets.begin());
					}
					else if (isScreenPositionPresented)
					{
						const QPointF sceenPos = mapToScene(mapFromGlobal(cursorPos));
						QList<QGraphicsItem*> graphicsItems = m_pScene->items(sceenPos);
						for (QGraphicsItem* pGraphicsItem : graphicsItems)
						{
							if (CNodeGraphViewGraphicsWidget* pViewWidget = qgraphicsitem_cast<CNodeGraphViewGraphicsWidget*>(pGraphicsItem))
							{
								pClipboardItems->SetViewWidget(pViewWidget);
								break;
							}
						}
					}

					GetIEditor()->GetIUndoManager()->Begin();

					pArchiveHost->LoadXmlNode(Serialization::SStruct(*pClipboardItems), xmlItems);
					if (pClipboardItems->GetNodes().size() || pClipboardItems->GetConnections().size() || pClipboardItems->GetCustomItems().size())
						GetIEditor()->GetIUndoManager()->Accept("Undo pasting into graph");
					else
						GetIEditor()->GetIUndoManager()->Cancel();

					DeselectAllItems();

					GraphItemSet pastedItems;

					for (CAbstractNodeItem* pNodeItem : pClipboardItems->GetNodes())
						pastedItems.emplace(pNodeItem);

					for (CAbstractConnectionItem* pConnectionItem : pClipboardItems->GetConnections())
						pastedItems.emplace(pConnectionItem);

					for (CAbstractNodeGraphViewModelItem* pAbstractItem : pClipboardItems->GetCustomItems())
						pastedItems.emplace(pAbstractItem);

					SelectItems(pastedItems);

					return true;
				}
			}
		}
	}
	return false;
}

bool CNodeGraphView::OnDeleteEvent()
{
	if (m_pModel)
	{
		DeleteSelectedItems();
		return true;
	}

	return false;
}

bool CNodeGraphView::OnUndoEvent()
{
	return false;
}

bool CNodeGraphView::OnRedoEvent()
{
	return false;
}

void CNodeGraphView::BeginCreateConnection(CPinWidget* pPinWidget)
{
	if (m_action == eAction_None && pPinWidget)
	{
		CAbstractPinItem& pinItem = pPinWidget->GetItem();
		if (pinItem.IsInputPin())
		{
			const ConnectionItemSet& connections = pinItem.GetConnectionItems();
			if (connections.size() != 1)
				return;

			CAbstractConnectionItem* pConnectionItem = *connections.begin();
			pPinWidget = GetPinWidget(pConnectionItem->GetSourcePinItem());

			CRY_ASSERT(pPinWidget);
			m_pModel->RemoveConnection(*pConnectionItem);
		}

		DeselectAllItems();

		if (!pinItem.CanConnect(nullptr))
			return;

		m_selectedWidgets.insert(pPinWidget);
		SetAction(eAction_ConnectionCreation);

		// TODO: Can we make this a member var instead of an pointer?
		if (!m_pNewConnectionLine)
		{
			m_pNewConnectionLine = new CConnectionWidget(nullptr, *this);
		}
		// ~TODO

		m_pNewConnectionLine->setAcceptedMouseButtons(0);
		m_pNewConnectionLine->setAcceptHoverEvents(false);
		m_pNewConnectionLine->setAcceptTouchEvents(false);
		m_pNewConnectionLine->SetSelected(true);
		if (pPinWidget->GetItem().IsOutputPin())
		{
			m_pNewConnectionLine->SetSourceConnectionPoint(&pPinWidget->GetConnectionPoint());
			m_pNewConnectionLine->SetTargetConnectionPoint(&m_mouseConnectionPoint);
		}
		else
		{
			m_pNewConnectionLine->SetSourceConnectionPoint(&m_mouseConnectionPoint);
			m_pNewConnectionLine->SetTargetConnectionPoint(&pPinWidget->GetConnectionPoint());
		}
		m_pScene->addItem(m_pNewConnectionLine);

		m_pNewConnectionBeginPin = pPinWidget;

		SBeginCreateConnectionEventArgs args(pPinWidget->GetItem());
		SendBeginCreateConnection(this, args);
	}
}

void CNodeGraphView::EndCreateConnection(CPinWidget* pTargetPin)
{
	if (m_action == eAction_ConnectionCreation || m_action == eAction_ConnectionCreationAddNodeMenu)
	{
		CRY_ASSERT(m_selectedWidgets.size() == 1);
		CPinWidget* pSourcePin = qobject_cast<CPinWidget*>(*m_selectedWidgets.begin());
		CRY_ASSERT(pSourcePin);

		SEndCreateConnectionEventArgs args(pSourcePin->GetItem());
		if (pTargetPin && pTargetPin != pSourcePin)
		{
			if (!pSourcePin->GetItem().IsOutputPin())
				std::swap(pTargetPin, pSourcePin);

			GetIEditor()->GetIUndoManager()->Begin();

			CAbstractConnectionItem* pConnectionItem = m_pModel->CreateConnection(pSourcePin->GetItem(), pTargetPin->GetItem());
			if (pConnectionItem)
				GetIEditor()->GetIUndoManager()->Accept("Undo connection creation");
			else
				GetIEditor()->GetIUndoManager()->Cancel();

			args.didFail = (pConnectionItem == nullptr);
			args.wasCanceled = false;
			args.pTargetPin = &pTargetPin->GetItem();
		}
		else
		{
			args.didFail = true;
			args.wasCanceled = true;
		}

		SetAction(eAction_None, false);
		if (m_pNewConnectionLine)
		{
			m_pScene->removeItem(m_pNewConnectionLine);
			m_pNewConnectionLine->DeleteLater();
			m_pNewConnectionLine = nullptr;
		}

		m_pNewConnectionBeginPin = nullptr;
		m_pNewConnectionPossibleTargetPin = nullptr;

		m_selectedWidgets.clear();
		SendEndCreateConnection(this, args);
	}
}

void CNodeGraphView::BeginPendingItemPlacement()
{
	m_pendingItemPlacementTimer.start();
}

void CNodeGraphView::RunPendingItemPlacement()
{
	GraphViewWidgetSet selection;

	// filter selected items
	for (auto pItem : m_selectedWidgets)
	{
		CAbstractNodeGraphViewModelItem* pAbstractItem = pItem->GetAbstractItem();
		if (pAbstractItem && (pAbstractItem->GetType() == eItemType_Node || pAbstractItem->GetType() == eItemType_Comment))
			selection.insert(pItem);
	}

	CGroupWidget* pGroupCanditate = GetGroupCandidateForPlacement(selection);
	if (pGroupCanditate)
	{
		pGroupCanditate->PushPendingSelection(selection);
	}
}

void CNodeGraphView::EndPendingItemPlacement()
{
	m_pendingItemPlacementTimer.stop();

	uint32 groupCount = m_pModel->GetGroupItemCount();
	for (int32 i = 0; i < groupCount; i++)
	{
		CGroupWidget* pGroup = m_pModel->GetGroupItemByIndex(i)->GetWidget();
		if (pGroup)
			pGroup->ClearPendingSelection();
	}

}

void CNodeGraphView::SelectMouseCursor(const QPointF& pos)
{
	CGroupWidget::EResizePolicy policy = CGroupWidget::ResizePolicy_None;
	if (m_pResizingBox)
		policy = m_pResizingBox->CalcPolicy(pos);

	switch (policy)
	{
	case CGroupWidget::ResizePolicy_None:
		{
			if (m_pResizingBox)
			{
				m_pResizingBox->setCursor(QCursor(Qt::ArrowCursor));
				m_pResizingBox = nullptr;
			}
		}
		break;
	case CGroupWidget::ResizePolicy_Top:
	case CGroupWidget::ResizePolicy_Bottom:
		{
			m_pResizingBox->setCursor(QCursor(Qt::SizeVerCursor));
		}
		break;
	case CGroupWidget::ResizePolicy_Right:
	case CGroupWidget::ResizePolicy_Left:
		{
			m_pResizingBox->setCursor(QCursor(Qt::SizeHorCursor));
		}
		break;
	case CGroupWidget::ResizePolicy_TopLeft:
	case CGroupWidget::ResizePolicy_BottomRight:
		{
			m_pResizingBox->setCursor(QCursor(Qt::SizeFDiagCursor));
		}
		break;
	case CGroupWidget::ResizePolicy_BottomLeft:
	case CGroupWidget::ResizePolicy_TopRight:
		{
			m_pResizingBox->setCursor(QCursor(Qt::SizeBDiagCursor));
		}
		break;
	}
}

void CNodeGraphView::PlaceSelectionToGroup(const GraphViewWidgetSet& selection)
{
	CGroupWidget* pGroupCanditate = GetGroupCandidateForPlacement(selection);
	if (pGroupCanditate)
	{
		for (auto pItem : selection)
		{
			LinkItemToGroup(*pGroupCanditate, *pItem);
			pGroupCanditate->UpdateAABB();
		}
	}
}

void CNodeGraphView::BeginSceneDragging(QMouseEvent* pEvent)
{
	SetAction(eAction_SceneNavigation);
	setDragMode(QGraphicsView::ScrollHandDrag);

	// We fake a left mouse click because build in scrolling works with left mouse button only.
	QMouseEvent leftPressEvent(
		pEvent->type(), pEvent->localPos(), pEvent->windowPos(), pEvent->screenPos(),
		Qt::LeftButton, pEvent->buttons() | Qt::LeftButton, pEvent->modifiers());
	QGraphicsView::mousePressEvent(&leftPressEvent);
}

void CNodeGraphView::EndSceneDragging(QMouseEvent* pEvent)
{
	// We fake a left mouse release because build-in scrolling works with left mouse button only.
	QMouseEvent leftReleaseEvent(
		pEvent->type(), pEvent->localPos(), pEvent->windowPos(), pEvent->screenPos(),
		Qt::LeftButton, pEvent->buttons() & ~Qt::LeftButton, pEvent->modifiers());
	QGraphicsView::mousePressEvent(&leftReleaseEvent);

	setDragMode(QGraphicsView::NoDrag);
	SetAction(eAction_None, false);
}

bool CNodeGraphView::RequestContextMenu(QMouseEvent* pEvent)
{
	if (pEvent->button() == Qt::MouseButton::RightButton)
	{
		QGraphicsItem* pItem = itemAt(pEvent->pos());
		if (m_pBackground == pItem)
		{
			ShowGraphContextMenu(pEvent->screenPos());
			return true;
		}
	}

	return false;
}

void CNodeGraphView::ShowNodeContextMenu(CNodeWidget* pNodeWidget, QPointF screenPos)
{
	ICommandManager* pCommandManager = GetIEditor()->GetICommandManager();
	CAbstractNodeItem& item = pNodeWidget->GetItem();

	QMenu menu;
	if (item.GetAcceptsCopy())
	{
		menu.addAction(pCommandManager->GetAction("general.copy"));
	}

	if (item.GetAcceptsPaste())
	{
		menu.addAction(pCommandManager->GetAction("general.paste"));
	}

	// TODO: Not yet implemented.
	//menu.addAction(pCommandManager->GetAction("general.cut"));
	// ~TODO

	if (item.GetAcceptsDeletion())
	{
		menu.addAction(pCommandManager->GetAction("general.delete"));
	}

	if (item.GetAcceptsRenaming())
	{
		menu.addSeparator();
		QAction* pAction = menu.addAction(QObject::tr("Rename"));
		QObject::connect(pAction, &QAction::triggered, this, [pNodeWidget](bool isChecked)
		{
			pNodeWidget->EditName();
		});

		menu.addSeparator();
	}

	QAction* pSeperator = menu.addSeparator();
	if (!PopulateNodeContextMenu(item, menu))
	{
		menu.removeAction(pSeperator);
	}

	if (item.GetAcceptsDeactivation())
	{
		menu.addSeparator();

		QAction* pAction = menu.addAction(QObject::tr("Active"));
		pAction->setCheckable(true);
		pAction->setChecked(!item.IsDeactivated());
		QObject::connect(pAction, &QAction::triggered, this, [&item](bool isChecked)
			{
				item.SetDeactivated(!isChecked);
			});

		menu.addSeparator();
	}

	const QPoint parent = mapFromGlobal(QPoint(screenPos.x(), screenPos.y()));
	const QPointF sceenPos = mapToScene(parent);
	m_contextMenuScenePos = sceenPos;

	menu.exec(QPoint(screenPos.x(), screenPos.y()));
}

void CNodeGraphView::ShowGraphContextMenu(QPointF screenPos)
{
	if (m_pModel)
	{
		if (m_action == eAction_ConnectionCreation)
			SetAction(eAction_ConnectionCreationAddNodeMenu, false);
		else if (m_action == eAction_None)
			SetAction(eAction_AddNodeMenu);
		else
			return;

		const QPoint parent = mapFromGlobal(QPoint(screenPos.x(), screenPos.y()));
		const QPointF sceenPos = mapToScene(parent);

		const QRectF viewRect = mapToScene(viewport()->geometry()).boundingRect();
		if (viewRect.contains(sceenPos))
		{
			CAbstractDictionary* pAvailableNodesDictionary = m_pModel->GetRuntimeContext().GetAvailableNodesDictionary();
			if (pAvailableNodesDictionary)
			{
				m_contextMenuScenePos = sceenPos;

				m_pContextMenuContent->RemoveAllDictionaries();
				m_pContextMenuContent->AddDictionary(*pAvailableNodesDictionary);

				m_pContextMenu->ShowAt(QPoint(screenPos.x(), screenPos.y()));
			}
		}
	}
}

void CNodeGraphView::ShowSelectionContextMenu(QPointF screenPos)
{
	bool isAnyItemCopyable = false;
	bool isAnyItemDeletable = false;
	bool isAnyItemDeactivatable = false;
	for (CAbstractNodeGraphViewModelItem* pViewItem : m_selectedItems)
	{
		isAnyItemCopyable |= pViewItem->GetAcceptsCopy();
		isAnyItemDeletable |= pViewItem->GetAcceptsDeletion();
		isAnyItemDeactivatable |= pViewItem->GetAcceptsDeactivation();

		if (isAnyItemDeactivatable && isAnyItemCopyable && isAnyItemDeletable)
		{
			break;
		}
	}

	ICommandManager* pCommandManager = GetIEditor()->GetICommandManager();
	QMenu menu;

	if (isAnyItemCopyable)
	{
		menu.addAction(pCommandManager->GetAction("general.copy"));
		// TODO: Not yet implemented
		//menu.addAction(pCommandManager->GetAction("general.cut"));
	}

	if (isAnyItemDeletable)
	{
		menu.addAction(pCommandManager->GetAction("general.delete"));
	}

	QAction* pSeperator = menu.addSeparator();
	if (!PopulateSelectionContextMenu(m_selectedItems, menu))
	{
		menu.removeAction(pSeperator);
	}

	if (isAnyItemDeactivatable)
	{
		menu.addSeparator();

		QAction* pActivateAction = menu.addAction(QObject::tr("Activate"));
		QObject::connect(pActivateAction, &QAction::triggered, this, [this](bool isChecked)
			{
				for (CAbstractNodeGraphViewModelItem* pViewItem : m_selectedItems)
				{
				  if (CAbstractNodeItem* pNodeItem = CAbstractNodeGraphViewModelItem::Cast<CAbstractNodeItem>(pViewItem))
				  {
				    pNodeItem->SetDeactivated(false);
				  }
				}
			});

		QAction* pDeactivateAction = menu.addAction(QObject::tr("Deactivate"));
		QObject::connect(pDeactivateAction, &QAction::triggered, this, [this](bool isChecked)
			{
				for (CAbstractNodeGraphViewModelItem* pViewItem : m_selectedItems)
				{
				  if (CAbstractNodeItem* pNodeItem = CAbstractNodeGraphViewModelItem::Cast<CAbstractNodeItem>(pViewItem))
				  {
				    pNodeItem->SetDeactivated(true);
				  }
				}
			});
	}

	const QPoint parent = mapFromGlobal(QPoint(screenPos.x(), screenPos.y()));
	const QPointF sceenPos = mapToScene(parent);
	m_contextMenuScenePos = sceenPos;

	menu.exec(QPoint(screenPos.x(), screenPos.y()));
}

void CNodeGraphView::ShowContentEditingPopup(const CTextWidget& textWidget, const QPoint& pos, const QSize& size, QString text, const QFont& font, bool multiline)
{
	SetAction(eAction_ShowContentEditPopup);

	CTextEdit* pTextEdit = new CTextEdit(textWidget);
	pTextEdit->setWordWrapMode(multiline ? QTextOption::WrapAnywhere : QTextOption::NoWrap);
	pTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	pTextEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	pTextEdit->setFocus();
	pTextEdit->selectAll();
	pTextEdit->setCurrentFont(font);
	pTextEdit->setText(text);
	pTextEdit->SignalEditingFinished.Connect(this, &CNodeGraphView::OnEditingContentPopupFinish);

	if (!m_pContentEditPopup)
	{
		m_pContentEditPopup.reset(new QPopupWidget("ContentEditPopup", pTextEdit));
	}
	else
	{
		m_pContentEditPopup->SetContent(pTextEdit);
	}

	m_pContentEditPopup->setFixedSize(size);
	m_pContentEditPopup->ShowAt(pos);
}

CPinWidget* CNodeGraphView::GetPossibleConnectionTarget(QPointF scenePos)
{
	const CAbstractPinItem& pinItem = m_pNewConnectionBeginPin->GetItem();
	static qreal radius = 100.0f;

	QPainterPath path;
	path.addEllipse(scenePos, radius, radius);
	QList<QGraphicsItem*> itemList = m_pScene->items(path, Qt::ItemSelectionMode::IntersectsItemBoundingRect);

	CPinWidget* pClosestPinWidget = nullptr;
	qreal closestPinDistance = radius + 1.0;
	for (QGraphicsItem* pGraphicsItem : itemList)
	{
		CPinWidget* pPinWidget = CNodeGraphViewGraphicsWidget::Cast<CPinWidget>(pGraphicsItem);
		if (pPinWidget && pPinWidget != m_pNewConnectionBeginPin)
		{
			if (pPinWidget->GetItem().CanConnect(&pinItem))
			{
				qreal dist = (pPinWidget->GetConnectionPoint().GetPosition() - scenePos).manhattanLength();
				if (dist < closestPinDistance)
				{
					closestPinDistance = dist;
					pClosestPinWidget = pPinWidget;
				}
			}
		}
	}

	return pClosestPinWidget;
}

CGroupWidget* CNodeGraphView::GetGroupCandidateForPlacement(const GraphViewWidgetSet& selection)
{
	uint32 candidateItemCount = 0;

	QPointF selectionAA;
	QPointF selectionBB;

	// calculate selection AABB
	for (auto pItem : selection)
	{
		if (QRectF(selectionAA, selectionBB).isEmpty())
		{
			selectionAA = pItem->geometry().topLeft();
			selectionBB = pItem->geometry().bottomRight();
		}

		QPointF itemAA = pItem->geometry().topLeft();
		QPointF itemBB = pItem->geometry().bottomRight();

		selectionAA.setX(std::min(selectionAA.x(), itemAA.x()));
		selectionAA.setY(std::min(selectionAA.y(), itemAA.y()));

		selectionBB.setX(std::max(selectionBB.x(), itemBB.x()));
		selectionBB.setY(std::max(selectionBB.y(), itemBB.y()));
	}

	QRectF selectionAABB(selectionAA, selectionBB);

	const QList<QGraphicsItem*> itemList = m_pScene->items(selectionAABB);
	for (int32 i = 0; i < itemList.size(); i++)
	{
		CGroupWidget* pGroupWidget = CNodeGraphViewGraphicsWidget::Cast<CGroupWidget>(itemList[i]);
		if (pGroupWidget)
		{
			pGroupWidget->ClearPendingSelection();
			QRectF groupRect = pGroupWidget->geometry();

			for (auto pItem : selection)
			{
				QRectF itemRect = pItem->geometry();
				float itemArea = itemRect.width() * itemRect.height();

				QRectF intersectRect = groupRect.intersected(itemRect);
				float intersectArea = intersectRect.width() * intersectRect.height();

				candidateItemCount = (intersectArea > itemArea * 0.5f - 0.00001f) ? candidateItemCount + 1 : candidateItemCount;
			}
		}

		if (candidateItemCount >= selection.size() * 0.5f)
		{
			return pGroupWidget;
		}

		candidateItemCount = 0;
	}

	return nullptr;
}

bool CNodeGraphView::IsSelected(const CNodeGraphViewGraphicsWidget* pItem) const
{
	auto result = m_selectedWidgets.find(const_cast<CNodeGraphViewGraphicsWidget*>(pItem));
	return result != m_selectedWidgets.end();
}

bool CNodeGraphView::IsNodeSelected(const CAbstractNodeItem& node) const
{
	if (const CNodeWidget* pNodeWidget = GetNodeWidget(node))
	{
		return IsSelected(pNodeWidget);
	}
	return false;
}

bool CNodeGraphView::IsScreenPositionPresented(const QPoint& screenPos) const
{
	const QPoint parent = mapFromGlobal(screenPos);
	const QPointF sceenPos = mapToScene(parent);
	const QRectF viewRect = mapToScene(viewport()->geometry()).boundingRect();

	return viewRect.contains(sceenPos);
}

// TODO: Get rid of this!
void CNodeGraphView::RegisterConnectionEventListener(CryGraphEditor::IConnectionEventsListener* pListener)
{
	m_connectionEventListeners.insert(pListener);
}

void CNodeGraphView::RemoveConnectionEventListener(CryGraphEditor::IConnectionEventsListener* pListener)
{
	m_connectionEventListeners.erase(pListener);
}

void CNodeGraphView::SendBeginCreateConnection(QWidget* pSender, CryGraphEditor::SBeginCreateConnectionEventArgs& args)
{
	for (CryGraphEditor::IConnectionEventsListener* pListener : m_connectionEventListeners)
	{
		pListener->OnBeginCreateConnection(pSender, args);
	}
}

void CNodeGraphView::SendEndCreateConnection(QWidget* pSender, CryGraphEditor::SEndCreateConnectionEventArgs& args)
{
	for (CryGraphEditor::IConnectionEventsListener* pListener : m_connectionEventListeners)
	{
		pListener->OnEndCreateConnection(pSender, args);
	}
}

// ~TODO

namespace Private_NodeGraphView
{

// Same as QGraphicsProxyWidget, but calls deleteLater() on the embedded widget, instead directly
// deleting it.
// #TODO: After stabilization, merge this class with CProxyWidget in ProxyWidget.cpp.
class CGraphicsProxyWidget : public QGraphicsProxyWidget
{
public:
	CGraphicsProxyWidget(QGraphicsItem* pParent = nullptr, Qt::WindowFlags wFlags = Qt::WindowFlags())
		: QGraphicsProxyWidget(pParent, wFlags)
	{
	}

	virtual ~CGraphicsProxyWidget() override
	{
		QWidget* const w = widget();
		if (w)
		{
			setWidget(nullptr);
			w->deleteLater();
		}
	}
};

} // Private_NodeGraphView

EDITOR_COMMON_API QGraphicsProxyWidget* AddWidget(QGraphicsScene* pScene, QWidget* pWidget)
{
	using namespace Private_NodeGraphView;

	CRY_ASSERT(pScene && pWidget);
	CGraphicsProxyWidget* const pProxy = new CGraphicsProxyWidget();
	pProxy->setWidget(pWidget);
	return pProxy;
}

}
