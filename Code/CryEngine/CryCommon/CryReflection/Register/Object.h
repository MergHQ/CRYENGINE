// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/Framework/Common.h>
#include <CryReflection/Register/TypeRegistrationChain.h>
#include <CryReflection/ITypeDesc.h>
#include <CryReflection/IObject.h>

namespace Cry {
namespace Reflection {

template<typename TYPE>
struct CObject : public IObject
{
	CObject() {}
	virtual ~CObject() {}

	// IObject
	virtual bool IsReflected() const override
	{
		return (TypeDesc() != nullptr);
	}

	virtual CTypeId GetTypeId() const override
	{
		const ITypeDesc* pTypeDesc = GetTypeDesc();
		return pTypeDesc ? pTypeDesc->GetTypeId() : CTypeId::Empty();
		;
	}

	virtual TypeIndex GetTypeIndex() const override
	{
		const ITypeDesc* pTypeDesc = GetTypeDesc();
		return pTypeDesc ? pTypeDesc->GetIndex() : TypeIndex::Empty();
	}

	virtual const ITypeDesc* GetTypeDesc() const override
	{
		return TypeDesc();
	}

	virtual bool IsEqualType(const IObject* pObject) const override final
	{
		CRY_ASSERT(IsReflected() && pObject->IsReflected(), "Object is not reflected.");
		return IsReflected() && pObject->IsReflected() && pObject->GetTypeId() == GetTypeId();
	}

	virtual bool IsEqualObject(const IObject* pObject) const override final
	{
		if (IsEqualType(pObject))
		{
			Type::CEqualOperator isEqual = GetTypeDesc()->GetEqualOperator();
			if (isEqual)
			{
				return isEqual(this, pObject);
			}
			CRY_ASSERT(isEqual, "Reflected object can't be equal compared.");
		}
		return false;
	}
	// ~IObject

	static inline STypeRegistrationParams RegisterType(const Type::CTypeDesc& typeDesc, CGuid guid, ReflectTypeFunction pReflectFunc, const SSourceFileInfo& srcPos)
	{
		return STypeRegistrationParams(typeDesc, guid, pReflectFunc, srcPos);
	}

	static const ITypeDesc* TypeDesc()
	{
		static const ITypeDesc* pTypeDesc = nullptr;
		if (pTypeDesc == nullptr)
		{
			pTypeDesc = CoreRegistry::GetTypeById(Type::IdOf<TYPE>());
			CRY_ASSERT(pTypeDesc, "Requested type not (yet) registered.");
		}
		return pTypeDesc;
	}
};

} // ~Reflection namespace
} // ~Cry namespace
