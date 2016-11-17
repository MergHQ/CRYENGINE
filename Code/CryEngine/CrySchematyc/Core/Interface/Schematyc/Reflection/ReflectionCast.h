// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// #SchematycTODO : Declare specialized cast functions for shared pointers?

#ifndef SCHEMATYC_REFLECTION_HEADER
	#error This file should only be included from Reflection.h!
#endif

namespace Schematyc
{
namespace Private
{
inline void* DynamicCast(const CCommonTypeInfo& fromTypeInfo, void* pFrom, const CCommonTypeInfo& toTypeInfo)
{
	SCHEMATYC_CORE_ASSERT(pFrom);
	if (fromTypeInfo == toTypeInfo)
	{
		return pFrom;
	}
	else if (fromTypeInfo.GetClassification() == ETypeClassification::Class)
	{
		for (const SClassTypeInfoBase& base : static_cast<const CClassTypeInfo&>(fromTypeInfo).GetBases())
		{
			void* pCastValue = DynamicCast(base.typeInfo, pFrom, toTypeInfo);
			if (pCastValue)
			{
				return static_cast<uint8*>(pCastValue) + base.offset;
			}
		}
	}
	return nullptr;
}

inline const void* DynamicCast(const CCommonTypeInfo& fromTypeInfo, const void* pFrom, const CCommonTypeInfo& toTypeInfo)
{
	SCHEMATYC_CORE_ASSERT(pFrom);
	if (fromTypeInfo == toTypeInfo)
	{
		return pFrom;
	}
	else if (fromTypeInfo.GetClassification() == ETypeClassification::Class)
	{
		for (const SClassTypeInfoBase& base : static_cast<const CClassTypeInfo&>(fromTypeInfo).GetBases())
		{
			const void* pCastValue = DynamicCast(base.typeInfo, pFrom, toTypeInfo);
			if (pCastValue)
			{
				return static_cast<const uint8*>(pCastValue) + base.offset;
			}
		}
	}
	return nullptr;
}

template<typename TO_TYPE> TO_TYPE* DynamicCast(const CCommonTypeInfo& fromTypeInfo, void* pFrom)
{
	return static_cast<TO_TYPE*>(DynamicCast(fromTypeInfo, pFrom, GetTypeInfo<TO_TYPE>()));
}

template<typename TO_TYPE> const TO_TYPE* DynamicCast(const CCommonTypeInfo& fromTypeInfo, const void* pFrom)
{
	return static_cast<const TO_TYPE*>(DynamicCast(fromTypeInfo, pFrom, GetTypeInfo<TO_TYPE>()));
}
} // Private

template<typename TO_TYPE, typename FROM_TYPE> TO_TYPE& DynamicCast(FROM_TYPE& from)
{
	void* pResult = Private::DynamicCast(GetTypeInfo<FROM_TYPE>(), &from, GetTypeInfo<TO_TYPE>());
	SCHEMATYC_CORE_ASSERT(pResult);
	return *static_cast<TO_TYPE*>(pResult);
}

template<typename TO_TYPE, typename FROM_TYPE> const TO_TYPE* DynamicCast(const FROM_TYPE& from)
{
	const void* pResult = Private::DynamicCast(GetTypeInfo<FROM_TYPE>(), &from, GetTypeInfo<TO_TYPE>());
	SCHEMATYC_CORE_ASSERT(pResult);
	return *static_cast<const TO_TYPE*>(pResult);
}

template<typename TO_TYPE, typename FROM_TYPE> TO_TYPE* DynamicCast(FROM_TYPE* pFrom)
{
	return pFrom ? static_cast<TO_TYPE*>(Private::DynamicCast(GetTypeInfo<FROM_TYPE>(), pFrom, GetTypeInfo<TO_TYPE>())) : nullptr;
}

template<typename TO_TYPE, typename FROM_TYPE> const TO_TYPE* DynamicCast(const FROM_TYPE* pFrom)
{
	return pFrom ? static_cast<const TO_TYPE*>(Private::DynamicCast(GetTypeInfo<FROM_TYPE>(), pFrom, GetTypeInfo<TO_TYPE>())) : nullptr;
}
} // Schematyc
