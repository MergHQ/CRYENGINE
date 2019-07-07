// Copyright 2001-20198 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/Framework/Common.h>
#include <CryReflection/IRegistry.h>

#include <unordered_map>
#include <vector>

namespace Cry {
namespace Reflection {

template<typename REGISTRY_INTERFACE_TYPE = IRegistry>
class CRegistry : public REGISTRY_INTERFACE_TYPE
{
public:
	CRegistry(const char* szLabel, const Type::CTypeDesc& typeDesc, CGuid guid);

	virtual ~CRegistry() {}

	virtual CGuid                    GetGuid() const final     { return m_guid; }
	virtual const char*              GetLabel() const final    { return m_label.c_str(); }
	virtual const Type::CTypeDesc&   GetTypeDesc() const final { return m_typeDesc; }
	virtual CTypeId                  GetTypeId() const final   { return m_typeDesc.GetTypeId(); }

	virtual TypeIndex::ValueType     GetTypeCount() const override;
	virtual const ITypeDesc*         GetTypeByIndex(TypeIndex index) const override;
	virtual const ITypeDesc*         GetTypeByGuid(CGuid guid) const override;
	virtual const ITypeDesc*         GetTypeById(CTypeId typeId) const override;

	virtual FunctionIndex::ValueType GetFunctionCount() const override;
	virtual const IFunctionDesc*     GetFunctionByIndex(FunctionIndex index) const override;
	virtual const IFunctionDesc*     GetFunctionByGuid(CGuid guid) const override;
	virtual const IFunctionDesc*     GetFunctionByName(const char* szName) const override;

protected:
	bool UseType_Internal(const ITypeDesc& typeDesc);
	bool UseFunction_Internal(const IFunctionDesc& functionDesc);

protected:
	std::vector<TypeIndex>                                    m_typesByIndex;
	std::unordered_map<CGuid, TypeIndex>                      m_typeIndicesByGuid;
	std::unordered_map<CTypeId::ValueType, TypeIndex>         m_typeIndicesByTypeId;

	std::vector<FunctionIndex>                                m_functionsByIndex;
	std::vector<std::pair<CGuid, FunctionIndex>>              m_functionsByGuid;
	std::vector<std::pair<uint32 /*NameCrc*/, FunctionIndex>> m_functionsByName;

private:
	const string          m_label;
	const CGuid           m_guid;
	const Type::CTypeDesc m_typeDesc;
};

} // ~Reflection namespace
} // ~Cry namespace
