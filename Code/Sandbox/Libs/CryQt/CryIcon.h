// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QIcon>
#include <QMap>
#include <QPainter>
#include <QString>

#include <QIconEngine>
typedef QIconEngine CryQtIconEngine;

#include "CryQtAPI.h"

struct CryPixmapIconEngineEntry
{
	CryPixmapIconEngineEntry() : mode(QIcon::Normal), state(QIcon::Off){}
	CryPixmapIconEngineEntry(const QPixmap& pm, QIcon::Mode m = QIcon::Normal, QIcon::State s = QIcon::Off)
		: pixmap(pm), size(pm.size()), mode(m), state(s){}
	CryPixmapIconEngineEntry(const QString& file, const QSize& sz = QSize(), QIcon::Mode m = QIcon::Normal, QIcon::State s = QIcon::Off)
		: fileName(file), size(sz), mode(m), state(s){}
	CryPixmapIconEngineEntry(const QString& file, const QImage& image, QIcon::Mode m = QIcon::Normal, QIcon::State s = QIcon::Off);
	QPixmap      pixmap;
	QString      fileName;
	QSize        size;
	QIcon::Mode  mode;
	QIcon::State state;
	bool         isNull() const { return (fileName.isEmpty() && pixmap.isNull()); }
};

inline CryPixmapIconEngineEntry::CryPixmapIconEngineEntry(const QString& file, const QImage& image, QIcon::Mode m, QIcon::State s)
	: fileName(file), size(image.size()), mode(m), state(s)
{
	pixmap.convertFromImage(image);
	// Reset the devicePixelRatio. The pixmap may be loaded from a @2x file,
	// but be used as a 1x pixmap by QIcon.
	pixmap.setDevicePixelRatio(1.0);
}

typedef QMap<QIcon::Mode, QBrush> CryIconColorMap;

class CRYQT_API CryPixmapIconEngine : public CryQtIconEngine
{
public:
	CryPixmapIconEngine(CryIconColorMap colorMap = CryIconColorMap());
	CryPixmapIconEngine(const CryPixmapIconEngine&);

	virtual void                      paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override;
	virtual QPixmap                   pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override;
	virtual CryPixmapIconEngineEntry* bestMatch(const QSize& size, QIcon::Mode mode, QIcon::State state, bool sizeOnly);
	virtual QSize                     actualSize(const QSize& size, QIcon::Mode mode, QIcon::State state) override;
	virtual void                      addPixmap(const QPixmap& pixmap, QIcon::Mode mode, QIcon::State state) override;
	virtual void                      addFile(const QString& fileName, const QSize& size, QIcon::Mode mode, QIcon::State state) override;

	virtual QString                   key() const override;
	virtual CryQtIconEngine*          clone() const override;
	virtual bool                      read(QDataStream& in) override;
	virtual bool                      write(QDataStream& out) const override;
	virtual void                      virtual_hook(int id, void* data) override;

private:
	CryPixmapIconEngineEntry* tryMatch(const QSize& size, QIcon::Mode mode, QIcon::State state);
	QBrush                    getBrush(QIcon::Mode mode);
	QVector<CryPixmapIconEngineEntry> pixmaps;
	CryIconColorMap                   m_colormap;
	friend class CryIcon;
};

class CRYQT_API CryIcon : public QIcon
{
public:
	explicit CryIcon(CryIconColorMap colorMap = CryIconColorMap());
	CryIcon(const QIcon& icon);
	explicit CryIcon(const QString& filename, CryIconColorMap colorMap = CryIconColorMap());
	explicit CryIcon(const QPixmap& pixmap, CryIconColorMap colorMap = CryIconColorMap());
	static void SetDefaultTint(QIcon::Mode mode, QBrush brush);
};
