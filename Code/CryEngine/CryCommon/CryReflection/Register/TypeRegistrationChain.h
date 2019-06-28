// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/Framework/Common.h>
#include <CryReflection/IModule.h>
#include <CryReflection/ITypeDesc.h>
#include <CryReflection/Debug/NatVisWriter.h>

namespace Cry {
namespace Reflection {

struct STypeRegistrationParams
{
	STypeRegistrationParams(const Type::CTypeDesc& typeDesc, CGuid guid, ReflectTypeFunction pReflectFunc, const SSourceFileInfo& srcPos)
		: m_guid(guid)
		, m_typeDesc(typeDesc)
		, m_pReflectFunc(pReflectFunc)
		, m_srcPos(srcPos)
	{}

	CGuid                  m_guid;
	const Type::CTypeDesc& m_typeDesc;
	ReflectTypeFunction    m_pReflectFunc;
	SSourceFileInfo        m_srcPos;
};

class CTypeRegistrationChain
{
public:
	CTypeRegistrationChain(const STypeRegistrationParams& params)
		: m_params(params)
	{
		CTypeRegistrationChain*& pFirst = GetFirst();
		m_pNextRegistration = pFirst;
		pFirst = this;
	}

	~CTypeRegistrationChain()
	{
		// TODO: Proper unregister!
	}

	static bool Execute(bool generateNatVis = false)
	{
		CTypeRegistrationChain* pRegistrar = GetFirst();
		while (pRegistrar)
		{
			pRegistrar->Register();
			pRegistrar = pRegistrar->GetNext();
		}

		if (generateNatVis)
		{
			NatVisWriter natVisWriter;
			natVisWriter.WriteFile();
		}

		return true;
	}

private:
	static CTypeRegistrationChain*& GetFirst()
	{
		static CTypeRegistrationChain* pFirst = nullptr;
		return pFirst;
	}

	CTypeRegistrationChain* GetNext() const
	{
		return m_pNextRegistration;
	}

	void Register()
	{
		Reflection::GetCoreRegistry()->RegisterType(m_params.m_typeDesc, m_params.m_guid, m_params.m_pReflectFunc, m_params.m_srcPos);
	}

private:
	CTypeRegistrationChain* m_pNextRegistration;
	STypeRegistrationParams m_params;
};

template<typename TYPE>
struct SObjectRegistration
{
	static CTypeRegistrationChain s_chainElement;
};

} // ~Reflection namespace
} // ~Cry namespace
