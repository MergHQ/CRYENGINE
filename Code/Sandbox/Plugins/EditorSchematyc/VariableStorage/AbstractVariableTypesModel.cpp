// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AbstractVariableTypesModel.h"

#include <Schematyc/Reflection/Schematyc_Reflection.h>
#include <Schematyc/Utils/Schematyc_SharedString.h>

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

	CREATE_TYPE_ITEM(bool, QColor(0, 108, 217));
	CREATE_TYPE_ITEM(int32, QColor(215, 55, 55));
	CREATE_TYPE_ITEM(uint32, QColor(215, 55, 55));
	CREATE_TYPE_ITEM(float, QColor(185, 185, 185));
	CREATE_TYPE_ITEM(Vec3, QColor(250, 232, 12));
	CREATE_TYPE_ITEM(Schematyc::SGUID, QColor(38, 184, 33));
	CREATE_TYPE_ITEM(Schematyc::CSharedString, QColor(128, 100, 162));
}
