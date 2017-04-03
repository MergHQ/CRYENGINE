// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MonoMethod.h"

#include "MonoObject.h"

CMonoMethod::CMonoMethod(MonoMethod* pMethod)
	: m_pMethod(pMethod)
{
}

std::shared_ptr<IMonoObject> CMonoMethod::Invoke(const IMonoObject* pObject, void** pParameters, bool &bEncounteredException) const
{
	return InvokeInternal(pObject != nullptr ? (MonoObject*)pObject->GetHandle() : nullptr, pParameters, bEncounteredException);
}

std::shared_ptr<IMonoObject> CMonoMethod::Invoke(const IMonoObject* pObject, void** pParameters) const
{
	bool bEncounteredException;
	std::shared_ptr<CMonoObject> pResult = InvokeInternal(pObject != nullptr ? (MonoObject*)pObject->GetHandle() : nullptr, pParameters, bEncounteredException);

	return pResult;
}

std::shared_ptr<CMonoObject> CMonoMethod::InvokeInternal(MonoObject* pMonoObject, void** pParameters, bool &bEncounteredException) const
{
	MonoObject* pException = nullptr;

	MonoObject* pResult = mono_runtime_invoke(m_pMethod, pMonoObject, pParameters, &pException);
	bEncounteredException = pException != nullptr;

	if (!bEncounteredException)
	{
		if (pResult != nullptr)
		{
			CMonoDomain* pDomain = static_cast<CMonoDomain*>(GetMonoRuntime()->GetActiveDomain());

			auto pResultObject = std::make_shared<CMonoObject>(pResult);
			pResultObject->SetWeakPointer(pResultObject);
			return pResultObject;
		}
		else
		{
			return nullptr;
		}
	}

	GetMonoRuntime()->HandleException(pException);
	return nullptr;
}

uint32 CMonoMethod::GetParameterCount() const
{
	MonoMethodSignature* pSignature = mono_method_signature(m_pMethod);
	return mono_signature_get_param_count(pSignature);
}

string CMonoMethod::GetSignatureDescription(bool bIncludeNamespace) const
{
	MonoMethodSignature* pSignature = mono_method_signature(m_pMethod);
	char* desc = mono_signature_get_desc(pSignature, bIncludeNamespace);
	const char *name = mono_method_get_name(m_pMethod);

	string result;
	result.Format(":%s(%s)", name, desc);

	mono_free(desc);
	return result;
}

void CMonoMethod::PrepareForSerialization()
{
	m_description = GetSignatureDescription(false);

	// Invalidate the method so we don't try to use it later
	m_pMethod = nullptr;
}