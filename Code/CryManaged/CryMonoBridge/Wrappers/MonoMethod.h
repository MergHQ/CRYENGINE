// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryMono/IMonoMethod.h>

#include <mono/metadata/object.h>

class CMonoMethod final : public IMonoMethod
{
public:
	CMonoMethod(MonoMethod* pMethod) : m_pMethod(pMethod) {}

	// IMonoMethod
	virtual std::shared_ptr<IMonoObject> Invoke(const IMonoObject* pObject, void** pParameters, bool &bEncounteredException) const override;
	virtual std::shared_ptr<IMonoObject> Invoke(const IMonoObject* pObject = nullptr, void** pParameters = nullptr) const override;
	// ~IMonoMethod

	std::shared_ptr<CMonoObject> InvokeInternal(MonoObject* pMonoObject, void** pParameters, bool &bEncounteredException) const;

protected:
	MonoMethod* m_pMethod;
	MonoMethodSignature* m_pMethodSignature;
};