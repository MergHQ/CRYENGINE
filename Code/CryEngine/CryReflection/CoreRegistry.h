// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "TypeDesc.h"

#include <CryReflection/IRegistry.h>
#include <CryReflection/ICoreRegistry.h>

#include <unordered_map>
#include <vector>

namespace Cry {
namespace Reflection {

struct ITypeDesc;
class CFunctionDesc;
struct IFunctionDesc;

class CCoreRegistry : public ICoreRegistry
{

public:
	CCoreRegistry();

	// ICoreRegistry
	virtual ITypeDesc*           RegisterType(const Type::CTypeDesc& typeDesc, CGuid guid, ReflectTypeFunction pReflectFunc, const SSourceFileInfo& srcPos) override;
	virtual TypeIndex::ValueType GetTypeCount() const override;
	virtual const ITypeDesc*     GetTypeByIndex(TypeIndex index) const override;
	virtual const ITypeDesc*     GetTypeByGuid(CGuid guid) const override;
	virtual const ITypeDesc*     GetTypeById(CTypeId typeId) const override;

	virtual IFunctionDesc*       RegisterFunction(const Reflection::CFunction& function, CGuid guid, const char* szLabel, ReflectFunctionFunction pReflectFunc, const SSourceFileInfo& srcPos) override;
	virtual TypeIndex::ValueType GetFunctionCount() const override;
	virtual const IFunctionDesc* GetFunctionByIndex(TypeIndex index) const override;
	virtual const IFunctionDesc* GetFunctionByGuid(CGuid guid) const override;
	virtual const IFunctionDesc* GetFunctionByName(const char* szName) override;

	virtual size_t               GetRegistriesCount() const override;
	virtual IRegistry*           GetRegistryByIndex(size_t index) const override;
	virtual IRegistry*           GetRegistryByGuid(CGuid guid) const override;
	virtual IRegistry*           GetRegistryById(CTypeId typeId) const override;

	virtual void                 RegisterRegistry(IRegistry* pRegistry) override;
	// ~ICoreRegistry

private:
	static const size_t                                   s_typeSlots = 512;
	std::vector<CTypeDesc>                                m_typesByIndex;
	std::unordered_map<CryGUID, TypeIndex>                m_typeIndicesByGuid;
	std::unordered_map<CTypeId::ValueType, TypeIndex>     m_typeIndicesByTypeId;

	std::vector<CFunctionDesc*>                           m_functionsByIndex;
	std::vector<std::pair<CryGUID, TypeIndex>>            m_functionsByGuid;
	std::vector<std::pair<uint32 /*NameCrc*/, TypeIndex>> m_functionsByName;

	std::vector<IRegistry*>                               m_customRegistriesByIndex;
};

} // ~Reflection namespace
} // ~Cry namespace
