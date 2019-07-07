// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "../../CryCore/BaseTypes.h"
#include "../../CrySerialization/Forward.h"
#include "../../CryCore/Assert/CryAssert.h"
#include "../../CryCore/Platform/platform.h"
#include "../../CryMemory/CryMemoryManager.h"

#include "../../CryType/Type.h"
#include "../../CryExtension/CryGUID.h"
#include "../../CryUtils/Index.h"

// TODO: Why do we need this in our header files?
#ifndef IF_UNLIKELY
	#define IF_UNLIKELY if
#endif

#ifndef PREFAST_ASSUME
	#define PREFAST_ASSUME(cond)
#endif
// ~TODO

namespace Cry {

using CGuid = CryGUID;

namespace Reflection {

using TypeIndex = Index<uint16, uint16(~0)>;
using ExtensionIndex = Index<size_t, uint16(~0)>;
using FunctionIndex = Index<uint16, uint16(~0)>;

struct SSourceFileInfo
{
	SSourceFileInfo()
		: m_line(~0)
	{}

	SSourceFileInfo(const char* szFile, uint32 line, const char* szFunction = " ")
		: m_file(szFile)
		, m_function(szFunction)
		, m_line(line)
	{}

	SSourceFileInfo(const SSourceFileInfo& srcPos)
		: m_file(srcPos.GetFile())
		, m_function(srcPos.GetFunction())
		, m_line(srcPos.GetLine())
	{}

	const char* GetFile() const     { return m_file.c_str(); }
	uint32      GetLine() const     { return m_line; }
	const char* GetFunction() const { return m_function.c_str(); }

	// Operators
	SSourceFileInfo& operator=(const SSourceFileInfo& rhs)
	{
		m_file = rhs.GetFile();
		m_line = rhs.GetLine();
		m_function = rhs.GetFunction();

		return *this;
	}

	bool operator==(const SSourceFileInfo& rhs) const
	{
		return (m_file == rhs.m_file && m_line == rhs.m_line && m_function == rhs.m_function);
	}

	bool operator!=(const SSourceFileInfo& rhs) const
	{
		return (m_file != rhs.m_file || m_line != rhs.m_line || m_function != rhs.m_function);
	}

private:
	string m_file;
	string m_function;
	int32  m_line;
};

} // ~Reflection namespace
} // ~Cry namespace
