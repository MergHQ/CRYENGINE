// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "IMonoObject.h"
#include "IMonoMethod.h"

struct IMonoAssembly;
struct IMonoException;

struct IMonoClass
{
	virtual ~IMonoClass() {}

	virtual const char* GetName() const = 0;
	virtual const char* GetNamespace() const = 0;

	virtual IMonoAssembly* GetAssembly() const = 0;
	virtual void* GetHandle() const = 0;

	// Creates a new instance of this class without calling any constructors
	virtual std::shared_ptr<IMonoObject> CreateUninitializedInstance() = 0;

	// Creates a new instance of this class
	virtual std::shared_ptr<IMonoObject> CreateInstance(void** pConstructorParams = nullptr, int numParams = 0) = 0;

	virtual std::shared_ptr<IMonoObject> CreateInstanceWithDesc(const char* parameterDesc, void** pConstructorParams) = 0;
	
	// Searches the specified class for the method
	// Will NOT search in base classes, see FindMethodInInheritedClasses
	virtual std::shared_ptr<IMonoMethod> FindMethod(const char* szName, int numParams = 0) const = 0;
	// Searches the entire inheritance tree for the specified method
	virtual std::shared_ptr<IMonoMethod> FindMethodInInheritedClasses(const char* szName, int numParams = 0) const = 0;

	// Searches the specified class for the method by its description
	// Will NOT search in base classes, see FindMethodWithDescInInheritedClasses
	virtual std::shared_ptr<IMonoMethod> FindMethodWithDesc(const char* szMethodDesc) const = 0;
	// Searches the entire inheritance tree for the specified method by its description
	virtual std::shared_ptr<IMonoMethod> FindMethodWithDescInInheritedClasses(const char* szMethodDesc) const = 0;

	// Check whether a method with the specified description is implemented in the class hierarchy
	// Returns false if method was not found, or pBaseClass is encountered in the hierarchy
	virtual bool IsMethodImplemented(IMonoClass* pBaseClass, const char* szMethodDesc) = 0;
};