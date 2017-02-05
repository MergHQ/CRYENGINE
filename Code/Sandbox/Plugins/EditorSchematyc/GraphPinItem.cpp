// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "GraphPinItem.h"

#include "GraphNodeItem.h"
#include "GraphViewModel.h"

#include "VariableStorage/AbstractVariableTypesModel.h"

#include <Schematyc/Script/IScriptGraph.h>
#include <Schematyc/Reflection/TypeDesc.h>

#include <NodeGraph/PinWidget.h>
#include <NodeGraph/NodeGraphViewStyle.h>

#include <QColor>

#include <QtUtil.h>

namespace CrySchematycEditor {

// TODO: Move this into its own source files.
/*typedef CryGraphEditor::CIconArray<CryGraphEditor::CPinWidget::Icon_Count> PinIconMap;

   class CPinIconManager
   {
   struct SColors
   {
    QColor base;
    QColor selected;
    QColor highlighted;
    QColor deactivated;
   };

   public:
   static CPinIconManager& GetInstance()
   {
    static CPinIconManager instance;
    return instance;
   }

   const PinIconMap& GetExecutionIconMap() const
   {
    return m_ExecutionIcons;
   }

   const PinIconMap& GetSignalIconMap() const
   {
    return m_SignalIcons;
   }

   const PinIconMap& GetDataIconMap(const CDataTypeItem& dataType) const
   {
    auto result = m_DataIconsByType.find(dataType.GetGUID());
    if (result != m_DataIconsByType.end())
    {
      return result->second;
    }
    return m_UnkownTypeIcons;
   }

   private:
   CPinIconManager()
   {
    const char* szDataIconFile = "icons:Graph/Node_connection_circle.ico";
    const char* szExectionIconFile = "icons:Graph/Node_connection_arrow_R.ico";
    const char* szUnknownTypeIconFile = "icons:Graph/Node_connection_circle.ico";
    const char* szSignalIconFile = "icons:Graph/Node_connection_arrow_R.ico";

    SColors colors;
    colors.selected = CryGraphEditor::CGlobalStyle::GetSelectionColor();
    colors.highlighted = CryGraphEditor::CGlobalStyle::GetHighlightColor();
    colors.deactivated = CryGraphEditor::CNodeStyle::GetPinDefaultColor();

    CDataTypesModel& dataTypesModel = CDataTypesModel::GetInstance();
    for (uint32 i = 0; i < dataTypesModel.GetTypeItemsCount(); ++i)
    {
      CDataTypeItem* pDataTypeItem = dataTypesModel.GetTypeItemByIndex(i);
      if (pDataTypeItem)
      {
        colors.base = pDataTypeItem->GetColor();
        SetIconMap(m_DataIconsByType[pDataTypeItem->GetGUID()], colors, szDataIconFile);
      }
    }

    colors.base = QColor(200, 200, 200);
    SetIconMap(m_ExecutionIcons, colors, szExectionIconFile);
    SetIconMap(m_SignalIcons, colors, szSignalIconFile);

    colors.base = CryGraphEditor::CConnectionStyle::GetLineDefaultColor();
    SetIconMap(m_UnkownTypeIcons, colors, szUnknownTypeIconFile);
   }

   void SetIconMap(PinIconMap& map, const SColors& colors, const char* szIconFile)
   {
    map.SetIcon(CryGraphEditor::CPinWidget::Icon_Default, CryIcon(szIconFile, { { QIcon::Mode::Normal, colors.base } }), PinIconMap::PinIcon);
    map.SetIcon(CryGraphEditor::CPinWidget::Icon_Selected, CryIcon(szIconFile, { { QIcon::Mode::Normal, colors.selected } }), PinIconMap::PinIcon);

    const QColor shc = CryGraphEditor::CGlobalStyle::GetHighlightColor();

    QColor highlightColor;
    highlightColor.setRedF(1.0f - (1.0f - colors.base.redF()) * (1.0f - shc.redF()));
    highlightColor.setGreenF(1.0f - (1.0f - colors.base.greenF()) * (1.0f - shc.greenF()));
    highlightColor.setBlueF(1.0f - (1.0f - colors.base.blueF()) * (1.0f - shc.blueF()));
    highlightColor.setAlphaF(1.0f - (1.0f - colors.base.alphaF()) * (1.0f - shc.alphaF()));

    map.SetIcon(CryGraphEditor::CPinWidget::Icon_Highlighted, CryIcon(szIconFile, { { QIcon::Mode::Normal, highlightColor } }), PinIconMap::PinIcon);
    map.SetIcon(CryGraphEditor::CPinWidget::Icon_Deactivated, CryIcon(szIconFile, { { QIcon::Mode::Normal, colors.deactivated } }), PinIconMap::PinIcon);
   }

   private:
   static CPinIconManager                           s_Instance;

   PinIconMap                                       m_UnkownTypeIcons;
   PinIconMap                                       m_ExecutionIcons;
   PinIconMap                                       m_SignalIcons;
   std::unordered_map<Schematyc::SGUID, PinIconMap> m_DataIconsByType;
   };*/
// ~TODO

CPinItem::CPinItem(uint32 index, uint32 flags, CNodeItem& nodeItem, CryGraphEditor::CNodeGraphViewModel& model)
	: CAbstractPinItem(model)
	, m_flags(flags)
	, m_nodeItem(nodeItem)
{
	UpdateWithNewIndex(index);
}

CPinItem::~CPinItem()
{

}

CryGraphEditor::CPinWidget* CPinItem::CreateWidget(CryGraphEditor::CNodeWidget& nodeWidget, CryGraphEditor::CNodeGraphView& view)
{
	CryGraphEditor::CPinWidget* pPinWidget = new CryGraphEditor::CPinWidget(*this, nodeWidget, view, true);
	return pPinWidget;
}

CryGraphEditor::CAbstractNodeItem& CPinItem::GetNodeItem() const
{
	return static_cast<CryGraphEditor::CAbstractNodeItem&>(m_nodeItem);
}

QString CPinItem::GetTypeName() const
{
	return m_pDataTypeItem->GetName();
}

QVariant CPinItem::GetId() const
{
	return QVariant::fromValue(m_id);
}

bool CPinItem::HasId(QVariant id) const
{
	const CPinId otherId = id.value<CPinId>();
	return (otherId == m_id);
}

bool CPinItem::CanConnect(const CryGraphEditor::CAbstractPinItem* pOtherPin) const
{
	if (pOtherPin)
	{
		CNodeGraphViewModel& model = static_cast<CNodeGraphViewModel&>(GetViewModel());
		Schematyc::IScriptGraph& scriptGraph = model.GetScriptGraph();

		if (IsOutputPin())
		{
			if (pOtherPin->IsInputPin())
			{
				const Schematyc::SGUID sourceGuid = m_nodeItem.GetGUID();
				const Schematyc::CUniqueId sourceId = GetPortId();

				const Schematyc::SGUID targetGuid = static_cast<CNodeItem&>(pOtherPin->GetNodeItem()).GetGUID();
				const Schematyc::CUniqueId targetId = static_cast<const CPinItem*>(pOtherPin)->GetPortId();

				return scriptGraph.CanAddLink(sourceGuid, sourceId, targetGuid, targetId);
			}
		}
		else
		{
			if (pOtherPin->IsOutputPin())
			{
				const Schematyc::SGUID sourceGuid = static_cast<CNodeItem&>(pOtherPin->GetNodeItem()).GetGUID();
				const Schematyc::CUniqueId sourceId = static_cast<const CPinItem*>(pOtherPin)->GetPortId();

				const Schematyc::SGUID targetGuid = m_nodeItem.GetGUID();
				const Schematyc::CUniqueId targetId = GetPortId();

				return scriptGraph.CanAddLink(sourceGuid, sourceId, targetGuid, targetId);
			}
		}
	}
	else
	{
		if (m_pinType == EPinType::Data || !IsConnected())
		{
			return (pOtherPin != this);
		}
	}

	return false;
}

Schematyc::CUniqueId CPinItem::GetPortId() const
{
	if (IsInputPin())
		return m_nodeItem.GetScriptElement().GetInputId(m_index);
	else
		return m_nodeItem.GetScriptElement().GetOutputId(m_index);
}

void CPinItem::UpdateWithNewIndex(uint32 index)
{
	CRY_ASSERT(m_index <= USHRT_MAX);
	m_index = index;

	Schematyc::IScriptGraphNode& scriptNode = m_nodeItem.GetScriptElement();
	Schematyc::ScriptGraphPortFlags::UnderlyingType portFlags = 0;
	if (IsInputPin())
	{
		m_name = scriptNode.GetInputName(index);
		m_pDataTypeItem = CDataTypesModel::GetInstance().GetTypeItemByGuid(scriptNode.GetInputTypeGUID(index));

		portFlags = m_nodeItem.GetScriptElement().GetInputFlags(index).UnderlyingValue();
		m_id = CPinId(m_nodeItem.GetScriptElement().GetInputId(index), CPinId::EType::Input);
	}
	else if (IsOutputPin())
	{
		m_name = scriptNode.GetOutputName(index);
		m_pDataTypeItem = CDataTypesModel::GetInstance().GetTypeItemByGuid(scriptNode.GetOutputTypeGUID(index));

		portFlags = m_nodeItem.GetScriptElement().GetOutputFlags(index).UnderlyingValue();
		m_id = CPinId(m_nodeItem.GetScriptElement().GetOutputId(index), CPinId::EType::Output);
	}
	else
	{
		CRY_ASSERT_MESSAGE(false, "Pin must be either of type input or output.");
	}

	if (m_pDataTypeItem == nullptr)
	{
		m_pDataTypeItem = &CDataTypeItem::Empty();
	}

	if (portFlags & static_cast<Schematyc::ScriptGraphPortFlags::UnderlyingType>(Schematyc::EScriptGraphPortFlags::Signal))
	{
		m_pinType = EPinType::Signal;
	}
	else if (portFlags & static_cast<Schematyc::ScriptGraphPortFlags::UnderlyingType>(Schematyc::EScriptGraphPortFlags::Flow))
	{
		m_pinType = EPinType::Execution;
	}
	else
	{
		m_pinType = EPinType::Data;
	}

	switch (m_pinType)
	{
	case EPinType::Signal:
		{
			m_styleId = "Pin::Signal";
		}
		break;
	case EPinType::Execution:
		{
			m_styleId = "Pin::Execution";
		}
		break;
	case EPinType::Data:
		{
			if (*m_pDataTypeItem != CDataTypeItem::Empty())
			{
				m_styleId = "Pin::";
				m_styleId.append(QtUtil::ToString(m_pDataTypeItem->GetName()).c_str());
			}
			else
				m_styleId = "Pin";
		}
		break;
	}

	SignalInvalidated();
}

}
