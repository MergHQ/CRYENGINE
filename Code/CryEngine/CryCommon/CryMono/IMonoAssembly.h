// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "IMonoClass.h"
#include "IMonoException.h"

struct IMonoDomain;

// Represents a managed assembly
struct IMonoAssembly
{
	virtual ~IMonoAssembly() {}

	virtual bool IsLoaded() const = 0;
	
	virtual const char* GetFilePath() = 0;
	
	virtual IMonoDomain* GetDomain() const = 0;
	
	// Finds a class inside this assembly, returns null if the lookup failed
	// The class will be stored inside the assembly on success and automatically updated on domain reload
	virtual IMonoClass* GetClass(const char *nameSpace, const char *className) = 0;

	// Finds a class inside this assembly, returns null if the lookup failed
	// The pointer returned is entirely temporary, and will be destroyed when the shared pointer goes out of scope
	virtual std::shared_ptr<IMonoClass> GetTemporaryClass(const char *nameSpace, const char *className) = 0;

	virtual std::shared_ptr<IMonoException> GetException(const char *nameSpace, const char *exceptionClass, const char *message, ...)
	{
		va_list	args;
		char szBuffer[4096];
		va_start(args, message);
		int count = cry_vsprintf(szBuffer, sizeof(szBuffer), message, args);
		if (count == -1 || count >= sizeof(szBuffer))
			szBuffer[sizeof(szBuffer) - 1] = '\0';
		va_end(args);

		return GetExceptionInternal(nameSpace, exceptionClass, szBuffer);
	}

protected:
	virtual std::shared_ptr<IMonoException> GetExceptionInternal(const char* nameSpace, const char* exceptionClass, const char* message = "") = 0;
};