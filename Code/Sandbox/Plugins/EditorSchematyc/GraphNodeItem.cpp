// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "GraphNodeItem.h"
#include "GraphPinItem.h"

#include <Schematyc/Script/IScriptGraph.h>

#include <NodeGraph/NodeWidget.h>
#include <NodeGraph/PinGridNodeContentWidget.h>

#include <QString>

namespace CrySchematycEditor {

CNodeIconMap CNodeIconMap::s_Instance;

CNodeIconMap::CNodeIconMap()
{

}

CNodeIconMap::~CNodeIconMap()
{
	delete m_pDefaultWidgetIcon;
	delete m_pDefaultMenuIcon;

	for (auto icon : m_menuIcons)
	{
		delete icon.second;
	}
}

void CNodeIconMap::LoadIcons()
{
	static bool iconsLoaded = false;
	if (iconsLoaded == false)
	{
		const CryGraphEditor::NodeStyle::EIconType iconType = CryGraphEditor::NodeStyle::IconType_Header;

		CryIcon* pDefaultIcon = new CryIcon("icons:schematyc/node_default.ico", {
				{ QIcon::Mode::Normal, QColor(255, 0, 0) }
		  });
		m_pDefaultMenuIcon = pDefaultIcon;
		m_pDefaultWidgetIcon = CryGraphEditor::NodeStyle::CreateIconPixmap(*pDefaultIcon, iconType);

		AddIcon("Core::FlowControl::Begin", "icons:schematyc/node_default.ico", QColor(26, 26, 26));
		AddIcon("Core::FlowControl::End", "icons:schematyc/node_default.ico", QColor(26, 26, 26));
		AddIcon("Core::FlowControl", "icons:schematyc/node_default.ico", QColor(97, 172, 236));
		AddIcon("Core::SendSignal", "icons:schematyc/node_default.ico", QColor(100, 193, 98));
		AddIcon("Core::ReceiveSignal", "icons:schematyc/node_default.ico", QColor(100, 193, 98));
		AddIcon("Core::Function", "icons:schematyc/node_default.ico", QColor(193, 98, 980));
		AddIcon("Core::Data", "icons:schematyc/node_default.ico", QColor(156, 98, 193));
		AddIcon("Core::Utility", "icons:schematyc/node_default.ico", QColor(153, 153, 153));
		AddIcon("Core::State", "icons:schematyc/node_default.ico", QColor(192, 193, 98));

		iconsLoaded = true;
	}
}

void CNodeIconMap::AddIcon(const char* szStyleId, const char* szIcon, QColor color)
{
	const uint32 nameHash = CCrc32::Compute(szStyleId);

	CryIcon* pIcon = new CryIcon(szIcon, {
			{ QIcon::Mode::Normal, color }
	  });
	m_menuIcons[nameHash] = pIcon;

	SetIcon(nameHash, *pIcon, CryGraphEditor::NodeStyle::IconType_Header);
}

QPixmap* CNodeIconMap::GetNodeWidgetIcon(const char* szStyleId)
{
	s_Instance.LoadIcons();
	if (szStyleId != nullptr)
	{
		const uint32 nameHash = CCrc32::Compute(szStyleId);
		QPixmap* pSpecialIcon = s_Instance.GetIcon(nameHash);
		if (pSpecialIcon)
			return pSpecialIcon;
	}
	return s_Instance.m_pDefaultWidgetIcon;
}

QIcon* CNodeIconMap::GetMenuIcon(const char* szStyleId)
{
	s_Instance.LoadIcons();
	if (szStyleId != nullptr)
	{
		const uint32 nameHash = CCrc32::Compute(szStyleId);
		auto result = s_Instance.m_menuIcons.find(nameHash);
		if (result != s_Instance.m_menuIcons.end())
		{
			return result->second;
		}
	}
	return s_Instance.m_pDefaultMenuIcon;
}

CNodeTypeIcon::CNodeTypeIcon(CryGraphEditor::CNodeWidget& nodeWidget)
	: CNodeHeaderIcon(nodeWidget)
{
	CNodeItem& nodeItem = static_cast<CNodeItem&>(nodeWidget.GetItem());
	SetDisplayIcon(CNodeIconMap::GetNodeWidgetIcon(nodeItem.GetStyleId()));
}

CNodeTypeIcon::~CNodeTypeIcon()
{
}

CNodeItem::CNodeItem(Schematyc::IScriptGraphNode& scriptNode, CryGraphEditor::CNodeGraphViewModel& model)
	: CAbstractNodeItem(model)
	, m_scriptNode(scriptNode)
	, m_isDirty(false)
{
	SetAcceptsRenaming(false);
	SetAcceptsDeactivation(false);

	LoadFromScriptElement();
}

CNodeItem::~CNodeItem()
{
	for (CryGraphEditor::CAbstractPinItem* pItem : m_pins)
		delete pItem;
}

CryGraphEditor::CNodeWidget* CNodeItem::CreateWidget(CryGraphEditor::CNodeGraphView& view)
{
	CryGraphEditor::CNodeWidget* pNodeWidget = new CryGraphEditor::CNodeWidget(*this, view);
	CryGraphEditor::CPinGridNodeContentWidget* pContent = new CryGraphEditor::CPinGridNodeContentWidget(*pNodeWidget, view);

	pNodeWidget->AddHeaderIcon(new CNodeTypeIcon(*pNodeWidget), CryGraphEditor::CNodeHeader::EIconSlot::Left);

	return pNodeWidget;
}

void CNodeItem::Serialize(Serialization::IArchive& archive)
{
	Schematyc::ESerializationPass serPass;
	if (archive.isEdit())
	{
		serPass = Schematyc::ESerializationPass::Edit;
	}
	else
	{
		serPass = archive.isInput() ? Schematyc::ESerializationPass::Load : Schematyc::ESerializationPass::Save;
	}

	Schematyc::SSerializationContextParams serializationContextParams(archive, serPass);
	Schematyc::ISerializationContextPtr pSerializationContext = gEnv->pSchematyc->CreateSerializationContext(serializationContextParams);

	m_scriptNode.Serialize(archive);
	if (archive.isInput())
	{
		m_isDirty = true;

		// TODO: This should happen in Serialize(...) function not here.
		gEnv->pSchematyc->GetScriptRegistry().ElementModified(m_scriptNode.GetGraph().GetElement());
		// ~TODO

		Validate();
	}
}

QPointF CNodeItem::GetPosition() const
{
	Vec2 pos = m_scriptNode.GetPos();
	return QPointF(pos.x, pos.y);
}

void CNodeItem::SetPosition(QPointF position)
{
	Vec2 pos(position.x(), position.y());
	m_scriptNode.SetPos(pos);

	CAbstractNodeItem::SetPosition(position);
}

QVariant CNodeItem::GetId() const
{
	return QVariant::fromValue(m_scriptNode.GetGUID());
}

bool CNodeItem::HasId(QVariant id) const
{
	return (id.value<Schematyc::SGUID>() == m_scriptNode.GetGUID());
}

QVariant CNodeItem::GetTypeId() const
{
	return QVariant::fromValue(m_scriptNode.GetTypeGUID());
}

CryGraphEditor::CAbstractPinItem* CNodeItem::GetPinItemById(QVariant id) const
{
	const CPinId pinId = id.value<CPinId>();
	return GetPinItemById(pinId);
}

CPinItem* CNodeItem::GetPinItemById(CPinId id) const
{
	if (id.IsInput())
	{
		for (CryGraphEditor::CAbstractPinItem* pAbstractPinItem : m_pins)
		{
			CPinItem* pPinItem = static_cast<CPinItem*>(pAbstractPinItem);
			if (pPinItem->IsInputPin() && pPinItem->GetPortId() == id.GetPortId())
			{
				return pPinItem;
			}
		}
	}
	else
	{
		for (CryGraphEditor::CAbstractPinItem* pAbstractPinItem : m_pins)
		{
			CPinItem* pPinItem = static_cast<CPinItem*>(pAbstractPinItem);
			if (pPinItem->IsOutputPin() && pPinItem->GetPortId() == id.GetPortId())
			{
				return pPinItem;
			}
		}
	}

	return nullptr;
}

Schematyc::SGUID CNodeItem::GetGUID() const
{
	return m_scriptNode.GetGUID();
}

const char* CNodeItem::GetStyleId()
{
	return m_scriptNode.GetStyleId();
}

void CNodeItem::Refresh(bool forceRefresh)
{
	if (m_isDirty || forceRefresh)
	{
		m_isDirty = false;

		// TODO: Just call RefreshLayout(...) here?!
		m_scriptNode.ProcessEvent(Schematyc::SScriptEvent(Schematyc::EScriptEventId::EditorRefresh));
		// ~TODO

		RefreshName();

		// We iterate through all pins and check what was removed/added.
		const uint32 numPins = m_scriptNode.GetInputCount() + m_scriptNode.GetOutputCount();

		CryGraphEditor::PinItemArray pins;
		pins.reserve(numPins);

		size_t numKeptPins = 0;

		const uint32 numInputPins = m_scriptNode.GetInputCount();
		for (uint32 i = 0; i < numInputPins; ++i)
		{
			CPinId pinId(m_scriptNode.GetInputId(i), CPinId::EType::Input);
			auto predicate = [pinId](CryGraphEditor::CAbstractPinItem* pAbstractPinItem) -> bool
			{
				CPinItem* pPinItem = static_cast<CPinItem*>(pAbstractPinItem);
				return (pPinItem && pinId == pPinItem->GetPinId());
			};

			auto result = std::find_if(m_pins.begin(), m_pins.end(), predicate);
			if (result == m_pins.end())
			{
				CPinItem* pPinItem = new CPinItem(i, EPinFlag::Input, *this, GetViewModel());
				pins.push_back(pPinItem);

				SignalPinAdded(*pPinItem);
			}
			else
			{
				CPinItem* pPinItem = static_cast<CPinItem*>(*result);
				pPinItem->UpdateWithNewIndex(i);
				pins.push_back(pPinItem);

				*result = nullptr;
				++numKeptPins;
			}
		}

		const uint32 numOutputPins = m_scriptNode.GetOutputCount();
		for (uint32 i = 0; i < numOutputPins; ++i)
		{
			CPinId pinId(m_scriptNode.GetOutputId(i), CPinId::EType::Output);
			auto predicate = [pinId](CryGraphEditor::CAbstractPinItem* pAbstractPinItem) -> bool
			{
				CPinItem* pPinItem = static_cast<CPinItem*>(pAbstractPinItem);
				return (pPinItem && pinId == pPinItem->GetPinId());
			};

			auto result = std::find_if(m_pins.begin(), m_pins.end(), predicate);
			if (result == m_pins.end())
			{
				CPinItem* pPinItem = new CPinItem(i, EPinFlag::Output, *this, GetViewModel());
				pins.push_back(pPinItem);

				SignalPinAdded(*pPinItem);
			}
			else
			{
				CPinItem* pPinItem = static_cast<CPinItem*>(*result);
				pPinItem->UpdateWithNewIndex(i);
				pins.push_back(pPinItem);

				*result = nullptr;
				++numKeptPins;
			}
		}

		m_pins.swap(pins);
		CRY_ASSERT(m_pins.size() == numPins);

		for (CryGraphEditor::CAbstractPinItem* pPinItem : pins)
		{
			if (pPinItem != nullptr)
			{
				SignalPinRemoved(*pPinItem);
				delete pPinItem;
			}
		}
	}
}

void CNodeItem::LoadFromScriptElement()
{
	// TODO: Just call RefreshLayout(...) here?!
	m_scriptNode.ProcessEvent(Schematyc::SScriptEvent(Schematyc::EScriptEventId::EditorRefresh)); // N.B. After the release of 5.3 we should figure out whether it's really necessary to refresh elements upon initial selection (as opposed to only refreshing when changes are made).
	// ~TODO

	RefreshName();

	const Vec2 pos = m_scriptNode.GetPos();
	SetPosition(QPoint(pos.x, pos.y));

	const QString styleId(GetStyleId());
	if (styleId != "Core::FlowControl::Begin" && styleId != "Core::FlowControl::End")
	{
		m_headerGradientColorL = CryGraphEditor::CNodeStyle::GetHeaderDefaultColorLeft();
		m_headerGradientColorR = CryGraphEditor::CNodeStyle::GetHeaderDefaultColorRight();
	}
	else
	{
		m_headerGradientColorL = QColor(97, 172, 236);
		m_headerGradientColorR = m_headerGradientColorL;
	}

	const uint32 numInputs = m_scriptNode.GetInputCount();
	for (uint32 i = 0; i < numInputs; ++i)
	{
		CryGraphEditor::CAbstractPinItem* pPinItem = new CPinItem(i, EPinFlag::Input, *this, GetViewModel());
		m_pins.push_back(pPinItem);
	}

	const uint32 numOutputs = m_scriptNode.GetOutputCount();
	for (uint32 i = 0; i < numOutputs; ++i)
	{
		CryGraphEditor::CAbstractPinItem* pPinItem = new CPinItem(i, EPinFlag::Output, *this, GetViewModel());
		m_pins.push_back(pPinItem);
	}

	Validate();
}

void CNodeItem::RefreshName()
{
	m_shortName = m_scriptNode.GetName();
	m_fullQualifiedName = QString("<b>FQNN: </b>%1").arg(m_shortName);

	CNodeItem::SignalNameChanged();
}

void CNodeItem::Validate()
{
	uint32 warningCount = 0;
	uint32 errorCount = 0;

	auto validateScriptNode = [&warningCount, &errorCount](Schematyc::EValidatorMessageType messageType, const char* szMessage)
	{
		switch (messageType)
		{
		case Schematyc::EValidatorMessageType::Warning:
			{
				++warningCount;
				break;
			}
		case Schematyc::EValidatorMessageType::Error:
			{
				++errorCount;
				break;
			}
		}
	};
	m_scriptNode.Validate(Schematyc::Validator::FromLambda(validateScriptNode));

	CAbstractNodeItem::SetWarnings(warningCount > 0);
	CAbstractNodeItem::SetErrors(errorCount > 0);

	SignalValidated(*this);
}

}
