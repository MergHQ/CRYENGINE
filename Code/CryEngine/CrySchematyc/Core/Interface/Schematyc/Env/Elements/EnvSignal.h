// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// #SchematycTODO : Explore idea of declaring signatures types rather than structures.
/*
template<typename SIGNATURE> class CFunctionSignature
{
};

template<typename RETURN_TYPE, typename ... PARAM_TYPES>
class CFunctionSignature<RETURN_TYPE(PARAM_TYPES ...)>
{
public:

	typedef RETURN_TYPE                 ReturnType;
	typedef std::tuple<PARAM_TYPES ...> ParamTypes;
};

typedef CFunctionSignature<void(int32)> FunctionSignature;

class CObject
{
public:

	template<typename SIGNATURE> typename SIGNATURE::ReturnType DoSomethingImpl(typename SIGNATURE::ParamTypes& params)
	{
	}

	template<typename SIGNATURE, typename ... PARAM_TYPES> typename SIGNATURE::ReturnType DoSomething(PARAM_TYPES&& ... params)
	{
		static_assert(std::is_same<SIGNATURE::ParamTypes, std::tuple<PARAM_TYPES...>>::value, "Parameters don't match signature!");
		return DoSomethingImpl<SIGNATURE>(std::make_tuple(params ...));
	}
};
*/

#pragma once

#include "Schematyc/FundamentalTypes.h"
#include "Schematyc/Env/EnvElementBase.h"
#include "Schematyc/Env/Elements/IEnvSignal.h"
#include "Schematyc/Reflection/Reflection.h"
#include "Schematyc/Utils/Any.h"
#include "Schematyc/Utils/SharedString.h"

#define SCHEMATYC_MAKE_ENV_SIGNAL(guid, name)      Schematyc::EnvSignal::MakeShared(guid, name, SCHEMATYC_SOURCE_FILE_INFO)
#define SCHEMATYC_MAKE_ENV_SIGNAL_TYPE(type, name) Schematyc::EnvSignal::MakeShared<type>(name, SCHEMATYC_SOURCE_FILE_INFO)

namespace Schematyc
{
class CEnvSignal : public CEnvElementBase<IEnvSignal>
{
private:

	struct SInput
	{
		inline SInput(uint32 _id, const char* _szName, const char* _szDescription, const CAnyValuePtr& _pData)
			: id(_id)
			, name(_szName)
			, description(_szDescription)
			, pData(_pData)
		{}

		uint32       id;
		string       name;
		string       description;
		CAnyValuePtr pData;
	};

	typedef std::vector<SInput> Inputs;

public:

	inline CEnvSignal(const SGUID& guid, const char* szName, const SSourceFileInfo& sourceFileInfo)
		: CEnvElementBase(guid, szName, sourceFileInfo)
	{}

	// IEnvElement

	virtual bool IsValidScope(IEnvElement& scope) const override
	{
		switch (scope.GetElementType())
		{
		case EEnvElementType::Root:
		case EEnvElementType::Module:
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

	// IEnvSignal

	virtual uint32 GetInputCount() const override
	{
		return m_inputs.size();
	}

	virtual uint32 GetInputId(uint32 inputIdx) const override
	{
		return inputIdx < m_inputs.size() ? m_inputs[inputIdx].id : InvalidId;
	}

	virtual const char* GetInputName(uint32 inputIdx) const override
	{
		return inputIdx < m_inputs.size() ? m_inputs[inputIdx].name.c_str() : "";
	}

	virtual const char* GetInputDescription(uint32 inputIdx) const override
	{
		return inputIdx < m_inputs.size() ? m_inputs[inputIdx].description.c_str() : "";
	}

	virtual CAnyConstPtr GetInputData(uint32 inputIdx) const override
	{
		return inputIdx < m_inputs.size() ? m_inputs[inputIdx].pData.get() : CAnyConstPtr();
	}

	// ~IEnvSignal

	inline void AddInput(uint32 id, const char* szName, const char* szDescription, const CAnyConstRef& value)
	{
		// #SchematycTODO : Validate id and name!
		m_inputs.push_back(SInput(id, szName, szDescription, CAnyValue::CloneShared(value)));
	}

private:

	Inputs m_inputs;
};

namespace EnvSignal
{
inline std::shared_ptr<CEnvSignal> MakeShared(const SGUID& guid, const char* szName, const SSourceFileInfo& sourceFileInfo)
{
	return std::make_shared<CEnvSignal>(guid, szName, sourceFileInfo);
}

template<typename TYPE> inline std::shared_ptr<CEnvSignal> MakeShared(const char* szName, const SSourceFileInfo& sourceFileInfo)
{
	static_assert(std::is_default_constructible<TYPE>::value, "Type must be default constructible!");
	static_assert(std::is_class<TYPE>::value, "Type must be struct/class!");

	const CClassTypeInfo& classTypeInfo = static_cast<const CClassTypeInfo&>(GetTypeInfo<TYPE>());
	std::shared_ptr<CEnvSignal> pSignal = std::make_shared<CEnvSignal>(classTypeInfo.GetGUID(), szName, sourceFileInfo);

	const TYPE defaultValue = TYPE();
	for (const SClassTypeInfoMember& member : classTypeInfo.GetMembers())
	{
		CAnyConstRef memberValue(member.typeInfo, reinterpret_cast<const uint8*>(&defaultValue) + member.offset);
		pSignal->AddInput(member.id, member.szLabel, member.szDescription, memberValue);
	}

	return pSignal;
}
} // EnvSignal
} // Schematyc
