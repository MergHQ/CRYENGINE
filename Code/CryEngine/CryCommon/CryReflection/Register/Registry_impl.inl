// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include <CryReflection/Register/Registry.h>
#include <CryReflection/IModule.h>
#include <CryReflection/ITypeDesc.h>
#include <CryReflection/IFunctionDesc.h>

namespace Cry {
namespace Reflection {

template<typename REGISTRY_INTERFACE_TYPE>
CRegistry<REGISTRY_INTERFACE_TYPE>::CRegistry(const char* szLabel, const Type::CTypeDesc& typeDesc, CGuid guid)
	: m_label(szLabel)
	, m_typeDesc(typeDesc)
	, m_guid(guid)
{
	Reflection::GetCoreRegistry()->RegisterRegistry(this);
}

template<typename REGISTRY_INTERFACE_TYPE>
TypeIndex::ValueType CRegistry<REGISTRY_INTERFACE_TYPE >::GetTypeCount() const
{
	return static_cast<TypeIndex::ValueType>(m_typesByIndex.size());
}

template<typename REGISTRY_INTERFACE_TYPE>
const ITypeDesc* CRegistry<REGISTRY_INTERFACE_TYPE >::GetTypeByIndex(TypeIndex index) const
{
	if (index < m_typesByIndex.size())
	{
		auto result = std::find(m_typesByIndex.begin(), m_typesByIndex.end(), index);
		if (result != m_typesByIndex.end())
		{
			return CoreRegistry::GetTypeByIndex(index);
		}
	}
	return nullptr;
}

template<typename REGISTRY_INTERFACE_TYPE>
const ITypeDesc* CRegistry<REGISTRY_INTERFACE_TYPE >::GetTypeByGuid(CGuid guid) const
{
	auto result = m_typeIndicesByGuid.find(guid);
	if (result != m_typeIndicesByGuid.end())
	{
		return CoreRegistry::GetTypeByIndex(result->second);
	}
	return nullptr;
}

template<typename REGISTRY_INTERFACE_TYPE>
const ITypeDesc* CRegistry<REGISTRY_INTERFACE_TYPE >::GetTypeById(CTypeId typeId) const
{
	auto result = m_typeIndicesByTypeId.find(typeId);
	if (result != m_typeIndicesByTypeId.end())
	{
		return CoreRegistry::GetTypeByIndex(result->second);
	}
	return nullptr;
}

template<typename REGISTRY_INTERFACE_TYPE>
bool CRegistry<REGISTRY_INTERFACE_TYPE >::UseType_Internal(const ITypeDesc& typeDesc)
{
	const TypeIndex index = typeDesc.GetIndex();

	const ITypeDesc* pTypeDesc = CoreRegistry::GetTypeByIndex(index);
	CRY_ASSERT(pTypeDesc, "Type '%s' not found in the global registry.", typeDesc.GetFullQualifiedName());
	if (pTypeDesc)
	{
		m_typesByIndex.emplace_back(index);
		m_typeIndicesByGuid.emplace(pTypeDesc->GetGuid(), index);
		m_typeIndicesByTypeId.emplace(pTypeDesc->GetTypeId().GetValue(), index);
		return true;
	}

	return false;
}

template<typename REGISTRY_INTERFACE_TYPE>
FunctionIndex::ValueType CRegistry<REGISTRY_INTERFACE_TYPE >::GetFunctionCount() const
{
	return static_cast<FunctionIndex::ValueType>(m_functionsByIndex.size());
}

template<typename REGISTRY_INTERFACE_TYPE>
const IFunctionDesc* CRegistry<REGISTRY_INTERFACE_TYPE >::GetFunctionByIndex(FunctionIndex index) const
{

	auto result = std::find(m_functionsByIndex.begin(), m_functionsByIndex.end(), index);
	if (result != m_functionsByIndex.end())
	{
		return CoreRegistry::GetFunctionByIndex(index);
	}

	return nullptr;
}

template<typename REGISTRY_INTERFACE_TYPE>
const IFunctionDesc* CRegistry<REGISTRY_INTERFACE_TYPE >::GetFunctionByGuid(CGuid guid) const
{
	for (const auto& curPair : m_functionsByGuid)
	{
		if (curPair.first == guid)
		{
			return CoreRegistry::GetFunctionByIndex(curPair.second);
		}
	}
	return nullptr;
}

template<typename REGISTRY_INTERFACE_TYPE>
const IFunctionDesc* CRegistry<REGISTRY_INTERFACE_TYPE >::GetFunctionByName(const char* szName) const
{
	if (szName)
	{
		const uint32_t nameHash = CCrc32::Compute(szName);
		for (const auto& curPair : m_functionsByName)
		{
			if (curPair.first == nameHash)
			{
				return CoreRegistry::GetFunctionByIndex(curPair.second);
			}
		}
	}
	CRY_ASSERT(szName, "Can't search for a function without a name.");

	return nullptr;
}

template<typename REGISTRY_INTERFACE_TYPE>
bool CRegistry<REGISTRY_INTERFACE_TYPE >::UseFunction_Internal(const IFunctionDesc& functionDesc)
{
	const FunctionIndex index = functionDesc.GetIndex();

	const IFunctionDesc* pFunctionDesc = CoreRegistry::GetFunctionByIndex(index);
	CRY_ASSERT(pFunctionDesc, "Function '%s' not found in the global registry.", pFunctionDesc->GetFullQualifiedName());
	if (pFunctionDesc)
	{
		m_functionsByIndex.emplace_back(index);
		m_functionsByGuid.emplace_back(pFunctionDesc->GetGuid(), index);
		// TODO: For now we use label as function name. Maybe this needs to be changed later.
		m_functionsByName.emplace_back(CCrc32::Compute(pFunctionDesc->GetLabel()), index);
		// ~TODO
		return true;
	}

	return false;
}

} // ~Reflection namespace
} // ~Cry namespace
