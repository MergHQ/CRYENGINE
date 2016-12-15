// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ItemTypeName.h"

#include "SerializationHelpers.h"
#include "DocSerializationContext.h"
#include "Settings.h"

//////////////////////////////////////////////////////////////////////////
// SItemTypeName
//////////////////////////////////////////////////////////////////////////

SItemTypeName::SItemTypeName()
	: typeName()
{}

SItemTypeName::SItemTypeName(const char* szTypeName)
	: typeName(szTypeName)
{}

SItemTypeName::SItemTypeName(const SItemTypeName& other)
	: typeName(other.typeName)
{}

SItemTypeName::SItemTypeName(SItemTypeName&& other)
	: typeName(std::move(other.typeName))
{}

SItemTypeName& SItemTypeName::operator=(const SItemTypeName& other)
{
	if (this != &other)
	{
		typeName = other.typeName;
	}
	return *this;
}

SItemTypeName& SItemTypeName::operator=(SItemTypeName&& other)
{
	if (this != &other)
	{
		typeName = std::move(other.typeName);
	}
	return *this;
}

void SItemTypeName::Serialize(Serialization::IArchive& archive)
{
	if (CUqsDocSerializationContext* pContext = archive.context<CUqsDocSerializationContext>())
	{
		if (pContext->GetSettings().bUseSelectionHelpers)
		{
			const Serialization::StringList& namesList = pContext->GetItemTypeNamesList();

			SerializeStringWithStringList(archive, "typeName", "^",
			                              namesList, typeName);
		}
		else
		{
			archive(typeName, "typeName", "^");
		}
	}
}

const char* SItemTypeName::c_str() const
{
	return typeName.c_str();
}

bool SItemTypeName::Empty() const
{
	return typeName.empty();
}

bool SItemTypeName::operator==(const SItemTypeName& other) const
{
	return typeName == other.typeName;
}
