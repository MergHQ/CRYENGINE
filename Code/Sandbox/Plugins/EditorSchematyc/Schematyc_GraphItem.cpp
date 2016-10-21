// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Schematyc_GraphItem.h"

#include "Schematyc_AbstractObjectModel.h"

#include "Schematyc_ScriptBrowserUtils.h"
#include "Schematyc_ObjectModel.h"
#include "Schematyc_GraphViewModel.h"

#include <Schematyc/SerializationUtils/Schematyc_ISerializationContext.h>

#include <Schematyc/Script/Elements/Schematyc_IScriptFunction.h>
#include <Schematyc/Script/Elements/Schematyc_IScriptConstructor.h>
#include <Schematyc/Script/Elements/Schematyc_IScriptDestructor.h>

#include <Schematyc/Script/Schematyc_IScriptGraph.h>
#include <Schematyc/Script/Schematyc_IScriptExtension.h>

#include <CryIcon.h>
#include <QtUtil.h>

namespace CrySchematycEditor {

QIcon CGraphItem::s_iconFunction = CryIcon("icons:schematyc/script_function.ico");
QIcon CGraphItem::s_iconConstructor = CryIcon("icons:schematyc/script_constructor.ico");
QIcon CGraphItem::s_iconDestructor = CryIcon("icons:schematyc/script_destructor.ico");
QIcon CGraphItem::s_iconSignalReceivers = CryIcon("icons:schematyc/script_signal_receiver.ico");

CGraphItem::CGraphItem(Schematyc::IScriptFunction& scriptFunction, CAbstractObjectStructureModel& model)
	: CAbstractObjectStructureModelItem(model)
	, m_pScriptFunction(&scriptFunction)
	, m_pParentItem(nullptr)
	, m_pGraphModel(nullptr)
{
	m_graphType = eGraphType_Function;
	LoadFromScriptElement();
}

CGraphItem::CGraphItem(Schematyc::IScriptConstructor& scriptConstructor, CAbstractObjectStructureModel& model)
	: CAbstractObjectStructureModelItem(model)
	, m_pScriptConstructor(&scriptConstructor)
	, m_pParentItem(nullptr)
	, m_pGraphModel(nullptr)
{
	m_graphType = eGraphType_Construction;
	LoadFromScriptElement();
}

CGraphItem::CGraphItem(Schematyc::IScriptDestructor& scriptDestructor, CAbstractObjectStructureModel& model)
	: CAbstractObjectStructureModelItem(model)
	, m_pScriptDestructor(&scriptDestructor)
	, m_pParentItem(nullptr)
	, m_pGraphModel(nullptr)
{
	m_graphType = eGraphType_Destruction;
	LoadFromScriptElement();
}

CGraphItem::CGraphItem(Schematyc::IScriptSignalReceiver& scriptSignalReceiver, CAbstractObjectStructureModel& model)
	: CAbstractObjectStructureModelItem(model)
	, m_pScriptSignalReceiver(&scriptSignalReceiver)
	, m_pParentItem(nullptr)
	, m_pGraphModel(nullptr)
{
	m_graphType = eGraphType_SignalReceiver;
	LoadFromScriptElement();
}

CGraphItem::~CGraphItem()
{

}

void CGraphItem::SetName(QString name)
{
	if (AllowsRenaming())
	{
		Schematyc::CStackString uniqueName = QtUtil::ToString(name).c_str();
		Schematyc::ScriptBrowserUtils::MakeScriptElementNameUnique(uniqueName, m_model.GetScriptElement());

		m_pScriptElement->SetName(uniqueName);
		m_name = m_pScriptElement->GetName();
	}
}

const QIcon* CGraphItem::GetIcon() const
{
	switch (m_graphType)
	{
	case eGraphType_Construction:
		return &s_iconConstructor;
	case eGraphType_Destruction:
		return &s_iconDestructor;
	case eGraphType_Function:
		return &s_iconFunction;
	case eGraphType_SignalReceiver:
		return &s_iconSignalReceivers;
	default:
		break;
	}

	return nullptr;
}

CAbstractObjectStructureModelItem* CGraphItem::GetParentItem() const
{
	return static_cast<CAbstractObjectStructureModelItem*>(m_pParentItem);
}

uint32 CGraphItem::GetIndex() const
{
	if (m_pParentItem)
	{
		return m_pParentItem->GetChildItemIndex(*this);
	}
	else
	{
		return GetModel().GetChildItemIndex(*this);
	}

	return 0xffffffff;
}

void CGraphItem::Serialize(Serialization::IArchive& archive)
{
	// TODO: This will only work for serialization to properties in inspector!
	Schematyc::SSerializationContextParams serParams(archive, Schematyc::ESerializationPass::Edit);
	Schematyc::ISerializationContextPtr pSerializationContext = GetSchematycFramework().CreateSerializationContext(serParams);
	// ~TODO

	m_pScriptElement->Serialize(archive);
}

bool CGraphItem::AllowsRenaming() const
{
	const bool allowsRenaming = !m_pScriptElement->GetElementFlags().Check(Schematyc::EScriptElementFlags::FixedName);
	return allowsRenaming;
}

Schematyc::SGUID CGraphItem::GetGUID() const
{
	return m_pScriptElement->GetGUID();
}

void CGraphItem::LoadFromScriptElement()
{
	m_name = m_pScriptElement->GetName();

	if (m_pGraphModel)
	{
		// TODO: Signal!
		delete m_pGraphModel;
	}

	Schematyc::IScriptExtension* pGraphExtension = m_pScriptElement->GetExtensions().QueryExtension(Schematyc::EScriptExtensionType::Graph);
	if (pGraphExtension)
	{
		Schematyc::IScriptGraph* pScriptGraph = static_cast<Schematyc::IScriptGraph*>(pGraphExtension);
		m_pGraphModel = new CNodeGraphViewModel(*pScriptGraph);
		// TODO: Signal!
	}
}

}
