// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryMono/IMonoClass.h>
#include <mono/metadata/object.h>

class CMonoLibrary;

class CMonoClass : public IMonoClass
{
public:
	CMonoClass(CMonoLibrary* pLibrary, const char *nameSpace, const char *name);
	CMonoClass(CMonoLibrary* pLibrary, MonoClass* pClass);

	virtual ~CMonoClass() = default;

	// IMonoClass
	virtual const char* GetName() const override;
	virtual const char* GetNamespace() const override;

	virtual IMonoAssembly* GetAssembly() const override;

	virtual std::shared_ptr<IMonoObject> CreateInstance(void** pConstructorParams = nullptr, int numParams = 0) override;
	virtual std::shared_ptr<IMonoObject> CreateInstanceWithDesc(const char* parameterDesc, void** pConstructorParams) override;

	virtual std::shared_ptr<IMonoObject> InvokeMethod(const char *name, const IMonoObject* pObject = nullptr, void **pParams = nullptr, int numParams = 0) const override;
	virtual std::shared_ptr<IMonoObject> InvokeMethodWithDesc(const char* methodDesc, const IMonoObject* pObject = nullptr, void** pParams = nullptr) const override;
	// ~IMonoClass

	void Serialize();
	void Deserialize();

	void ReloadClass();

	MonoClass* GetHandle() const { return m_pClass; }

	// Temporary classes are assigned a weak pointer of itself to pass on to created objects
	void SetWeakPointer(std::weak_ptr<CMonoClass> pClass) { m_pThis = pClass; }

protected:
	std::shared_ptr<IMonoObject> InvokeMethod(MonoMethod* pMethod, MonoObject* pObjectHandle, void** pParams, bool bHadException) const;

protected:
	MonoClass* m_pClass;
	CMonoLibrary* m_pLibrary;

	// Each class has to contain a weak pointer to itself in order to provide a shared pointer to the objects it creates
	// Otherwise temporary classes may create objects that outlive the class itself
	std::weak_ptr<CMonoClass> m_pThis;

	// Vector containing weak pointers to all created objects
	// We don't actually manage the deletion of these objects, that is up to whoever created the instance.
	std::vector<std::weak_ptr<IMonoObject>> m_objects;

	string m_namespace;
	string m_name;
};