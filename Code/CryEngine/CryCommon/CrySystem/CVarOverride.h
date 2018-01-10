// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryCore/CryCrc32.h>

template<class T>
class CVarOverride
{
public:
	using hash_type = uint32;
	using value_type = T;

	constexpr CVarOverride(const char* szName, const value_type value)
		: m_hash(CCrc32::ComputeLowercase_CompileTime(szName))
		, m_value(value)
	{
	}

	constexpr hash_type GetHashedName() const
	{
		return m_hash;
	}

	constexpr value_type GetValue() const
	{
		return m_value;
	}

private:
	const hash_type m_hash;
	const value_type m_value;
};

template<class T>
constexpr T GetCVarOverride(const typename CVarOverride<T>::hash_type hashedName, T defaultValue)
{
	return defaultValue;
}

template<class T>
inline constexpr T GetCVarOverride(const char* szName, const T defaultValue)
{
	return GetCVarOverride(CCrc32::ComputeLowercase_CompileTime(szName), defaultValue);
}

#if defined(CRY_CVAR_OVERRIDE_FILE)
#	include CRY_CVAR_OVERRIDE_FILE
#endif

namespace detail
{
	template<class T>
	inline constexpr T GetCVarOverrideImpl(const CVarOverride<T>* pContainer, const size_t numOverrides, const typename CVarOverride<T>::hash_type hashedName, const T defaultValue, const size_t index)
	{
		return index < numOverrides
			? (pContainer[index].GetHashedName() == hashedName ? pContainer[index].GetValue() : GetCVarOverrideImpl(pContainer, numOverrides, hashedName, defaultValue, index + 1))
			: defaultValue;
	}
}
#define GET_CVAR_IMPL(container, hashedName, defaultValue) \
	return detail::GetCVarOverrideImpl(container, CRY_ARRAY_COUNT(container), hashedName, defaultValue, 0)

#if defined(CVAR_FLOAT_OVERRIDES)
template<>
inline constexpr float GetCVarOverride<float>(const typename CVarOverride<float>::hash_type hashedName, const float defaultValue)
{
	GET_CVAR_IMPL(CVAR_FLOAT_OVERRIDES, hashedName, defaultValue);
}
#endif // defined(CVAR_FLOAT_OVERRIDES)

#if defined(CVAR_INT_OVERRIDES)
template<>
inline constexpr int GetCVarOverride<int>(const typename CVarOverride<int>::hash_type hashedName, const int defaultValue)
{
	GET_CVAR_IMPL(CVAR_INT_OVERRIDES, hashedName, defaultValue);
}
#endif // defined(CVAR_INT_OVERRIDES)

#if defined(CVAR_STRING_OVERRIDES)
template<>
inline constexpr const char* GetCVarOverride<const char*>(const typename CVarOverride<const char>::hash_type hashedName, const char* defaultValue)
{
	GET_CVAR_IMPL(CVAR_STRING_OVERRIDES, hashedName, defaultValue);
}
#endif // defined(CVAR_STRING_OVERRIDES)

#undef GET_CVAR_IMPL
