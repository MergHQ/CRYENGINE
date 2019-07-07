// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EditorFramework/Editor.h"
#include <QAbstractItemModel>

namespace OutputModelAttributes
{
extern CItemModelAttributeEnumFunc s_SeverityAttribute;
extern CItemModelAttribute s_CodeAttribute;
extern CItemModelAttribute s_DescriptionAttribute;
extern CItemModelAttribute s_FileAttribute;
extern CItemModelAttribute s_LineAttribute;
}

class CCSharpOutputModel final
	: public QAbstractItemModel
{
public:
	enum EColumn : int32
	{
		eColumn_ErrorType = 0,
		eColumn_ErrorNumber,
		eColumn_ErrorText,
		eColumn_FileName,
		eColumn_Line,
		eColumn_COUNT
	};

public:
	CCSharpOutputModel(QObject* pParent = nullptr);

	// QAbstractItemModel
	virtual QVariant    data(const QModelIndex& index, int role = Qt::DisplayRole) const final;
	virtual QVariant    headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const final;
	virtual int         rowCount(const QModelIndex& parent = QModelIndex()) const final;

	virtual int         columnCount(const QModelIndex& parent = QModelIndex()) const final                { return eColumn_COUNT; }
	virtual QModelIndex parent(const QModelIndex& index) const final                                      { return QModelIndex(); }
	virtual bool        hasChildren(const QModelIndex& parent = QModelIndex()) const final                { return !parent.isValid(); }
	virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const final { return createIndex(row, column, nullptr); }
	// ~QAbstractItemModel

	void               ResetModel();
	static QStringList GetSeverityList();

private:
	const CItemModelAttribute* GetColumnAttribute(int column) const;
};