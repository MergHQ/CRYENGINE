// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "SceneModelCommon.h"

//! Scene model that allows selection of a single node.
class CSceneModelSingleSelection : public CSceneModelCommon
{
public:
	typedef std::function<void (const FbxTool::SNode*)> SetNode;
	typedef std::function<const FbxTool::SNode*()>      GetNode;

	void SetNodeAccessors(const SetNode& setNode, const GetNode& getNode)
	{
		m_setNode = setNode;
		m_getNode = getNode;
	}

	// QAbstractItemModel implementation.
	int           columnCount(const QModelIndex& index) const override;
	QVariant      data(const QModelIndex& index, int role) const override;
	QVariant      headerData(int column, Qt::Orientation orientation, int role) const override;
	Qt::ItemFlags flags(const QModelIndex& modelIndex) const override;
	bool          setData(const QModelIndex& index, const QVariant& value, int role) override;
private:
	SetNode m_setNode;
	GetNode m_getNode;
};
