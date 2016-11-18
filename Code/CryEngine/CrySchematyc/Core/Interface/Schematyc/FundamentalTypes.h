// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryEntitySystem/IEntity.h>
#include <CrySerialization/Forward.h>

#include "Schematyc/Utils/GUID.h"
#include "Schematyc/Utils/PreprocessorUtils.h"

#define SCHEMATYC_SOURCE_FILE_INFO Schematyc::SSourceFileInfo(SCHEMATYC_FILE_NAME, SCHEMATYC_LINE_NUMBER)

namespace Schematyc
{
enum : uint32
{
	InvalidId = ~0u,
	InvalidIdx = ~0u
};

enum class EVisitStatus
{
	Continue,   // Continue visit.
	Recurse,    // Continue visit recursively (if applicable).
	Stop,       // Stop visit.
	Error       // Stop visit because error occurred.
};

enum class EVisitResult
{
	Complete,   // Visit was completed.
	Stopped,    // Visit was stopped before completion.
	Error       // Visit was stopped before completion error occurred.
};

// In-place memory allocation parameters.
struct SInPlaceAllocationParams
{
	explicit inline SInPlaceAllocationParams(uint32 _capacity, void* _pStorage)
		: capacity(_capacity)
		, pStorage(_pStorage)
	{}

	uint32 capacity;
	void*  pStorage;
};

struct SSourceFileInfo
{
	explicit inline SSourceFileInfo(const char* _szFileName, uint32 _lineNumber)
		: szFileName(_szFileName)
		, lineNumber(_lineNumber)
	{}

	const char* szFileName;
	uint32      lineNumber;
};

enum class EDomain /* : uint8*/ // Ideally this would have uint8 as an underlying type but that screws with serialization.
{
	Unknown,
	Env,
	Script
};

struct SElementId
{
	inline SElementId()
		: domain(EDomain::Unknown)
		, guid(SGUID())
	{}

	inline SElementId(EDomain _domain, const SGUID& _guid)
		: domain(_domain)
		, guid(_guid)
	{}

	inline void Serialize(Serialization::IArchive& archive)
	{
		archive(domain, "domain");
		archive(guid, "guid");
	}

	inline bool operator==(const SElementId& rhs) const
	{
		return (domain == rhs.domain) && (guid == rhs.guid);
	}

	inline bool operator!=(const SElementId& rhs) const
	{
		return (domain != rhs.domain) || (guid != rhs.guid);
	}

	EDomain domain;
	SGUID   guid;
};

enum class EOverridePolicy // #SchematycTODO : Move to IScriptElement.h and rename EScriptElementOverridePolicy?
{
	Default,
	Override,
	Final
};

enum class ESimulationMode
{
	Idle,   // Not running.
	Game,   // Running in game mode.
	Editor, // Running in editor mode.
	Preview // Running in preview window.
};

enum class EComparisonMode : int
{
	NotSet = 0,
	Equal,
	NotEqual,
	LessThan,
	LessThanOrEqual,
	GreaterThan,
	GreaterThanOrEqual,
};

enum class ObjectId : uint32
{
	Invalid = 0xffffffff
};

inline bool Serialize(Serialization::IArchive& archive, ObjectId& value, const char* szName, const char* szLabel)
{
	if (!archive.isEdit())
	{
		return archive(static_cast<uint32>(value), szName, szLabel);
	}
	return true;
}

constexpr const char* g_szCrytek = "Crytek GmbH";
} // Schematyc
