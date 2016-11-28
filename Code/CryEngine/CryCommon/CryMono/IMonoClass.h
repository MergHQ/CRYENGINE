// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "IMonoObject.h"

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
	
	// Invokes a method by name
	// This will NOT find methods in child or parent classes!
	// If pObject is null we'll invoke a static function
	virtual std::shared_ptr<IMonoObject> InvokeMethod(const char *name, const IMonoObject* pObject = nullptr, void** pParams = nullptr, int numParams = 0) const = 0;

	// Invokes a method by method description (e.g. "Env:ScanAssembly(System.Reflection.Assembly)")
	// This will NOT find methods in child or parent classes!
	// If pObject is null we'll invoke a static function
	virtual std::shared_ptr<IMonoObject> InvokeMethodWithDesc(const char* methodDesc, const IMonoObject* pObject = nullptr, void** pParams = nullptr) const = 0;

	// Check whether a method with the specified description is implemented in the class hierarchy
	// Returns false if method was not found, or pBaseClass is encountered in the hierarchy
	virtual bool IsMethodImplemented(IMonoClass* pBaseClass, const char* methodDesc) = 0;
};