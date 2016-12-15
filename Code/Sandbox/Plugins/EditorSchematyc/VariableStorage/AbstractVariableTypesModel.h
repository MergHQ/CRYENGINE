// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QString>
#include <QColor>

// Note: This is just a temporary solution for now!

class CDataTypeItem
{
public:
	static const CDataTypeItem& Empty();

	CDataTypeItem(QString name, const QColor& color, const Schematyc::SGUID& guid);
	virtual ~CDataTypeItem();

	const QString&          GetName() const  { return m_name; }
	const QColor&           GetColor() const { return m_color; }

	const Schematyc::SGUID& GetGUID() const  { return m_guid; }

protected:
	QString          m_name;
	QColor           m_color;

	Schematyc::SGUID m_guid;
};

class CDataTypesModel
{
	typedef std::unordered_map<Schematyc::SGUID, CDataTypeItem*> TypesByGuid;
	typedef std::vector<CDataTypeItem*>                          TypesByIndex;

public:
	static CDataTypesModel& GetInstance();

	uint32                  GetTypeItemsCount() const { return m_typesByIndex.size(); }
	CDataTypeItem*          GetTypeItemByIndex(uint32 index) const;
	CDataTypeItem*          GetTypeItemByGuid(const Schematyc::SGUID& guid) const;
	//CDataTypeItem*       CreateType();
	//virtual bool         RemoveType(CDataTypeItem& type);

private:
	CDataTypesModel();
	~CDataTypesModel();

	void GenerateTypeInfo();

private:
	TypesByGuid  m_typesByGuid;
	TypesByIndex m_typesByIndex;
};
