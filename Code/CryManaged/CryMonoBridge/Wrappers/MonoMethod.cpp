// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MonoMethod.h"

#include "MonoObject.h"

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