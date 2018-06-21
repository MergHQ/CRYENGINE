// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/Framework/Common.h>

namespace Cry {
namespace Reflection {

struct ITypeDesc;

struct IObject
{
	virtual ~IObject() {}

	virtual bool             IsReflected() const = 0;

	virtual CTypeId          GetTypeId() const = 0;
	virtual TypeIndex        GetTypeIndex() const = 0;
	virtual const ITypeDesc* GetTypeDesc() const = 0;

	virtual bool             IsEqualType(const IObject* pObject) const = 0;
	virtual bool             IsEqualObject(const IObject* pOtherObject) const = 0;
};

} // ~Reflection namespace
} // ~Cry namespace

inline bool operator==(const Cry::Reflection::IObject& lh, Cry::CTypeId rh)
{
	return lh.GetTypeId() == rh;
}

inline bool operator!=(const Cry::Reflection::IObject& lh, Cry::CTypeId rh)
{
	return lh.GetTypeId() != rh;
}

inline bool operator==(const Cry::Reflection::IObject& lh, const Cry::CTypeDesc& rh)
{
	return lh.GetTypeId() == rh.GetTypeId();
}

inline bool operator!=(const Cry::Reflection::IObject& lh, const Cry::CTypeDesc& rh)
{
	return lh.GetTypeId() != rh.GetTypeId();
}
