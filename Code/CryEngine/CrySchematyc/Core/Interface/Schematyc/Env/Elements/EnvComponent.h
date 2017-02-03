// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryMemory/STLPoolAllocator.h>

#include "Schematyc/Component.h"
#include "Schematyc/FundamentalTypes.h"
#include "Schematyc/Env/EnvElementBase.h"
#include "Schematyc/Env/Elements/IEnvComponent.h"
#include "Schematyc/Reflection/ComponentDesc.h"

#define SCHEMATYC_MAKE_ENV_COMPONENT(component) Schematyc::EnvComponent::MakeShared<component>(SCHEMATYC_SOURCE_FILE_INFO)

namespace Schematyc
{

template<typename COMPONENT> class CEnvComponent : public CEnvElementBase<IEnvComponent>
{
public:

	inline CEnvComponent(const SSourceFileInfo& sourceFileInfo)
		: CEnvElementBase(sourceFileInfo)
	{
		const CComponentDesc& desc = Schematyc::GetTypeDesc<COMPONENT>();
		CEnvElementBase::SetGUID(desc.GetGUID());
		CEnvElementBase::SetName(desc.GetLabel());
		CEnvElementBase::SetDescription(desc.GetDescription());
	}

	// IEnvElement

	virtual bool IsValidScope(IEnvElement& scope) const override
	{
		switch (scope.GetType())
		{
		case EEnvElementType::Module:
		case EEnvElementType::Class:
			{
				return true;
			}
		default:
			{
				return false;
			}
		}
	}

	// ~IEnvElement

	// IEnvComponent

	virtual const CComponentDesc& GetDesc() const override
	{
		return GetTypeDesc<COMPONENT>();
	}

	virtual CComponentPtr CreateFromPool() const override
	{
		return std::allocate_shared<COMPONENT>(m_allocator);
	}

	// ~IEnvComponent

private:

	stl::STLPoolAllocator<COMPONENT> m_allocator;
};

namespace EnvComponent
{

template<typename TYPE> inline std::shared_ptr<CEnvComponent<TYPE>> MakeShared(const SSourceFileInfo& sourceFileInfo)
{
	static_assert(std::is_base_of<CComponent, TYPE>::value, "Type must be derived from Schematyc::CComponent!");
	SCHEMATYC_VERIFY_TYPE_IS_REFLECTED(TYPE);

	return std::make_shared<CEnvComponent<TYPE>>(sourceFileInfo);
}

} // EnvComponent
} // Schematyc
