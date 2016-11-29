// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct IMonoObject
{
	virtual ~IMonoObject() {}

	virtual std::shared_ptr<IMonoObject> InvokeMethod(const char *methodName, void **pParams = nullptr, int numParams = 0) const = 0;
	virtual std::shared_ptr<IMonoObject> InvokeMethodWithDesc(const char* methodDesc, void** pParams = nullptr) const = 0;

	virtual const char* ToString() const = 0;

	virtual size_t GetArraySize() const = 0;
	virtual char* GetArrayAddress(size_t elementSize, size_t index) const = 0;

	virtual void* GetHandle() const = 0;
	virtual struct IMonoClass* GetClass() const = 0;
};