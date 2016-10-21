// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <NodeGraph/AbstractConnectionItem.h>

#include "Schematyc_GraphPinItem.h"

class QColor;

namespace Schematyc {

struct IScriptGraphLink;

}

namespace CrySchematycEditor {

class CConnectionItem : public CryGraphEditor::CAbstractConnectionItem
{
public:
	CConnectionItem(Schematyc::IScriptGraphLink& scriptGraphLink, CPinItem& sourcePin, CPinItem& targetPin, CryGraphEditor::CNodeGraphViewModel& model);
	virtual ~CConnectionItem();

	// CryGraphEditor::CAbstractConnectionItem
	virtual CryGraphEditor::CConnectionWidget* CreateWidget(CryGraphEditor::CNodeGraphView& view) override;

	virtual CryGraphEditor::CAbstractPinItem&  GetSourcePinItem() const override { return m_sourcePin; }
	virtual CryGraphEditor::CAbstractPinItem&  GetTargetPinItem() const override { return m_targetPin; }

	virtual QVariant                           GetId() const override;
	virtual bool                               HasId(QVariant id) const override;

	virtual const QColor& GetSourceColor() const override;
	virtual const QColor& GetTargetColor() const override;
	// ~CryGraphEditor::CAbstractConnectionItem

private:
	Schematyc::IScriptGraphLink& m_scriptGraphLink;
	CPinItem&                    m_sourcePin;
	CPinItem&                    m_targetPin;
};

}
