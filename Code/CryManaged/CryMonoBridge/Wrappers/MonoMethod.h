// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "MonoObject.h"

#include <CryMono/IMonoMethod.h>

class CMonoMethod final : public IMonoMethod
{
public:
	CMonoMethod(MonoMethod* pMethod);

	// IMonoMethod
	virtual std::shared_ptr<IMonoObject> Invoke(const IMonoObject* pObject, void** pParameters, bool &bEncounteredException) const override;
	virtual std::shared_ptr<IMonoObject> Invoke(const IMonoObject* pObject = nullptr, void** pParameters = nullptr) const override;
	
	virtual uint32 GetParameterCount() const override;
	virtual string GetSignatureDescription(bool bIncludeNamespace = true) const override;
	// ~IMonoMethod

	std::shared_ptr<CMonoObject> InvokeInternal(MonoObject* pMonoObject, void** pParameters, bool &bEncounteredException) const;

	void PrepareForSerialization();
	const char* GetSerializedDescription() const { return m_description; }

	void OnDeserialized(MonoMethod* pMethod) { m_pMethod = pMethod; }

protected:
	MonoMethod* m_pMethod;
	
	string m_description;
};