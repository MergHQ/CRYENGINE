// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

//! \cond INTERNAL

#pragma once

#include <CryString/CryFixedString.h>
#include <array>

//////////////////////////////////////////////////////////////////////////
/** This class keeps file version information.
 */
struct SFileVersion
{
	constexpr SFileVersion()
		: v({ { 0, 0, 0, 0 } })
	{
	}

	constexpr SFileVersion(int a, int b, int c, int d)
		: v({ { d, c, b, a } })
	{
	}

	explicit SFileVersion(const char* szVersion)
	{
		Set(szVersion);
	}

	void Set(const char* szVersion)
	{
		CRY_ASSERT(szVersion != nullptr, "No version string passed");
		if (szVersion != nullptr)
		{
			v.fill(0);

			const char* it = szVersion;
			for (int i = v.size() - 1; i >= 0; --i)
			{
				v[i] = atoi(it);
				it = strchr(it + 1, '.');
				if (it == nullptr)
					break;
				++it; // Skip the '.'
			}
		}
	}

	constexpr bool operator<(const SFileVersion& rhs) const
	{
		return v[3] < rhs.v[3]
			|| (v[3] == rhs.v[3] && v[2] < rhs.v[2])
			|| (v[3] == rhs.v[3] && v[2] == rhs.v[2] && v[1] < rhs.v[1])
			|| (v[3] == rhs.v[3] && v[2] == rhs.v[2] && v[1] == rhs.v[1] && v[0] < rhs.v[0]);
	}

	constexpr bool operator>(const SFileVersion& rhs) const
	{
		return rhs < *this;
	}

	constexpr bool operator<=(const SFileVersion& rhs) const
	{
		return !(rhs < *this);
	}

	constexpr bool operator>=(const SFileVersion& rhs) const
	{
		return !(*this < rhs);
	}

	constexpr bool operator==(const SFileVersion& rhs) const
	{
		return v[0] == rhs.v[0]
			&& v[1] == rhs.v[1]
			&& v[2] == rhs.v[2]
			&& v[3] == rhs.v[3];
	}

	constexpr bool operator!=(const SFileVersion& rhs) const
	{
		return !(*this == rhs);
	}

	int& operator[](size_t i)
	{
		CRY_ASSERT(i < v.size(), "Invalid index");
		return v[i];
	}

	constexpr int operator[](size_t i) const
	{
		return v[i];
	}

	void ToShortString(char* s) const
	{
		sprintf(s, "%d.%d.%d", v[2], v[1], v[0]);
	}

	void ToString(char* s) const
	{
		sprintf(s, "%d.%d.%d.%d", v[3], v[2], v[1], v[0]);
	}

	CryFixedStringT<32> ToString() const
	{
		CryFixedStringT<32> str;
		str.Format("%d.%d.%d.%d", v[3], v[2], v[1], v[0]);
		return str;
	}

protected:
	std::array<int, 4> v;
};

//! \endcond
