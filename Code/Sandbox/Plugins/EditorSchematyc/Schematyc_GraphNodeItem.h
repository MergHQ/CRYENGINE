// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <NodeGraph/AbstractNodeItem.h>

#include "Schematyc_GraphPinItem.h"
#include "Schematyc_NodeGraphRuntimeContext.h"

namespace Schematyc {

struct IScriptGraphNode;

}

namespace CrySchematycEditor {

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

	virtual const CryGraphEditor::PinItemArray& GetPinItems() const { return m_pins; };
	virtual const QString                       GetName() const     { return m_name; }

	virtual CryGraphEditor::CAbstractPinItem*   GetPinItemById(QVariant id) const;
	// CryGraphEditor::CAbstractNodeItem

	CPinItem*                    GetPinItemById(CPinId id) const;

	Schematyc::IScriptGraphNode& GetScriptElement() const { return m_scriptNode; }
	Schematyc::SGUID             GetGUID() const;

protected:
	void LoadFromScriptElement();

private:
	QString                      m_name;
	CryGraphEditor::PinItemArray m_pins;

	Schematyc::IScriptGraphNode& m_scriptNode;
};

}
