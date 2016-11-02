// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <QWidget>
#include <QLabel>
#include <QSplitter>
#include <QList>
#include <QRubberBand>
#include <QTimer>

#include "QToolWindowManagerCommon.h"
#include "IToolWindowArea.h"
#include "IToolWindowWrapper.h"

class IToolWindowDragHandler;
class QToolWindowManager;

class QTOOLWINDOWMANAGER_EXPORT QToolWindowManagerClassFactory
{
public:
	virtual IToolWindowArea* createArea(QToolWindowManager* manager, QWidget *parent = 0);
	virtual IToolWindowWrapper* createWrapper(QToolWindowManager* manager);
	virtual IToolWindowDragHandler* createDragHandler(QToolWindowManager* manager);
	virtual QSplitter* createSplitter(QToolWindowManager* manager);
};

class QTOOLWINDOWMANAGER_EXPORT QToolWindowManager: public QWidget
{
	Q_OBJECT

public:
	QToolWindowManager(QWidget *parent = 0, QVariant config = QVariant(), QToolWindowManagerClassFactory* factory = nullptr);
	virtual ~QToolWindowManager();
	bool empty() { return m_areas.empty(); };

	void removeArea(IToolWindowArea* area);
	void removeWrapper(IToolWindowWrapper* wrapper);

	bool ownsArea(IToolWindowArea* area) { return m_areas.contains(area); }
	bool ownsWrapper(IToolWindowWrapper* wrapper) { return m_wrappers.contains(wrapper); }
	bool ownsToolWindow(QWidget* toolWindow) { return m_toolWindows.contains(toolWindow); }

	IToolWindowWrapper* draggedWrapper() { return m_draggedWrapper; }

	const QVariantMap& config() const { return m_config; }

	void startDrag(const QList<QWidget*> &toolWindows, IToolWindowArea* area);
	void startDrag(IToolWindowWrapper* wrapper);

	class AreaReference
	{
	public:
		enum eType
		{
			Top = 0,
			Bottom,
			Left,
			Right,
			HSplitTop,
			HSplitBottom,
			VSplitLeft,
			VSplitRight,
			Combine,
			Floating,
			Drag,
			Hidden
		};

	private:
		eType type;
	public:
		AreaReference(eType& t) : type(t){ }
		AreaReference(int t) : type((eType)t) { }
		inline operator eType () const { return type; }

		inline bool isOuter() const { return type <= Right; }
		inline bool requiresSplit() const { return type < Combine;	}
		Qt::Orientation splitOrientation() const
		{
			switch (type)
			{
			case QToolWindowManager::AreaReference::Top:
			case QToolWindowManager::AreaReference::Bottom:
			case QToolWindowManager::AreaReference::HSplitTop:
			case QToolWindowManager::AreaReference::HSplitBottom:
				return Qt::Vertical;
			case QToolWindowManager::AreaReference::Left:
			case QToolWindowManager::AreaReference::Right:
			case QToolWindowManager::AreaReference::VSplitLeft:
			case QToolWindowManager::AreaReference::VSplitRight:
				return Qt::Horizontal;
			case QToolWindowManager::AreaReference::Combine:
			case QToolWindowManager::AreaReference::Floating:
			case QToolWindowManager::AreaReference::Hidden:
			default:
				return (Qt::Orientation)0; // Invalid
			}
		}
	};

	struct AreaTarget
	{
		IToolWindowArea* area;
		AreaReference reference;
		int index;
		QRect geometry;

		AreaTarget() : area(nullptr), reference(AreaReference::Combine), index(-1), geometry(){};
		AreaTarget(AreaReference::eType reference, int index = -1, QRect geometry = QRect()) : area(nullptr), reference(reference), index(index), geometry(geometry) {};
		AreaTarget(IToolWindowArea* area, AreaReference::eType reference, int index = -1, QRect geometry = QRect()) : area(area), reference(reference), index(index), geometry(geometry) {};
	};

	void addToolWindow(QWidget* toolWindow, const AreaTarget& target) { addToolWindow(toolWindow, target.area, target.reference, target.index, target.geometry); }
	void addToolWindow(QWidget* toolWindow, IToolWindowArea* area = nullptr, AreaReference reference = AreaReference::Combine, int index = -1, QRect geometry = QRect());
	void addToolWindows(const QList<QWidget*>& toolWindows, const AreaTarget& target) { addToolWindows(toolWindows, target.area, target.reference, target.index, target.geometry); }
	void addToolWindows(const QList<QWidget*>& toolWindows, IToolWindowArea* area, AreaReference reference = AreaReference::Combine, int index = -1, QRect geometry = QRect());
	
	void moveToolWindow(QWidget* toolWindow, const AreaTarget& target) { addToolWindow(toolWindow, target.area, target.reference, target.index, target.geometry); }
	void moveToolWindow(QWidget* toolWindow, IToolWindowArea* area, AreaReference reference = AreaReference::Combine, int index = -1, QRect geometry = QRect());
	void moveToolWindows(const QList<QWidget*>& toolWindows, const AreaTarget& target) { addToolWindows(toolWindows, target.area, target.reference, target.index, target.geometry); }
	void moveToolWindows(const QList<QWidget*>& toolWindows, IToolWindowArea* area, AreaReference reference = AreaReference::Combine, int index = -1, QRect geometry = QRect());
	
	bool releaseToolWindow(QWidget *toolWindow, bool allowClose = false);
	bool releaseToolWindows(QList<QWidget*> toolWindows, bool allowClose = false);

	IToolWindowArea *areaOf(QWidget *toolWindow);

	bool isWrapper(QWidget* w)
	{
		if (!w)
			return false;
		// Allow a single non-splitter between widget and wrapper
		return qobject_cast<IToolWindowWrapper*>(w) || (!qobject_cast<QSplitter*>(w) && qobject_cast<IToolWindowWrapper*>(w->parentWidget()));
	}
	bool isMainWrapper(QWidget* w)
	{
		return isWrapper(w) && (qobject_cast<IToolWindowWrapper*>(w) == m_mainWrapper || (qobject_cast<IToolWindowWrapper*>(w->parentWidget()) == m_mainWrapper));
	}
	bool isFloatingWrapper(QWidget* w)
	{
		if (!isWrapper(w))
			return false;
		// Allow a single non-splitter between widget and wrapper
		if (qobject_cast<IToolWindowWrapper*>(w) && qobject_cast<IToolWindowWrapper*>(w) != m_mainWrapper)
			return true;
		return (!qobject_cast<QSplitter*>(w) && qobject_cast<IToolWindowWrapper*>(w->parentWidget()) && qobject_cast<IToolWindowWrapper*>(w->parentWidget()) != m_mainWrapper);
	}
	QList<QWidget*> toolWindows() { return m_toolWindows; }

	QVariant saveState();
	void restoreState(const QVariant &data);

	bool isAnyWindowActive();
	void updateDragPosition();
	void finishWrapperDrag();

	QVariantMap saveWrapperState(IToolWindowWrapper* wrapper);
	IToolWindowWrapper* restoreWrapperState(const QVariantMap &data, int stateFormat, IToolWindowWrapper* wrapper = nullptr);
	QVariantMap saveSplitterState(QSplitter *splitter);
	QSplitter *restoreSplitterState(const QVariantMap &data, int stateFormat);

	IToolWindowArea* createArea();
	IToolWindowWrapper* createWrapper();

	class QTOOLWINDOWMANAGER_EXPORT QTWMNotifyLock
	{
	public:
		QTWMNotifyLock(QToolWindowManager* parent, bool allowNotify=true);
		~QTWMNotifyLock();
		QTWMNotifyLock(QTWMNotifyLock&&) = default;
	private:
		QToolWindowManager* m_parent;
		bool m_notify;
	};

	QTWMNotifyLock getNotifyLock(bool allowNotify = true) { return QTWMNotifyLock(this, allowNotify); }
	
	void hide() { m_mainWrapper->getWidget()->hide(); }

	void bringAllToFront();
	void bringToFront(QWidget* toolWindow);

signals:
	void toolWindowVisibilityChanged(QWidget* toolWindow, bool visible);

	// Notifies connected slots that the layout has changed. Should not normally be triggered directly; go through notifyLayoutChange instead to make sure signal is only fired if not suspended.
	void layoutChanged();

	// Requests that any tracking tooltip be updated with the given text and shown at the given position.
	// Will be called with a blank text if the tooltip should no longer be displayed.
	void updateTrackingTooltip(QString text, QPoint pos);

protected:
	virtual bool eventFilter(QObject *, QEvent *) Q_DECL_OVERRIDE;
	virtual bool event(QEvent* e) Q_DECL_OVERRIDE;

	bool tryCloseToolWindow(QWidget* toolWindow);

protected:
	QList<IToolWindowArea*> m_areas;
	QList<IToolWindowWrapper*> m_wrappers;
	QList<QWidget*> m_toolWindows;
	QList<QWidget*> m_draggedToolWindows;
	IToolWindowArea* m_lastArea;
	IToolWindowWrapper* m_mainWrapper;
	QRubberBand* m_preview;
	IToolWindowDragHandler* m_dragHandler;
	QVariantMap m_config;
	QToolWindowManagerClassFactory* m_factory;
	QTimer* m_raiseTimer;
	IToolWindowWrapper* m_draggedWrapper;
	int m_layoutChangeNotifyLocks;
	
	QSplitter *createSplitter();
	QWidget* splitArea(QWidget* area, AreaReference reference, QWidget* insertWidget = nullptr);

	IToolWindowArea* getClosestParentArea(QWidget* widget);
	IToolWindowArea* getFurthestParentArea(QWidget* widget);

	QString textForPosition(AreaReference reference);

	void simplifyLayout(bool clearMain = false);

	IToolWindowDragHandler* createDragHandler();

private:
	int m_closingWindow;

	void suspendLayoutNotifications() { m_layoutChangeNotifyLocks++; }
	void resumeLayoutNotifications()
	{
		if (m_layoutChangeNotifyLocks > 0)
		{
			m_layoutChangeNotifyLocks--;
		}
	}
	void notifyLayoutChange()
	{
		if (m_layoutChangeNotifyLocks == 0)
		{
			QMetaObject::invokeMethod(this, "layoutChanged", Qt::QueuedConnection);
			qApp->processEvents();
		}
	}


protected slots:
	void raiseCurrentArea();
};

// Preserves sizes as much as possible when a child is removed, only distributing the left over space to immediate neighbors.
class QTOOLWINDOWMANAGER_EXPORT QSizePreservingSplitter : public QSplitter
{
public:
	QSizePreservingSplitter(QWidget * parent = 0);
	QSizePreservingSplitter(Qt::Orientation orientation, QWidget * parent = 0);

protected:
	virtual void QSizePreservingSplitter::childEvent(QChildEvent *c);
};
