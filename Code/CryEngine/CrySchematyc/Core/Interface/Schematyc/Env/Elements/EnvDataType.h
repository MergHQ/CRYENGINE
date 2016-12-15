// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/FundamentalTypes.h"
#include "Schematyc/Env/EnvElementBase.h"
#include "Schematyc/Env/Elements/IEnvDataType.h"
#include "Schematyc/Reflection/Reflection.h"
#include "Schematyc/Utils/Any.h"
#include "Schematyc/Utils/TypeName.h"

#define SCHEMATYC_MAKE_ENV_DATA_TYPE(type, name) Schematyc::EnvDataType::MakeShared<type>(name, SCHEMATYC_SOURCE_FILE_INFO)

namespace Schematyc
{
template<typename TYPE> class CEnvDataType : public CEnvElementBase<IEnvDataType>
{
public:

	inline CEnvDataType(const char* szName, const SSourceFileInfo& sourceFileInfo)
		: CEnvElementBase(Schematyc::GetTypeInfo<TYPE>().GetGUID(), szName, sourceFileInfo)
		, m_defaultValue()
	{}

	// IEnvElement

	virtual bool IsValidScope(IEnvElement& scope) const override
	{
		const EEnvElementType elementType = scope.GetElementType();
		switch (elementType)
		{
		case EEnvElementType::Root:
		case EEnvElementType::Module:
		case EEnvElementType::Class:
		case EEnvElementType::Component:
		case EEnvElementType::Action:
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

	// IEnvType

	virtual const CCommonTypeInfo& GetTypeInfo() const override
	{
		return Schematyc::GetTypeInfo<TYPE>();
	}

	virtual EnvDataTypeFlags GetFlags() const override
	{
		return m_flags;
	}

	virtual CAnyValuePtr Create() const override
	{
		return CAnyValue::MakeShared(m_defaultValue);
	}

	// IEnvType

	inline void SetFlags(const EnvDataTypeFlags& flags)
	{
		m_flags = flags;
	}

	inline void SetDefaultValue(const TYPE& defaultValue)
	{
		m_defaultValue = defaultValue;
	}

private:

	EnvDataTypeFlags m_flags;
	TYPE             m_defaultValue;
};

namespace EnvDataType
{
template<typename TYPE> inline std::shared_ptr<CEnvDataType<TYPE>> MakeShared(const char* szName, const SSourceFileInfo& sourceFileInfo)
{
	SCHEMATYC_VERIFY_TYPE_IS_REFLECTED(TYPE);

	return std::make_shared<CEnvDataType<TYPE>>(szName, sourceFileInfo);
}
} // EnvDataType
} // Schematyc
