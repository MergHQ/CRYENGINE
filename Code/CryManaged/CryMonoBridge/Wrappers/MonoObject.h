// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "MonoClass.h"

#include <CryMono/IMonoObject.h>

#include <mono/metadata/object.h>
#include <mono/metadata/class.h>

struct IMonoClass;

class CMonoObject final : public IMonoObject
{
public:
	CMonoObject(MonoObject* pObject, std::shared_ptr<CMonoClass> pClass);
	CMonoObject(MonoObject* pObject);
	virtual ~CMonoObject();

	// IMonoObject
	virtual const char* ToString() const override;

	virtual size_t GetArraySize() const override;
	virtual char* GetArrayAddress(size_t elementSize, size_t index) const override;

	virtual void* GetHandle() const override { return m_pObject; }
	virtual IMonoClass* GetClass() override;
	// ~IMonoObject

	void AssignObject(MonoObject* pObject);

	// GetClass needs to be aware of the object weak ptr
	void SetWeakPointer(std::weak_ptr<CMonoObject> pObject) { m_pThis = pObject; }

protected:
	MonoObject* m_pObject;
	uint32 m_gcHandle;

	std::shared_ptr<CMonoClass> m_pClass;

	// Only needed if the object was constructed without knowledge of its class (m_pClass is null)
	std::weak_ptr<CMonoObject> m_pThis;
};