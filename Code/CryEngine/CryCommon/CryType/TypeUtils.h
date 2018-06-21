// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryType/TypeTraits.h>
#include <CryType/TypeOperators.h>

#include <CryCore/CryCrc32.h>
#include <CryString/CryString.h>

namespace Cry {
namespace Type {
namespace Utils {

// TODO: Move this to somewhere were everyone would expect it.
class CCompileTime_String
{
public:
	template<size_t LENGTH>
	constexpr CCompileTime_String(const char (&charArray)[LENGTH])
		: m_length(LENGTH)
		, m_pData(charArray)
	{}

	constexpr CCompileTime_String(const char* szData, size_t length)
		: m_length(length)
		, m_pData(szData)
	{}

	constexpr char operator[](size_t index) const
	{
		return index < m_length ? m_pData[index] : 0;
	}

	constexpr size_t length() const
	{
		return m_length;
	}

	constexpr const char* c_str() const
	{
		return m_pData;
	}

private:
	const size_t m_length;
	const char*  m_pData;
};
// ~TODO

template<typename T>
struct Explicit {};

template<typename T>
struct SCompileTime_TypeInfo
{
	constexpr static CCompileTime_String GetNamePointer()
	{
		return TypeName();
	}

	constexpr static CCompileTime_String GetName()
	{
		return TypeName();
	}

	constexpr static uint32 GetCrc32()
	{
#if _MSC_VER < 1910
		return CCrc32::Compute_CompileTime(__FUNCTION__);
#else
		return ComputeCrc(TypeName());
#endif
	}

private:
	constexpr static uint32 ComputeCrc(const CCompileTime_String str)
	{
		return CCrc32::Compute_CompileTime(str.c_str(), str.length());
	}

	constexpr static CCompileTime_String TypeName()
	{
#if defined (CRY_PLATFORM_ORBIS) /*|| defined(CRY_PLATFORM_ANDROID) || defined(CRY_PLATFORM_APPLE) || defined(CRY_PLATFORM_LINUX)*/
		return CCompileTime_String(__PRETTY_FUNCTION__ + GetTypeNameBegin(__PRETTY_FUNCTION__), GetTypeNameLength(__PRETTY_FUNCTION__));
#elif defined (CRY_PLATFORM_WINDOWS) || defined(CRY_PLATFORM_DURANGO)
		return CCompileTime_String(&__FUNCTION__[GetTypeNameBegin(__FUNCTION__)], GetTypeNameLength(__FUNCTION__));
#else
		return CCompileTime_String(__PRETTY_FUNCTION__);
#endif
	}

	enum { Offset = sizeof("Cry::Type::Utils::CompileTime_TypeInfo<") };

	template<size_t LENGTH>
	constexpr static uint32 GetTypeNameBegin(const char(&charArray)[LENGTH])
	{
		// TODO: Validate that this runs on the commented out platforms.
#if defined(CRY_PLATFORM_ORBIS) /*|| defined(CRY_PLATFORM_ANDROID) || defined(CRY_PLATFORM_APPLE) || defined(CRY_PLATFORM_LINUX)*/
		return static_cast<uint32>(sizeof("static Cry::Type::Utils::CCompileTime_String Cry::Type::Utils::SCompileTime_TypeInfo<") - 1);
#elif defined(CRY_PLATFORM_WINDOWS) || defined(CRY_PLATFORM_DURANGO)
		return static_cast<uint32>(
			charArray[Offset + 0] == 'c' &&
			charArray[Offset + 1] == 'l' &&
			charArray[Offset + 2] == 'a' &&
			charArray[Offset + 3] == 's' &&
			charArray[Offset + 4] == 's' &&
			charArray[Offset + 5] == ' ' ? sizeof("Cry::Type::Utils::CompileTime_TypeInfo<class ") :
			charArray[Offset + 0] == 's' &&
			charArray[Offset + 1] == 't' &&
			charArray[Offset + 2] == 'r' &&
			charArray[Offset + 3] == 'u' &&
			charArray[Offset + 4] == 'c' &&
			charArray[Offset + 5] == 't' &&
			charArray[Offset + 6] == ' ' ? sizeof("Cry::Type::Utils::CompileTime_TypeInfo<struct ") :
			charArray[Offset + 0] == 'e' &&
			charArray[Offset + 1] == 'n' &&
			charArray[Offset + 2] == 'u' &&
			charArray[Offset + 3] == 'm' &&
			charArray[Offset + 4] == ' ' ? sizeof("Cry::Type::Utils::CompileTime_TypeInfo<enum ") : Offset);
#else
		return 0;
#endif
	}

	template<size_t LENGTH>
	constexpr static uint32 GetTypeNameLength(const char(&charArray)[LENGTH])
	{
		return FindTypeNameEnd(charArray + GetTypeNameBegin(charArray), LENGTH - GetTypeNameBegin(charArray), 0);
	}

	constexpr static uint32 FindTypeNameEnd(const char* szData, size_t length, size_t fromIndex)
	{
		// TODO: Validate that this runs on the commented out platforms.
#if defined(CRY_PLATFORM_ORBIS) /*|| defined(CRY_PLATFORM_ANDROID) || defined(CRY_PLATFORM_APPLE) || defined(CRY_PLATFORM_LINUX)*/

		return szData[fromIndex + 0] == '>' &&
		       szData[fromIndex + 1] == ':' &&
		       szData[fromIndex + 2] == ':' &&
		       szData[fromIndex + 3] == 'T' &&
		       szData[fromIndex + 4] == 'y' &&
		       szData[fromIndex + 5] == 'p' &&
		       szData[fromIndex + 6] == 'e' &&
		       szData[fromIndex + 7] == 'N' &&
		       szData[fromIndex + 8] == 'a' &&
		       szData[fromIndex + 9] == 'm' &&
		       szData[fromIndex + 10] == 'e' &&
		       szData[fromIndex + 11] == '(' &&
		       szData[fromIndex + 12] == ')' &&
		       szData[fromIndex + 13] == ' ' &&
		       szData[fromIndex + 14] == '[' &&
		       szData[fromIndex + 15] == 'T' &&
		       szData[fromIndex + 16] == ' ' &&
		       szData[fromIndex + 17] == '=' &&
		       szData[fromIndex + 18] == ' '
		       ? szData[fromIndex - 1] != ' ' ? fromIndex : fromIndex - 1
		       : FindTypeNameEnd(szData, length, fromIndex + 1);
#elif defined (CRY_PLATFORM_WINDOWS) || defined(CRY_PLATFORM_DURANGO)
		return static_cast<uint32>(length - (szData[length - sizeof(" >::TypeName")] != ' ' ? sizeof(">::TypeName") : sizeof(" >::TypeName")));
#else
		return 0;
#endif
	}
};

}   // ~Utils namespace

// TODO: Where to move this?
template<typename TYPE>
struct PureType
{
	typedef typename std::remove_cv<typename std::remove_reference<typename std::remove_pointer<TYPE>::type>::type>::type Type;
};

template<typename TYPE, bool IS_ABSTRACT = std::is_abstract<TYPE>::value>
struct AlignOf
{
	static const size_t Value = alignof(TYPE);
};

template<typename TYPE>
struct AlignOf<TYPE, true>
{
	static const size_t Value = 0;
};
// ~TODO

} // ~Type namespace
} // ~Cry namespace
