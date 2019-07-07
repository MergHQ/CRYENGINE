// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/Framework/Common.h>
#include <CryReflection/IDescExtension.h>

namespace Cry {
namespace Reflection {

class CFunction;
struct IBaseTypeDesc;
struct IFunctionDesc;
struct IVariableDesc;
struct IEnumValueDesc;

enum class ETypeCategory
{
	Unset = 0,

	Abstract,
	Class,
	Enum,
	Fundamental,
	Union
};

struct ITypeDesc : public IExtensibleDesc
{
	virtual ~ITypeDesc() {}

	virtual CTypeId                GetTypeId() const = 0;
	virtual const Type::CTypeDesc& GetTypeDesc() const = 0;

	virtual bool                   IsAbstract() const = 0;
	virtual bool                   IsArray() const = 0;
	virtual bool                   IsClass() const = 0;
	virtual bool                   IsEnum() const = 0;
	virtual bool                   IsFundamental() const = 0;
	virtual bool                   IsUnion() const = 0;

	virtual CGuid                  GetGuid() const = 0;
	virtual ETypeCategory          GetCategory() const = 0;
	virtual TypeIndex              GetIndex() const = 0;

	virtual const char*            GetFullQualifiedName() const = 0;
	virtual void                   SetLabel(const char* szLabel) = 0;
	virtual const char*            GetLabel() const = 0;

	virtual void                   SetDescription(const char* szDescription) = 0;
	virtual const char*            GetDescription() const = 0;

	virtual uint16                 GetSize() const = 0;
	virtual uint16                 GetAlignment() const = 0;

	virtual const SSourceFileInfo& GetSourceInfo() const = 0;

	// Class Members
	virtual bool                 AddBaseType(CTypeId typeId, Type::CBaseCastFunction baseCastFunction, const SSourceFileInfo& srcInfo) = 0;
	virtual size_t               GetBaseTypeCount() const = 0;
	virtual const IBaseTypeDesc* GetBaseTypeByIndex(size_t index) const = 0;

	virtual IFunctionDesc*       AddFunction(const CFunction& function, const char* szLabel, CGuid guid, const SSourceFileInfo& srcInfo) = 0;
	virtual size_t               GetFunctionCount() const = 0;
	virtual const IFunctionDesc* GetFunctionByIndex(size_t index) const = 0;

	virtual IVariableDesc*       AddVariable(CTypeId typeId, ptrdiff_t offset, const char* szLabel, CGuid guid, bool isConst, bool isPublic, const SSourceFileInfo& srcInfo) = 0;
	virtual size_t               GetVariableCount() const = 0;
	virtual const IVariableDesc* GetVariableByIndex(size_t index) const = 0;

	// Enum Members
	virtual IEnumValueDesc*       AddEnumValue(const char* szLabel, uint64 value, const char* szDescription) = 0;
	virtual size_t                GetEnumValueCount() const = 0;
	virtual const IEnumValueDesc* GetEnumValueByIndex(size_t index) const = 0;

	// Generic Members
	virtual Type::CDefaultConstructor GetDefaultConstructor() const = 0;
	virtual Type::CCopyConstructor    GetCopyConstructor() const = 0;
	virtual Type::CMoveConstructor    GetMoveConstructor() const = 0;

	virtual Type::CDestructor         GetDestructor() const = 0;

	virtual Type::CDefaultNewOperator GetDefaultNewOperator() const = 0;
	virtual Type::CDeleteOperator     GetDeleteOperator() const = 0;

	virtual Type::CCopyAssignOperator GetCopyAssignOperator() const = 0;
	virtual Type::CMoveAssignOperator GetMoveAssignOperator() const = 0;
	virtual Type::CEqualOperator      GetEqualOperator() const = 0;

	virtual Type::CSerializeFunction  GetSerializeFunction() const = 0;
	virtual Type::CToStringFunction   GetToStringFunction() const = 0;

	virtual bool                      AddConversionOperator(CTypeId typeId, Type::CConversionOperator conversionOperator) = 0;
	virtual Type::CConversionOperator GetConversionOperator(CTypeId typeId) const = 0;

	// Operators
	bool operator==(const ITypeDesc& other) const
	{
		return GetGuid() == other.GetGuid();
	}

	bool operator!=(const ITypeDesc& other) const
	{
		return GetGuid() != other.GetGuid();
	}
};

} // ~Reflection namespace
} // ~Cry namespace
