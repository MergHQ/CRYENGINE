// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/Framework/Common.h>
#include <CryReflection/ITypeDesc.h>
#include <CryReflection/IFunctionDesc.h>
#include <CryReflection/IVariableDesc.h>
#include <CryReflection/IEnumValueDesc.h>
#include <CryReflection/Register/Object.h>
#include <CryReflection/Register/TypeRegistrationChain.h>

namespace Cry {
namespace Reflection {

template<typename BASE_TYPE, typename TYPE>
inline bool ReflectBaseType(Reflection::ITypeDesc& typeDesc, Type::Utils::SExplicit<TYPE>, Type::Utils::SExplicit<BASE_TYPE>, const SSourceFileInfo& srcInfo)
{
	return typeDesc.AddBaseType(Type::IdOf<BASE_TYPE>(), Type::CAdapter_BaseCast<TYPE, BASE_TYPE>(), srcInfo);
}

template<typename FUNCTION_TYPE>
IFunctionDesc* ReflectFunction(Reflection::ITypeDesc& typeDesc, FUNCTION_TYPE funcPtr, const char* szLabel, const char* szDescription, CGuid guid, const SSourceFileInfo& srcInfo)
{
	IFunctionDesc* pFunctionDesc = typeDesc.AddFunction(CFunctionCreator<Utils::CFunctionDesc<decltype(funcPtr)>>(funcPtr), szLabel, guid, srcInfo);
	if (pFunctionDesc)
	{
		pFunctionDesc->SetDescription(szDescription);
	}
	return pFunctionDesc;
}

template<typename VARIABLE_TYPE, typename OBJECT_TYPE>
IVariableDesc* ReflectVariable(Reflection::ITypeDesc& typeDesc, VARIABLE_TYPE OBJECT_TYPE::* pVariable, const char* szLabel, const char* szDescription, CGuid guid, bool isPublic, const SSourceFileInfo& srcInfo)
{
	const bool isConst = std::is_const<VARIABLE_TYPE>::value;
	const Type::CTypeDesc desc = Type::DescOf<VARIABLE_TYPE>();
	IVariableDesc* pVariableDesc = typeDesc.AddVariable(desc.GetTypeId(), CMemberVariableOffset<VARIABLE_TYPE, OBJECT_TYPE>(pVariable), szLabel, guid, isConst, isPublic, srcInfo);
	if (pVariable)
	{
		pVariableDesc->SetDescription(szDescription);
	}
	return pVariableDesc;
}

inline IEnumValueDesc* ReflectEnumValue(Reflection::ITypeDesc& typeDesc, const char* szLabel, uint64 value, const char* szDescription)
{
	return typeDesc.AddEnumValue(szLabel, value, szDescription);
}

//inline bool AddConversion(Reflection::ITypeDesc& typeDesc, CTypeId typeId, Type::CConversionOperator& conversionOperator)
//{
//	return typeDesc.AddConversionOperator(typeId, conversionOperator);
//}

#define CRY_REFLECT_OBJECT(type, guid)                                                                 \
  template<>                                                                                           \
  Cry::Reflection::CTypeRegistrationChain Cry::Reflection::SObjectRegistration<type>::s_chainElement = \
    Cry::Reflection::CObject<type>::RegisterType(Cry::Type::DescOf<type>(), guid, &type::ReflectType, Cry::Reflection::SSourceFileInfo(__FILE__, __LINE__))

#define CRY_REFLECT_SIMPLE_TYPE(type, guid, label, desc)                                               \
  void Reflect_ ## type(Cry::Reflection::ITypeDesc & typeDesc)                                         \
  {                                                                                                    \
    if (label) typeDesc.SetLabel(label);                                                               \
    if (desc)  typeDesc.SetDescription(desc);                                                          \
  }                                                                                                    \
  template<>                                                                                           \
  Cry::Reflection::CTypeRegistrationChain Cry::Reflection::SObjectRegistration<type>::s_chainElement = \
    Cry::Reflection::CObject<type>::RegisterType(Cry::Type::DescOf<type>(), guid, &Reflect_ ## type, Cry::Reflection::SSourceFileInfo(__FILE__, __LINE__))

#define CRY_REFLECT_TYPE(type, guid, reflectFunc)                                                      \
  template<>                                                                                           \
  Cry::Reflection::CTypeRegistrationChain Cry::Reflection::SObjectRegistration<type>::s_chainElement = \
    Cry::Reflection::CObject<type>::RegisterType(Cry::Type::DescOf<type>(), guid, &reflectFunc, Cry::Reflection::SSourceFileInfo(__FILE__, __LINE__))

#define CRY_REFLECT_BASE(typeDesc, classType, baseType) \
  Cry::Reflection::ReflectBaseType(typeDesc, Cry::Type::Utils::SExplicit<typename Cry::Type::PureType<classType>::Type>(), Cry::Type::Utils::SExplicit<typename Cry::Type::PureType<baseType>::Type>(),  Cry::Reflection::SSourceFileInfo(__FILE__, __LINE__, __FUNCTION__))

#define CRY_REFLECT_MEMBER_FUNCTION(typeDesc, funcPtr, label, desc, guid) \
  Cry::Reflection::ReflectFunction(typeDesc, &funcPtr, label, desc, guid, Cry::Reflection::SSourceFileInfo(__FILE__, __LINE__, __FUNCTION__))

#define CRY_REFLECT_MEMBER_VARIABLE(typeDesc, variable, label, desc, guid, isPublic) \
  Cry::Reflection::ReflectVariable(typeDesc, &variable, label, desc, guid, isPublic, Cry::Reflection::SSourceFileInfo(__FILE__, __LINE__, __FUNCTION__))

#define CRY_REFLECT_ENUM_VALUE(typeDesc, label, value, desc) \
  Cry::Reflection::ReflectEnumValue(typeDesc, label, static_cast<uint64>(value), desc);

//#define CRY_REFLECT_CONVERSION(typeDesc, typeId, conversionOperator)
//  Cry::Reflection::AddConversionOperator(typeDesc, typeId, conversionOperator);

} // ~Reflection namespace
} // ~Cry namespace
