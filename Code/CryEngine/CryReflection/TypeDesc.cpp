// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "StdAfx.h"

#include "TypeDesc.h"
#include "FunctionDesc.h"
#include "VariableDesc.h"
#include "EnumValueDesc.h"
#include "BaseTypeDesc.h"

#include <CryReflection/IModule.h>

namespace Cry {
namespace Reflection {

CTypeDesc::CTypeDesc()
	: m_category(ETypeCategory::Unset)
{
}

CTypeDesc::CTypeDesc(CGuid guid, const Type::CTypeDesc& typeDesc)
	: m_typeDesc(typeDesc)
	, m_guid(guid)
{
	m_category = ETypeCategory::Unset;
	if (typeDesc.IsAbstract())
	{
		m_category = ETypeCategory::Abstract;
	}
	else if (typeDesc.IsClass())
	{
		m_category = ETypeCategory::Class;
	}
	else if (typeDesc.IsEnum())
	{
		m_category = ETypeCategory::Enum;
	}
	else if (typeDesc.IsFundamental())
	{
		m_category = ETypeCategory::Fundamental;
	}
	else if (typeDesc.IsUnion())
	{
		m_category = ETypeCategory::Union;
	}
}

CTypeDesc::CTypeDesc(CTypeDesc&& rhs)
	: m_typeDesc(rhs.m_typeDesc)
	, m_guid(rhs.m_guid)
	, m_label(rhs.m_label)
	, m_description(rhs.m_description)
	, m_sourcePos(rhs.m_sourcePos)
	, m_category(rhs.m_category)
	, m_index(rhs.m_index)
{
	switch (m_category)
	{
	case ETypeCategory::Class:
		{
			m_classMembers.baseTypesByIndex.swap(rhs.m_classMembers.baseTypesByIndex);
			m_classMembers.functionsByIndex.swap(rhs.m_classMembers.functionsByIndex);
			m_classMembers.variablesByIndex.swap(rhs.m_classMembers.variablesByIndex);
		}
		break;
	case ETypeCategory::Enum:
		{
			m_enumMembers.valuesByIndex.swap(rhs.m_enumMembers.valuesByIndex);
		}
		break;
	default:
		break;
	}

	m_conversionsByTypeId.swap(rhs.m_conversionsByTypeId);
	m_extensions.swap(rhs.m_extensions);
}

bool CTypeDesc::AddBaseType(CTypeId typeId, Type::CBaseCastFunction baseCastFunction, const SSourceFileInfo& srcPos)
{
	CRY_ASSERT(IsClass(), "Base can only be added to class types.");
	if (IsClass())
	{
#if defined(USE_CRY_ASSERT)
		auto compare = [typeId](const CBaseTypeDesc* pDesc) -> bool
		{
			return (pDesc->GetTypeId() == typeId);
		};
#endif

		CRY_ASSERT(std::find_if(m_classMembers.baseTypesByIndex.begin(), m_classMembers.baseTypesByIndex.end(), compare) == m_classMembers.baseTypesByIndex.end(), "Type already registered as base.");
		CBaseTypeDesc* pBaseTypeDesc = new CBaseTypeDesc(typeId, baseCastFunction);
		m_classMembers.baseTypesByIndex.emplace_back(pBaseTypeDesc);
		return true;
	}
	return false;
}

const IBaseTypeDesc* CTypeDesc::GetBaseTypeByIndex(size_t index) const
{
	if (IsClass() && index < m_classMembers.baseTypesByIndex.size())
	{
		return m_classMembers.baseTypesByIndex[index];
	}
	return nullptr;
}

IFunctionDesc* CTypeDesc::AddFunction(const CFunction& function, const char* szLabel, CGuid guid, const SSourceFileInfo& srcPos)
{
	CRY_ASSERT(IsClass(), "Functions can only be added to class types.");
	if (IsClass())
	{
		auto compare = [&guid](const CFunctionDesc* pDesc) -> bool
		{
			return (pDesc->GetGuid() == guid);
		};

		const bool isGuidUnused = (std::find_if(m_classMembers.functionsByIndex.begin(), m_classMembers.functionsByIndex.end(), compare) == m_classMembers.functionsByIndex.end());
		CRY_ASSERT(isGuidUnused, "Function guid {%s} already used.", guid.ToDebugString());
		if (isGuidUnused)
		{
			CFunctionDesc* pFunctionDesc = new CFunctionDesc(this, function.GetReturnTypeId(), function.GetFunctionDelegate(), function.GetParams(), szLabel, guid);
			m_classMembers.functionsByIndex.emplace_back(pFunctionDesc);
			return pFunctionDesc;
		}
	}
	return nullptr;
}

const IFunctionDesc* CTypeDesc::GetFunctionByIndex(size_t index) const
{
	if (IsClass() && index < m_classMembers.functionsByIndex.size())
	{
		return m_classMembers.functionsByIndex[index];
	}
	return nullptr;
}

IVariableDesc* CTypeDesc::AddVariable(CTypeId typeId, ptrdiff_t offset, const char* szLabel, CGuid guid, bool isConst, bool isPublic, const SSourceFileInfo& srcPos)
{
	CRY_ASSERT(IsClass(), "Variables can only be added to class types.");
	if (IsClass())
	{
		// TODO: Decide if we should always check.
#if defined(USE_CRY_ASSERT)
		auto compare = [offset](const CVariableDesc* pDesc) -> bool
		{
			return (pDesc->GetOffset() == offset);
		};
#endif
		CRY_ASSERT(std::find_if(m_classMembers.variablesByIndex.begin(), m_classMembers.variablesByIndex.end(), compare) == m_classMembers.variablesByIndex.end(), "Variable already registered.");

		CVariableDesc* pVariableDesc = new CVariableDesc(offset, szLabel, *this, typeId, guid, isConst, isPublic, srcPos);

		m_classMembers.variablesByIndex.emplace_back(pVariableDesc);
		return m_classMembers.variablesByIndex.back();
	}
	return nullptr;
}

const IVariableDesc* CTypeDesc::GetVariableByIndex(size_t index) const
{
	if (IsClass() && index < m_classMembers.variablesByIndex.size())
	{
		return m_classMembers.variablesByIndex[index];
	}
	return nullptr;
}

IEnumValueDesc* CTypeDesc::AddEnumValue(const char* szLabel, uint64 value, const char* szDescription)
{
	CRY_ASSERT(IsEnum(), "Enum values can only be added to enum types.");
	if (IsEnum())
	{
		// TODO: Decide if we should always check.
#if defined(USE_CRY_ASSERT)
		CEnumValueDesc enumNameValue(szLabel, value, "");
		auto compare = [&enumNameValue](const CEnumValueDesc* pDesc) -> bool
		{
			return (*pDesc == enumNameValue);
		};
#endif
		CRY_ASSERT(std::find_if(m_enumMembers.valuesByIndex.begin(), m_enumMembers.valuesByIndex.end(), compare) == m_enumMembers.valuesByIndex.end(), "Name '%s' with value '%d' already exists.", szLabel, value);
		// ~TODO

		CEnumValueDesc* pEnumDesc = new CEnumValueDesc(szLabel, value, szDescription ? szDescription : "");

		m_enumMembers.valuesByIndex.emplace_back(pEnumDesc);
		return pEnumDesc;
	}
	return nullptr;
}

const IEnumValueDesc* CTypeDesc::GetEnumValueByIndex(size_t index) const
{
	if (IsEnum() && (index < m_enumMembers.valuesByIndex.size()))
	{
		return m_enumMembers.valuesByIndex[index];
	}
	return nullptr;
}

bool CTypeDesc::AddConversionOperator(CTypeId typeId, Type::CConversionOperator conversionOperator)
{
	const bool isNotRegistered = (m_conversionsByTypeId.find(typeId) == m_conversionsByTypeId.end());
	CRY_ASSERT(isNotRegistered, "Conversion already registered.");
	if (isNotRegistered)
	{
		m_conversionsByTypeId.emplace(typeId, conversionOperator);
		return true;
	}
	return false;
}

Type::CConversionOperator CTypeDesc::GetConversionOperator(CTypeId typeId) const
{
	auto result = m_conversionsByTypeId.find(typeId);
	if (result != m_conversionsByTypeId.end())
	{
		return result->second;
	}
	return Type::CConversionOperator();
}

} // ~Reflection namespace
} // ~Cry namespace
