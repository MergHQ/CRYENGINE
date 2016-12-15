// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/Forward.h>
#include <CrySerialization/STL.h>

//////////////////////////////////////////////////////////////////////////
// CFixedContainerSTL
//////////////////////////////////////////////////////////////////////////

template<class TContainer, class TElement>
class CFixedContainerSTL : public Serialization::ContainerSTL<TContainer, TElement>
{
	typedef Serialization::ContainerSTL<TContainer, TElement> BaseClass;
public:
	explicit CFixedContainerSTL(TContainer* pContainer = nullptr)
		: BaseClass(pContainer)
	{}

	virtual bool isFixedSize() const override { return true; }
};

template<typename TContainer>
CFixedContainerSTL<TContainer, typename TContainer::value_type> FixedContainerSTL(TContainer& container)
{
	typedef typename TContainer::value_type TElement;
	return CFixedContainerSTL<TContainer, TElement>(&container);
}

template<class TContainer, class TElement>
bool Serialize(yasli::Archive& ar, CFixedContainerSTL<TContainer, TElement>& ser, const char* szName, const char* szLabel)
{
	return ar(static_cast<Serialization::IContainer&>(ser), szName, szLabel);
}

//////////////////////////////////////////////////////////////////////////
// SerializeStringWithStringList
//////////////////////////////////////////////////////////////////////////

struct SValidatorKey
{
	template<typename T>
	static SValidatorKey UniqueFromType()
	{
		// TODO pavloi 2015.12.07: usually, we use just the address of a serialized object as validator search key.
		// But UQS editor uses many temporary objects, so the addresses start to clash. Hence, this temporary hack
		// until I will come up with better solution.
		static intptr_t counter = 0;

		const void* p = (const void*)++counter;
		return SValidatorKey(p, Serialization::TypeID::get<T>());
	}

	template<typename T>
	static SValidatorKey FromObject(const T& object)
	{
		return SValidatorKey(&object, Serialization::TypeID::get<T>());
	}

	explicit SValidatorKey(const void* pHandle, const Serialization::TypeID& typeId)
		: pHandle(pHandle)
		, typeId(typeId)
	{}

	const void*                 pHandle;
	const Serialization::TypeID typeId;
};

template<typename TSetterFunc>
bool SerializeWithStringList(
  Serialization::IArchive& archive, const char* szName, const char* szLabel,
  const Serialization::StringList& strList, const char* szOldValue, TSetterFunc setterFunc, const SValidatorKey* pRecord = nullptr)
{
	const int oldValueIdx = strList.find(szOldValue);
	const bool bIsNpos = (oldValueIdx == Serialization::StringList::npos);

	const SValidatorKey validatorKey =
	  pRecord
	  ? *pRecord
	  : SValidatorKey(szOldValue, Serialization::TypeID::get<const char*>());

	Serialization::StringListValue strValue(strList, !bIsNpos ? oldValueIdx : 0, validatorKey.pHandle, validatorKey.typeId);
	const int oldStrValueIndex = strValue.index();
	const bool bRes = archive(strValue, szName, szLabel);

	const bool bChanged = oldStrValueIndex != strValue.index();
	if (bIsNpos && !bChanged)
	{
		archive.error(validatorKey.pHandle, validatorKey.typeId, "Unable to find value <%s> in the list", szOldValue);
	}

	if (bChanged)
	{
		setterFunc(strValue.c_str());
	}

	return bRes;
}

template<typename TString>
bool SerializeStringWithStringList(
  Serialization::IArchive& archive, const char* szName, const char* szLabel,
  const Serialization::StringList& strList, TString& str)
{
	return SerializeWithStringList(archive, szName, szLabel,
	                               strList, str.c_str(), [&str](const char* szNewVal) { str = szNewVal; }, nullptr);
}

template<typename TString>
bool SerializeStringWithStringList(
  Serialization::IArchive& archive, const char* szName, const char* szLabel,
  const Serialization::StringList& strList, TString& str, const SValidatorKey& validatorKey)
{
	return SerializeWithStringList(archive, szName, szLabel,
	                               strList, str.c_str(), [&str](const char* szNewVal) { str = szNewVal; }, &validatorKey);
}
