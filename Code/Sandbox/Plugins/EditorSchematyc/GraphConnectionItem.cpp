// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "GraphConnectionItem.h"

#include "VariableStorage/AbstractVariableTypesModel.h"

#include <Schematyc/Script/IScriptGraph.h>
#include <NodeGraph/ConnectionWidget.h>

#include <QColor>

namespace CrySchematycEditor {

CConnectionItem::CConnectionItem(Schematyc::IScriptGraphLink& scriptGraphLink, CPinItem& sourcePin, CPinItem& targetPin, CryGraphEditor::CNodeGraphViewModel& model)
	: CAbstractConnectionItem(model)
	, m_scriptGraphLink(scriptGraphLink)
	, m_sourcePin(sourcePin)
	, m_targetPin(targetPin)
{
	// TODO: This should happen in CAbstractConnectionItem constructor.
	m_sourcePin.AddConnection(*this);
	m_targetPin.AddConnection(*this);
	// ~TODO

	switch (m_sourcePin.GetPinType())
	{
	case EPinType::Execution:
	case EPinType::Signal:
		SetLineWidth(3.0f);
		break;
	default:
		break;
	}
}

CConnectionItem::~CConnectionItem()
{
	// TODO: This should happen in CAbstractConnectionItem destructor.
	m_sourcePin.RemoveConnection(*this);
	m_targetPin.RemoveConnection(*this);
	// ~TODO
}

CryGraphEditor::CConnectionWidget* CConnectionItem::CreateWidget(CryGraphEditor::CNodeGraphView& view)
{
	return new CryGraphEditor::CConnectionWidget(this, view);
}

QVariant CConnectionItem::GetId() const
{
	return QVariant::fromValue(reinterpret_cast<quintptr>(&m_scriptGraphLink));
}

bool CConnectionItem::HasId(QVariant id) const
{
	const Schematyc::IScriptGraphLink* pGraphLink = reinterpret_cast<Schematyc::IScriptGraphLink*>(id.value<quintptr>());
	return (&m_scriptGraphLink == pGraphLink);
}

const QColor& CConnectionItem::GetSourceColor() const
{
	return m_sourcePin.GetColor();
}

const QColor& CConnectionItem::GetTargetColor() const
{
	return m_targetPin.GetColor();
}

}
