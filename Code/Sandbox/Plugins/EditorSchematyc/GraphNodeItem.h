// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <NodeGraph/AbstractNodeItem.h>
#include <NodeGraph/NodeWidget.h>
#include <NodeGraph/NodeHeaderIconWidget.h>
#include <NodeGraph/NodeGraphViewStyle.h>

#include "GraphPinItem.h"
#include "NodeGraphRuntimeContext.h"

#include <unordered_map>

class QPixmap;
class QIcon;

namespace Schematyc {

struct IScriptGraphNode;

}

namespace CrySchematycEditor {

// TODO: Move in its own files.
class CNodeStyle : public CryGraphEditor::CNodeWidgetStyle
{
public:
	CNodeStyle()
		: CNodeWidgetStyle()
		, m_pMenuIcon(nullptr)
	{}

	CNodeStyle(const char* szStyleId)
		: CNodeWidgetStyle(szStyleId)
		, m_pMenuIcon(nullptr)
	{}

	~CNodeStyle()
	{
		delete m_pMenuIcon;
	}

	const QIcon* GetMenuIcon() const       { return m_pMenuIcon; }
	void         SetMenuIcon(QIcon* pIcon) { m_pMenuIcon = pIcon; }

private:
	const QIcon* m_pMenuIcon;
};

class CNodeStyles : protected CryGraphEditor::CNodeStyleMap
{
	static const uint32 s_NumStyles = 10;

public:
	static const CNodeStyle* GetStyleById(const char* szStyleId);

protected:
	CNodeStyles();

	void LoadIcons();
	void CreateStyle(const char* szStyleId, const char* szIcon, QColor color);

private:
	static CNodeStyles s_instance;

	CNodeStyle         m_nodeStyles[s_NumStyles];
	uint32             m_createdStyles;
};

class CNodeTypeIcon : public CryGraphEditor::CNodeHeaderIcon
{
public:
	CNodeTypeIcon(CryGraphEditor::CNodeWidget& nodeWidget);
	virtual ~CNodeTypeIcon();
};
// ~TODO

class CNodeItem : public CryGraphEditor::CAbstractNodeItem
{
public:
	CNodeItem(Schematyc::IScriptGraphNode& scriptNode, CryGraphEditor::CNodeGraphViewModel& model);
	virtual ~CNodeItem();

	// CryGraphEditor::CAbstractNodeItem
	virtual CryGraphEditor::CNodeWidget*            CreateWidget(CryGraphEditor::CNodeGraphView& view) override;
	virtual const CryGraphEditor::CNodeWidgetStyle* GetStyle() const override;
	virtual void                                    Serialize(Serialization::IArchive& archive) override;

	virtual QPointF                                 GetPosition() const override;
	virtual void                                    SetPosition(QPointF position) override;

	virtual QVariant                                GetId() const override;
	virtual bool                                    HasId(QVariant id) const override;

	virtual QVariant                                GetTypeId() const override;

	virtual const CryGraphEditor::PinItemArray&     GetPinItems() const override    { return m_pins; };
	virtual QString                                 GetName() const override        { return m_shortName; }

	virtual QString                                 GetToolTipText() const override { return m_fullQualifiedName; }

	virtual CryGraphEditor::CAbstractPinItem*       GetPinItemById(QVariant id) const;
	// CryGraphEditor::CAbstractNodeItem

	CPinItem*                    GetPinItemById(CPinId id) const;

	Schematyc::IScriptGraphNode& GetScriptElement() const { return m_scriptNode; }
	Schematyc::SGUID             GetGUID() const;

	const char*                  GetStyleId() const;
	void                         Refresh(bool forceRefresh = false);

protected:
	void LoadFromScriptElement();
	void RefreshName();
	void Validate();

private:
	QString                      m_shortName;
	QString                      m_fullQualifiedName;
	CryGraphEditor::PinItemArray m_pins;

	Schematyc::IScriptGraphNode& m_scriptNode;
	QColor                       m_headerTextColor;
	bool                         m_isDirty;
};

}
