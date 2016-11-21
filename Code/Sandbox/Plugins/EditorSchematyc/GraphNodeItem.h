// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <NodeGraph/AbstractNodeItem.h>
#include <NodeGraph/NodeWidget.h>

#include "GraphPinItem.h"
#include "NodeGraphRuntimeContext.h"

#include <unordered_map>

class QPixmap;
class QIcon;

namespace Schematyc {

struct IScriptGraphNode;

}

namespace CrySchematycEditor {

typedef CryGraphEditor::CIconMap<uint32 /* name hash */> IconMap;

class CNodeIconMap : public IconMap
{
public:
	static QPixmap* GetNodeWidgetIcon(const char* szStyleId);
	static QIcon*   GetMenuIcon(const char* szStyleId);

protected:
	CNodeIconMap();
	~CNodeIconMap();

	void LoadIcons();
	void AddIcon(const char* szStyleId, const char* szIcon, QColor color);

private:
	static CNodeIconMap s_Instance;
	QPixmap*            m_pDefaultWidgetIcon;
	std::unordered_map<uint32 /* name hash */, QIcon*> m_menuIcons;
	QIcon*              m_pDefaultMenuIcon;
};

class CNodeTypeIcon : public CryGraphEditor::CNodeHeaderIcon
{
public:
	CNodeTypeIcon(CryGraphEditor::CNodeWidget& nodeWidget);
	virtual ~CNodeTypeIcon();
};

class CNodeItem : public CryGraphEditor::CAbstractNodeItem
{
public:
	CNodeItem(Schematyc::IScriptGraphNode& scriptNode, CryGraphEditor::CNodeGraphViewModel& model);
	virtual ~CNodeItem();

	// CryGraphEditor::CAbstractNodeItem
	virtual CryGraphEditor::CNodeWidget*        CreateWidget(CryGraphEditor::CNodeGraphView& view) override;
	virtual void                                Serialize(Serialization::IArchive& archive) override;

	virtual QPointF                             GetPosition() const override;
	virtual void                                SetPosition(QPointF position) override;

	virtual QVariant                            GetId() const override;
	virtual bool                                HasId(QVariant id) const override;

	virtual QVariant                            GetTypeId() const override;

	virtual const CryGraphEditor::PinItemArray& GetPinItems() const override    { return m_pins; };
	virtual QString                             GetName() const override        { return m_shortName; }

	virtual QString                             GetToolTipText() const override { return m_fullQualifiedName; }

	virtual CryGraphEditor::CAbstractPinItem*   GetPinItemById(QVariant id) const;
	// CryGraphEditor::CAbstractNodeItem

	CPinItem*                    GetPinItemById(CPinId id) const;

	Schematyc::IScriptGraphNode& GetScriptElement() const { return m_scriptNode; }
	Schematyc::SGUID             GetGUID() const;

	const char*                  GetStyleId();
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

	bool                         m_isDirty;
};

}
