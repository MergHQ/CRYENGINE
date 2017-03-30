// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct IMonoObject
{
	virtual ~IMonoObject() {}

	// Gets the string form of the object
	virtual const char* ToString() const = 0;

	// Gets the size of the array this object represents, if any
	virtual size_t GetArraySize() const = 0;
	// Gets the address of an element inside the array
	virtual char* GetArrayAddress(size_t elementSize, size_t index) const = 0;
	
	// Gets the internal handle for the object
	virtual void* GetHandle() const = 0;
	// Gets the class of the object, queries if not already available
	virtual struct IMonoClass* GetClass() = 0;
};