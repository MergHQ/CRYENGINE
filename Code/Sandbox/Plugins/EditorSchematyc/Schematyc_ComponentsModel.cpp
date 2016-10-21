// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Schematyc_ComponentsModel.h"

#include "Schematyc_ScriptBrowserUtils.h"
#include "Schematyc_DetailWidget.h"

#include <Schematyc/Utils/Schematyc_Any.h>
#include <Schematyc/SerializationUtils/Schematyc_ISerializationContext.h>

#include <CryIcon.h>

#include "QtUtil.h"

namespace CrySchematycEditor {

CComponentItem::CComponentItem(Schematyc::IScriptComponentInstance& componentInstance, CAbstractComponentsModel& model)
	: m_componentInstance(componentInstance)
	, m_model(model)
{
	m_name = m_componentInstance.GetName();
}

CComponentItem::~CComponentItem()
{

}

void CComponentItem::SetName(QString name)
{
	Schematyc::CStackString uniqueName = QtUtil::ToString(name).c_str();
	Schematyc::ScriptBrowserUtils::MakeScriptElementNameUnique(uniqueName, m_model.GetScriptElement());

	m_componentInstance.SetName(uniqueName.c_str());
	m_name = m_componentInstance.GetName();
}

QString CComponentItem::GetDescription() const
{
	return "";
}

QIcon CComponentItem::GetIcon() const
{
	return QIcon(Schematyc::ScriptBrowserUtils::GetScriptElementIcon(m_componentInstance));
}

void CComponentItem::Serialize(Serialization::IArchive& archive)
{
	Schematyc::SSerializationContextParams serializationParams(archive, Schematyc::ESerializationPass::Edit);
	Schematyc::ISerializationContextPtr pSerializationContext = GetSchematycFramework().CreateSerializationContext(serializationParams);
	m_componentInstance.Serialize(archive);
	if (archive.isInput())
	{
		GetSchematycFramework().GetScriptRegistry().ElementModified(m_componentInstance);
	}
}

}
