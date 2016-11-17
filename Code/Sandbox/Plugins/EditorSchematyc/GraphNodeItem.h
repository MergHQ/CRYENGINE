// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <NodeGraph/AbstractNodeItem.h>

#include "GraphPinItem.h"
#include "NodeGraphRuntimeContext.h"

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

	virtual const CryGraphEditor::PinItemArray& GetPinItems() const override    { return m_pins; };
	virtual QString                             GetName() const override        { return m_shortName; }

	virtual QString                             GetToolTipText() const override { return m_fullQualifiedName; }

	virtual CryGraphEditor::CAbstractPinItem*   GetPinItemById(QVariant id) const;
	// CryGraphEditor::CAbstractNodeItem

	CPinItem*                    GetPinItemById(CPinId id) const;

	Schematyc::IScriptGraphNode& GetScriptElement() const { return m_scriptNode; }
	Schematyc::SGUID             GetGUID() const;

	void                         Refresh(bool bForceRefresh);

protected:
	void LoadFromScriptElement();
	void Validate();

private:
	QString                      m_shortName;
	QString                      m_fullQualifiedName;
	CryGraphEditor::PinItemArray m_pins;

	Schematyc::IScriptGraphNode& m_scriptNode;

	bool                         m_isDirty;
};

}
