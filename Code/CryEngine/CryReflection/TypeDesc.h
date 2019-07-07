// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "DescExtension.h"

#include <CryReflection/ITypeDesc.h>
#include <CryString/CryString.h>

#include <unordered_map>
#include <vector>

namespace Cry {
namespace Reflection {

class CFunction;
class CBaseTypeDesc;
class CFunctionDesc;
class CVariableDesc;
class CEnumValueDesc;

class CTypeDesc : public CExtensibleDesc<ITypeDesc>
{
public:
	CTypeDesc();
	CTypeDesc(CGuid guid, const Type::CTypeDesc& typeDesc);
	CTypeDesc(CTypeDesc&& other);

	virtual ~CTypeDesc() {}

	virtual CTypeId                GetTypeId() const final                         { return m_typeDesc.GetTypeId(); }
	virtual const Type::CTypeDesc& GetTypeDesc() const final                       { return m_typeDesc; }

	virtual bool                   IsAbstract() const final                        { return m_typeDesc.IsAbstract(); }
	virtual bool                   IsArray() const final                           { return m_typeDesc.IsArray(); }
	virtual bool                   IsClass() const final                           { return m_typeDesc.IsClass(); }
	virtual bool                   IsEnum() const final                            { return m_typeDesc.IsEnum(); }
	virtual bool                   IsFundamental() const final                     { return m_typeDesc.IsFundamental(); }
	virtual bool                   IsUnion() const final                           { return m_typeDesc.IsUnion(); }

	virtual CryGUID                GetGuid() const final                           { return m_guid; }
	virtual ETypeCategory          GetCategory() const final                       { return m_category; }
	virtual TypeIndex              GetIndex() const final                          { return m_index; }

	virtual const char*            GetFullQualifiedName() const final              { return m_typeDesc.GetRawName(); }
	virtual void                   SetLabel(const char* szLabel) final             { m_label = szLabel; }
	virtual const char*            GetLabel() const final                          { return m_label; }

	virtual void                   SetDescription(const char* szDescription) final { m_description = szDescription; }
	virtual const char*            GetDescription() const final                    { return m_description; }

	virtual uint16                 GetSize() const final                           { return m_typeDesc.GetSize(); }
	virtual uint16                 GetAlignment() const final                      { return m_typeDesc.GetAlignment(); }

	virtual const SSourceFileInfo& GetSourceInfo() const final                     { return m_sourcePos; }

	// Class Members
	virtual bool                 AddBaseType(CTypeId typeId, Type::CBaseCastFunction baseCastFunction, const SSourceFileInfo& srcInfo) final;
	virtual size_t               GetBaseTypeCount() const final { return IsClass() ? m_classMembers.baseTypesByIndex.size() : 0; }
	virtual const IBaseTypeDesc* GetBaseTypeByIndex(size_t index) const final;

	virtual IFunctionDesc*       AddFunction(const CFunction& function, const char* szLabel, CGuid guid, const SSourceFileInfo& srcInfo) final;
	virtual size_t               GetFunctionCount() const final { return IsClass() ? m_classMembers.functionsByIndex.size() : 0; }
	virtual const IFunctionDesc* GetFunctionByIndex(size_t index) const final;

	virtual IVariableDesc*       AddVariable(CTypeId typeId, ptrdiff_t offset, const char* szLabel, CGuid guid, bool isConst, bool IsPublic, const SSourceFileInfo& srcInfo) final;
	virtual size_t               GetVariableCount() const final { return IsClass() ? m_classMembers.variablesByIndex.size() : 0; }
	virtual const IVariableDesc* GetVariableByIndex(size_t index) const final;

	// Enum Members
	virtual IEnumValueDesc*       AddEnumValue(const char* szLabel, uint64 value, const char* szDescription) final;
	virtual size_t                GetEnumValueCount() const final { return IsEnum() ? m_enumMembers.valuesByIndex.size() : 0; }
	virtual const IEnumValueDesc* GetEnumValueByIndex(size_t index) const final;

	// Generic Members
	virtual Type::CDefaultConstructor GetDefaultConstructor() const final { return m_typeDesc.GetDefaultConstructor(); }
	virtual Type::CCopyConstructor    GetCopyConstructor() const final    { return m_typeDesc.GetCopyConstructor(); }
	virtual Type::CMoveConstructor    GetMoveConstructor() const final    { return m_typeDesc.GetMoveConstructor(); }

	virtual Type::CDestructor         GetDestructor() const final         { return m_typeDesc.GetDestructor(); }

	virtual Type::CDefaultNewOperator GetDefaultNewOperator() const final { return m_typeDesc.GetDefaultNewOperator(); }
	virtual Type::CDeleteOperator     GetDeleteOperator() const final     { return m_typeDesc.GetDeleteOperator(); }

	virtual Type::CCopyAssignOperator GetCopyAssignOperator() const final { return m_typeDesc.GetCopyAssignOperator(); }
	virtual Type::CMoveAssignOperator GetMoveAssignOperator() const final { return m_typeDesc.GetMoveAssignOperator(); }
	virtual Type::CEqualOperator      GetEqualOperator() const final      { return m_typeDesc.GetEqualOperator(); }

	virtual Type::CSerializeFunction  GetSerializeFunction() const final  { return m_typeDesc.GetSerializeFunction(); }
	virtual Type::CToStringFunction   GetToStringFunction() const final   { return m_typeDesc.GetToStringFunction(); }

	virtual bool                      AddConversionOperator(CTypeId typeId, Type::CConversionOperator conversionOperator) override;
	virtual Type::CConversionOperator GetConversionOperator(CTypeId typeId) const override;

	// Operators
	bool operator==(const CTypeDesc& other) const
	{
		return m_guid == other.GetGuid();
	}

	bool operator!=(const CTypeDesc& other) const
	{
		return m_guid != other.GetGuid();
	}

private:
	friend class CCoreRegistry;

private:
	Type::CTypeDesc m_typeDesc;

	CGuid           m_guid;

	string          m_label;
	string          m_description;

	SSourceFileInfo m_sourcePos;
	ETypeCategory   m_category;
	TypeIndex       m_index;

	std::unordered_map<CTypeId::ValueType, Type::CConversionOperator> m_conversionsByTypeId;

	struct
	{
		std::vector<CBaseTypeDesc*> baseTypesByIndex;
		std::vector<CFunctionDesc*> functionsByIndex;
		std::vector<CVariableDesc*> variablesByIndex;
	} m_classMembers;

	struct
	{
		std::vector<CEnumValueDesc*> valuesByIndex;
	} m_enumMembers;
};

} // ~Reflection namespace
} // ~Cry namespace
