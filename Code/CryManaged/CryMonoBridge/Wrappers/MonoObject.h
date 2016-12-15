// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryMono/IMonoObject.h>

#include <mono/metadata/object.h>
#include <mono/metadata/class.h>

struct IMonoClass;

class CMonoObject final : public IMonoObject
{
public:
	CMonoObject(MonoObject* pObject, std::shared_ptr<IMonoClass> pClass);
	virtual ~CMonoObject();

	// IMonoObject
	virtual std::shared_ptr<IMonoObject> InvokeMethod(const char *methodName, void **pParams, int numParams) const override;
	virtual std::shared_ptr<IMonoObject> InvokeMethodWithDesc(const char* methodDesc, void** pParams = nullptr) const override;

	virtual const char* ToString() const override;

	virtual size_t GetArraySize() const override;
	virtual char* GetArrayAddress(size_t elementSize, size_t index) const override;

	virtual void* GetHandle() const override { return m_pObject; }
	virtual IMonoClass* GetClass() const override { return m_pClass.get(); }
	// ~IMonoObject

	void Serialize();
	void Deserialize();

protected:
	MonoObject* m_pObject;
	uint32 m_gcHandle;

	std::shared_ptr<IMonoClass> m_pClass;
};