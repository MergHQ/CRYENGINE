// Copyright 2001-2016 Crytek GmbH. All rights reserved.
#pragma once

#include <QWidget>

#include "CryIcon.h"
#include "CryQtAPI.h"
#include "CryQtCompatibility.h"
#include <memory>

class QPushButton;
class QLabel;

//! This is a reusable UI component used for grouping widgets together with a title and the group can be collapsed
class CRYQT_API QCollapsibleFrame : public QWidget
{
	Q_OBJECT;

	friend class CCollapsibleFrameHeader;
public:
	explicit QCollapsibleFrame(const QString& title = QString(), QWidget* pParent = nullptr);
	virtual ~QCollapsibleFrame();

	QWidget* GetWidget() const;
	QWidget* GetDragHandler() const;
	void     SetWidget(QWidget* pWidget);
	void	 SetClosable(bool closable);
	bool	 Closable() const;
	void     SetTitle(const QString& title);
	void	 SetCollapsed(bool collapsed);
	bool	 Collapsed() const;

protected:
	virtual void paintEvent(QPaintEvent*) override;

private:
	struct SImplementation;
	std::unique_ptr<SImplementation> m_pImpl;
protected slots:
	void OnCloseRequested();

signals:
	void CloseRequested(QCollapsibleFrame* caller);
};

// This class is an implementation detail of QCollapsibleFrame and
// also used so that the frame header is styleable. It should not
// be used directly which is why it has non-public members only.
// This class is not placed in the cpp file because then the moc
// file would have to be included. It cannot be nested in QCollapsibleFrame
// either because moc does not allow that
class CCollapsibleFrameHeader : public QWidget
{
	Q_OBJECT;

	Q_PROPERTY(QSize iconSize READ GetIconSize WRITE SetIconSize)

	friend class QCollapsibleFrame;
private:
	CCollapsibleFrameHeader(const QString& title, QCollapsibleFrame* pParentCollapsible);

	QSize GetIconSize() const;
	void  SetIconSize(const QSize& iconSize);
	void  SetClosable(bool closable);
	bool  Closable() const;
protected:
	virtual void paintEvent(QPaintEvent*) override;
	virtual bool eventFilter(QObject* pWatched, QEvent* pEvent) override;
protected Q_SLOTS:
	void OnCollapseButtonClick();
private:
	struct SImplementation;
	std::unique_ptr<SImplementation> m_pImpl;
};
