// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryCore/CryCrc32.h>
#include "CryUtils.h"

enum class CVarOverrideType
{
	Numeric,
	String,
};

template<CVarOverrideType T>
class CVarOverride
{
public:
	using hash_type = uint32;
	using value_type = typename std::conditional<T == CVarOverrideType::Numeric, double, const char*>::type;

	constexpr CVarOverride(const char* szName, const value_type value)
		: m_hash(CCrc32::ComputeLowercase_CompileTime(szName))
		, m_value(value)
	{
	}

	constexpr hash_type GetHashedName() const
	{
		return m_hash;
	}

	template<class U>
	constexpr U GetValue() const
	{
		return static_cast<U>(m_value);
	}

private:
	const hash_type m_hash;
	const value_type m_value;
};

#ifndef USE_RUNTIME_CVAR_OVERRIDES

struct CVarWhitelistEntry
{
public:
	using hash_type = uint32;

	constexpr CVarWhitelistEntry(const char* szName)
		: m_hash(CCrc32::ComputeLowercase_CompileTime(szName))
	{
	}

	constexpr hash_type GetHashedName() const
	{
		return m_hash;
	}

private:
	const hash_type m_hash;
};

// Integers & Floats
#define ADD_CVAR_OVERRIDE_NUMERIC(name, value) { name, value },
#define ADD_CVAR_OVERRIDE_STRING(name, value)

constexpr CVarOverride<CVarOverrideType::Numeric> g_engineCvarNumericOverrides[] = 
{
#if defined(CRY_CVAR_OVERRIDE_FILE)
#	include CRY_CVAR_OVERRIDE_FILE
#endif
	{"_CVarNumericOverrideDummy", 0} // At least 1 element is required for compilation (cannot have empty array) and to remove trailing comma
};

#undef ADD_CVAR_OVERRIDE_NUMERIC
#undef ADD_CVAR_OVERRIDE_STRING

// strings
#define ADD_CVAR_OVERRIDE_NUMERIC(name, value)
#define ADD_CVAR_OVERRIDE_STRING(name, value) { name, value },

constexpr CVarOverride<CVarOverrideType::String> g_engineCvarStringOverrides[] =
{
#if defined(CRY_CVAR_OVERRIDE_FILE)
#	include CRY_CVAR_OVERRIDE_FILE
#endif
	{"_CVarNumericStringDummy", ""} // At least 1 element is required for compilation (cannot have empty array) and to remove trailing comma
};

#undef ADD_CVAR_OVERRIDE_NUMERIC
#undef ADD_CVAR_OVERRIDE_STRING

#if defined(CRY_CVAR_WHITELIST_FILE)
#	include CRY_CVAR_WHITELIST_FILE
#endif

namespace detail
{
	template<CVarOverrideType T, class U>
	inline constexpr U GetCVarOverrideImpl(const CVarOverride<T>* pContainer, const size_t numOverrides, const typename CVarOverride<T>::hash_type hashedName, const U defaultValue, const size_t index)
	{
		return index < numOverrides
			? (pContainer[index].GetHashedName() == hashedName ? pContainer[index].template GetValue<U>() : GetCVarOverrideImpl(pContainer, numOverrides, hashedName, defaultValue, index + 1))
			: defaultValue;
	}

	constexpr bool IsCVarWhitelistedImpl(const CVarWhitelistEntry* pContainer, const size_t numWhitelistEntries, const CVarWhitelistEntry::hash_type hashedName, const size_t index)
	{
		return index < numWhitelistEntries
			? (pContainer[index].GetHashedName() == hashedName ? true : IsCVarWhitelistedImpl(pContainer, numWhitelistEntries, hashedName, index + 1))
			: false;
	}
}

constexpr float GetCVarOverride(const char* szName, const float defaultValue)
{
	return detail::GetCVarOverrideImpl(g_engineCvarNumericOverrides, CRY_ARRAY_COUNT(g_engineCvarNumericOverrides), CCrc32::ComputeLowercase_CompileTime(szName), defaultValue, 0);
}

constexpr int GetCVarOverride(const char* szName, const int defaultValue)
{
	return detail::GetCVarOverrideImpl(g_engineCvarNumericOverrides, CRY_ARRAY_COUNT(g_engineCvarNumericOverrides), CCrc32::ComputeLowercase_CompileTime(szName), defaultValue, 0);
}

constexpr int GetCVarOverride(const char* szName, const int64 defaultValue)
{
	return detail::GetCVarOverrideImpl(g_engineCvarNumericOverrides, CRY_ARRAY_COUNT(g_engineCvarNumericOverrides), CCrc32::ComputeLowercase_CompileTime(szName), static_cast<int>(defaultValue), 0);
}

constexpr const char* GetCVarOverride(const char* szName, const char* defaultValue)
{
	return detail::GetCVarOverrideImpl(g_engineCvarStringOverrides, CRY_ARRAY_COUNT(g_engineCvarStringOverrides), CCrc32::ComputeLowercase_CompileTime(szName), defaultValue, 0);
}

#else // USE_RUNTIME_CVAR_OVERRIDES

template<class T>
inline constexpr T GetCVarOverride(const char* szName, const T defaultValue)
{
	return defaultValue;
}

#endif // USE_RUNTIME_CVAR_OVERRIDES

constexpr bool IsCVarWhitelisted(const char* szName)
{
#if defined(CVARS_WHITELIST) && !defined(USE_RUNTIME_CVAR_OVERRIDES)
#	if defined(CVAR_WHITELIST_ENTRIES)
	return detail::IsCVarWhitelistedImpl(CVAR_WHITELIST_ENTRIES, CRY_ARRAY_COUNT(CVAR_WHITELIST_ENTRIES), CCrc32::ComputeLowercase_CompileTime(szName), 0);
#	else
	return false;
#	endif // defined(CVAR_WHITELIST_ENTRIES)
#else
	return true;
#endif // defined(CVARS_WHITELIST)
}