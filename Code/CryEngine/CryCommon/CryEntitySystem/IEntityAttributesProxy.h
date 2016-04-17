// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __IENTITYATTRIBUTESPROXY_H__
#define __IENTITYATTRIBUTESPROXY_H__

#pragma once

#include "IEntityProxy.h"
#include <CrySerialization/IArchiveHost.h>
#include <CrySerialization/Serializer.h>

struct IEntityAttribute;

DECLARE_SHARED_POINTERS(IEntityAttribute)

typedef DynArray<IEntityAttributePtr> TEntityAttributeArray;

//////////////////////////////////////////////////////////////////////////
struct SEntityAttributesSerializer
{
	SEntityAttributesSerializer(TEntityAttributeArray& _attributes)
		: attributes(_attributes)
	{}

	void Serialize(Serialization::IArchive& archive)
	{
		for (size_t iAttribute = 0, attributeCount = attributes.size(); iAttribute < attributeCount; ++iAttribute)
		{
			IEntityAttribute* pAttribute = attributes[iAttribute].get();
			if (pAttribute != NULL)
			{
				archive(*pAttribute, pAttribute->GetName(), pAttribute->GetLabel());
			}
		}
	}

	TEntityAttributeArray& attributes;
};

//////////////////////////////////////////////////////////////////////////
// Description:
//    Proxy for storage of entity attributes.
//////////////////////////////////////////////////////////////////////////
struct IEntityAttributesProxy : public IEntityProxy
{
	virtual ~IEntityAttributesProxy() {}

	virtual void                         SetAttributes(const TEntityAttributeArray& attributes) = 0;
	virtual TEntityAttributeArray&       GetAttributes() = 0;
	virtual const TEntityAttributeArray& GetAttributes() const = 0;
};

namespace EntityAttributeUtils
{
//////////////////////////////////////////////////////////////////////////
// Description:
//    Clone array of entity attributes.
//////////////////////////////////////////////////////////////////////////
inline void CloneAttributes(const TEntityAttributeArray& src, TEntityAttributeArray& dst)
{
	dst.clear();
	dst.reserve(src.size());
	for (TEntityAttributeArray::const_iterator iAttribute = src.begin(), iEndAttribute = src.end(); iAttribute != iEndAttribute; ++iAttribute)
	{
		dst.push_back((*iAttribute)->Clone());
	}
}

//////////////////////////////////////////////////////////////////////////
// Description:
//    Find entity attribute by name.
//////////////////////////////////////////////////////////////////////////
inline IEntityAttributePtr FindAttribute(TEntityAttributeArray& attributes, const char* name)
{
	CRY_ASSERT(name != NULL);
	if (name != NULL)
	{
		for (TEntityAttributeArray::iterator iAttribute = attributes.begin(), iEndAttribute = attributes.end(); iAttribute != iEndAttribute; ++iAttribute)
		{
			if (strcmp((*iAttribute)->GetName(), name) == 0)
			{
				return *iAttribute;
			}
		}
	}
	return IEntityAttributePtr();
}

//////////////////////////////////////////////////////////////////////////
// Description:
//    Find entity attribute by name.
//////////////////////////////////////////////////////////////////////////
inline IEntityAttributeConstPtr FindAttribute(const TEntityAttributeArray& attributes, const char* name)
{
	CRY_ASSERT(name != NULL);
	if (name != NULL)
	{
		for (TEntityAttributeArray::const_iterator iAttribute = attributes.begin(), iEndAttribute = attributes.end(); iAttribute != iEndAttribute; ++iAttribute)
		{
			if (strcmp((*iAttribute)->GetName(), name) == 0)
			{
				return *iAttribute;
			}
		}
	}
	return IEntityAttributePtr();
}
}

#endif //__IENTITYATTRIBUTESPROXY_H__
