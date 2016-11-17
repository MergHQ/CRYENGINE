// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AbstractVariableTypesModel.h"

#include <Schematyc/Reflection/Reflection.h>
#include <Schematyc/Utils/SharedString.h>

const CDataTypeItem& CDataTypeItem::Empty()
{
	static CDataTypeItem empty("Unknown", QColor(255, 55, 100), Schematyc::SGUID());
	return empty;
}

CDataTypeItem::CDataTypeItem(QString name, const QColor& color, const Schematyc::SGUID& guid)
	: m_name(name)
	, m_color(color)
	, m_guid(guid)
{

}

CDataTypeItem::~CDataTypeItem()
{

}

CDataTypesModel& CDataTypesModel::GetInstance()
{
	static CDataTypesModel instance;
	return instance;
}

CDataTypesModel::CDataTypesModel()
{
	GenerateTypeInfo();
}

CDataTypeItem* CDataTypesModel::GetTypeItemByIndex(uint32 index) const
{
	if (index < m_typesByIndex.size())
	{
		return m_typesByIndex[index];
	}
	return nullptr;
}

CDataTypeItem* CDataTypesModel::GetTypeItemByGuid(const Schematyc::SGUID& guid) const
{
	TypesByGuid::const_iterator result = m_typesByGuid.find(guid);
	if (result != m_typesByGuid.end())
	{
		return result->second;
	}
	return nullptr;
}

CDataTypesModel::~CDataTypesModel()
{
	for (CDataTypeItem* pTypeItem : m_typesByIndex)
		delete pTypeItem;
}

void CDataTypesModel::GenerateTypeInfo()
{
#define CREATE_TYPE_ITEM(_type_, _color_)                                                                    \
  {                                                                                                          \
    const Schematyc::CTypeInfo<_type_>& typeInfo = Schematyc::GetTypeInfo<_type_>();                         \
    CDataTypeItem* pTypeItem = new CDataTypeItem(typeInfo.GetName().c_str(), (_color_), typeInfo.GetGUID()); \
    m_typesByIndex.emplace_back(pTypeItem);                                                                  \
    m_typesByGuid.emplace(typeInfo.GetGUID(), pTypeItem);                                                    \
  }

	CREATE_TYPE_ITEM(bool, QColor(17, 100, 100));
	CREATE_TYPE_ITEM(int32, QColor(125, 19, 19));
	CREATE_TYPE_ITEM(uint32, QColor(125, 40, 19));
	CREATE_TYPE_ITEM(float, QColor(196, 137, 0));
	CREATE_TYPE_ITEM(Vec3, QColor(140, 133, 38));
	CREATE_TYPE_ITEM(Schematyc::SGUID, QColor(38, 184, 33));
	CREATE_TYPE_ITEM(Schematyc::CSharedString, QColor(128, 100, 162));
	CREATE_TYPE_ITEM(Schematyc::ObjectId, QColor(70, 60, 120));
	//CREATE_TYPE_ITEM(Schematyc::ExplicitEntityId, QColor(110, 180, 160));
}
