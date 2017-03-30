// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryMono/IMonoClass.h>
#include <mono/metadata/object.h>

class CMonoLibrary;
class CMonoObject;

class CMonoClass final : public IMonoClass
{
public:
	CMonoClass(CMonoLibrary* pLibrary, const char *nameSpace, const char *name);
	CMonoClass(CMonoLibrary* pLibrary, MonoClass* pClass);

	virtual ~CMonoClass() = default;

	// IMonoClass
	virtual const char* GetName() const override;
	virtual const char* GetNamespace() const override;

	virtual IMonoAssembly* GetAssembly() const override;
	virtual void* GetHandle() const override { return m_pClass; }

	virtual std::shared_ptr<IMonoObject> CreateUninitializedInstance() override;
	virtual std::shared_ptr<IMonoObject> CreateInstance(void** pConstructorParams = nullptr, int numParams = 0) override;
	virtual std::shared_ptr<IMonoObject> CreateInstanceWithDesc(const char* parameterDesc, void** pConstructorParams) override;

	virtual std::shared_ptr<IMonoMethod> FindMethod(const char* szName, int numParams = 0) const override;
	virtual std::shared_ptr<IMonoMethod> FindMethodInInheritedClasses(const char* szName, int numParams = 0) const override;

	virtual std::shared_ptr<IMonoMethod> FindMethodWithDesc(const char* szMethodDesc) const override;
	virtual std::shared_ptr<IMonoMethod> FindMethodWithDescInInheritedClasses(const char* szMethodDesc) const override;
	
	virtual bool IsMethodImplemented(IMonoClass* pBaseClass, const char* szMethodDesc) override;
	// ~IMonoClass

	void Serialize();
	void Deserialize();

	void ReloadClass();

	// Temporary classes are assigned a weak pointer of itself to pass on to created objects
	void SetWeakPointer(std::weak_ptr<CMonoClass> pClass) { m_pThis = pClass; }

	void RegisterObject(std::weak_ptr<CMonoObject> pObject);

protected:
	MonoClass* m_pClass;
	CMonoLibrary* m_pLibrary;

	// Each class has to contain a weak pointer to itself in order to provide a shared pointer to the objects it creates
	// Otherwise temporary classes may create objects that outlive the class itself
	std::weak_ptr<CMonoClass> m_pThis;

	// Vector containing weak pointers to all created objects
	// We don't actually manage the deletion of these objects, that is up to whoever created the instance.
	std::vector<std::weak_ptr<CMonoObject>> m_objects;

	string m_namespace;
	string m_name;
};