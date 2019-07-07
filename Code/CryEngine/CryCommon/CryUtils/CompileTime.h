// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include <CryCore/CryCrc32.h>

namespace Cry {
namespace Utils {
namespace CompileTime {

template<typename T>
class CArray
{
public:
	template<size_t LENGTH>
	constexpr CArray(const T(&data)[LENGTH])
		: m_length(LENGTH)
		, m_pData(data)
	{
	}

	constexpr CArray(const T* szData, size_t length)
		: m_length(length)
		, m_pData(szData)
	{
	}

	constexpr const T* GetBegin() const
	{
		return m_pData;
	}

	constexpr const T* GetEnd() const
	{
		return m_pData + m_length;
	}

	constexpr size_t GetLength() const
	{
		return m_length;
	}

	constexpr T operator[](size_t index) const
	{
		return index < m_length ? m_pData[index] : 0;
	}

private:
	const size_t m_length;
	const T*     m_pData;
};

using CString = CArray<char>;
using CWString = CArray<wchar_t>;

class CHashString
{
public:
	template<size_t LENGTH>
	constexpr CHashString(const char(&data)[LENGTH])
		: m_length(LENGTH)
		, m_pData(data)
		, m_crc32(CCrc32::Compute_CompileTime(data, LENGTH, 0))
	{
	}

	constexpr CHashString(const char* szData, size_t length)
		: m_length(length)
		, m_pData(szData)
		, m_crc32(CCrc32::Compute_CompileTime(szData, length, 0))
	{
	}

	constexpr const char* GetBegin() const
	{
		return m_pData;
	}

	constexpr const char* GetEnd() const
	{
		return m_pData + m_length;
	}

	constexpr size_t GetLength() const
	{
		return m_length;
	}

	constexpr uint32 GetCrc32() const
	{
		return m_crc32;
	}

	constexpr char operator[](size_t index) const
	{
		return index < m_length ? m_pData[index] : 0;
	}

private:
	const size_t m_length;
	const char*  m_pData;
	uint32       m_crc32;
};

} // ~CompileTime namespace
} // ~Utils namespace
} // ~Cry namespace
