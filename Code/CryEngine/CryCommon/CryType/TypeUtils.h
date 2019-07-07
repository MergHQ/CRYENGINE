// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryUtils/CompileTime.h>

#include <CryType/TypeTraits.h>
#include <CryType/TypeOperators.h>

#include <CryCore/CryCrc32.h>
#include <CryString/CryString.h>

namespace Cry {
namespace Type {
namespace Utils {

template<typename T>
struct SExplicit
{
};

template<typename T>
struct SCompileTime_TypeInfo
{
	// TODO: Does not work as expected on orbis-clang++, although clang on Linux works. Needs more investigation!
	constexpr static Cry::Utils::CompileTime::CString GetName()
	{
		return TypeName();
	}

	constexpr static uint32 GetCrc32()
	{
		// TODO: Workaround try build machines using VS2015 compiler. ComputeCrc(TypeName()) not constexpr.
#if defined (CRY_COMPILER_MSVC) && (_MSC_VER < 1910)
		return CCrc32::Compute_CompileTime(__FUNCTION__);
		// ~TODO
#else
		return ComputeCrc(TypeName());
#endif
	}

private:
	constexpr static uint32 ComputeCrc(Cry::Utils::CompileTime::CString typeName)
	{
		return CCrc32::Compute_CompileTime(typeName.GetBegin(), typeName.GetLength(), 0);
	}

	constexpr static Cry::Utils::CompileTime::CString TypeName()
	{
#if defined (CRY_COMPILER_MSVC)
		return Cry::Utils::CompileTime::CString(&__FUNCTION__[GetTypeNameBegin(__FUNCTION__)], GetTypeNameLength(__FUNCTION__));
#elif defined (CRY_COMPILER_CLANG)
		return Cry::Utils::CompileTime::CString(&__PRETTY_FUNCTION__[GetTypeNameBegin(__PRETTY_FUNCTION__)], GetTypeNameLength(__PRETTY_FUNCTION__));
#elif defined (CRY_COMPILER_GCC)
		// TODO: Workaround gcc7 on our try build machines (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66639)
		return Cry::Utils::CompileTime::CString(__PRETTY_FUNCTION__);
		//return Cry::Utils::CompileTime::CString(&__PRETTY_FUNCTION__[GetTypeNameBegin(__PRETTY_FUNCTION__)], GetTypeNameLength(__PRETTY_FUNCTION__));
		// ~TODO
#else
		static_assert(false, "Not supported for the active compiler.");
		return Cry::Utils::CompileTime::CString("");
#endif
	}

	enum
	{
		eTypeNameOffset_MSVC  = sizeof("Cry::Type::Utils::SCompileTime_TypeInfo<") - 1,
		eTypeNameOffset_CLANG = sizeof("static Cry::Utils::CompileTime::CString Cry::Type::Utils::SCompileTime_TypeInfo<") - 1,
		eTypeNameOffset_GCC   = sizeof("static constexpr Cry::Utils::CompileTime::CString Cry::Type::Utils::SCompileTime_TypeInfo<T>::TypeName() [with T = ") - 1
	};

	template<size_t LENGTH>
	constexpr static uint32 GetTypeNameBegin(const char(&charArray)[LENGTH])
	{
#if defined (CRY_COMPILER_MSVC)
		return static_cast<uint32>(
			charArray[eTypeNameOffset_MSVC + 0] == 'c' &&
			charArray[eTypeNameOffset_MSVC + 1] == 'l' &&
			charArray[eTypeNameOffset_MSVC + 2] == 'a' &&
			charArray[eTypeNameOffset_MSVC + 3] == 's' &&
			charArray[eTypeNameOffset_MSVC + 4] == 's' &&
			charArray[eTypeNameOffset_MSVC + 5] == ' ' ? sizeof("Cry::Type::Utils::SCompileTime_TypeInfo<class") :
			charArray[eTypeNameOffset_MSVC + 0] == 's' &&
			charArray[eTypeNameOffset_MSVC + 1] == 't' &&
			charArray[eTypeNameOffset_MSVC + 2] == 'r' &&
			charArray[eTypeNameOffset_MSVC + 3] == 'u' &&
			charArray[eTypeNameOffset_MSVC + 4] == 'c' &&
			charArray[eTypeNameOffset_MSVC + 5] == 't' &&
			charArray[eTypeNameOffset_MSVC + 6] == ' ' ? sizeof("Cry::Type::Utils::SCompileTime_TypeInfo<struct") :
			charArray[eTypeNameOffset_MSVC + 0] == 'e' &&
			charArray[eTypeNameOffset_MSVC + 1] == 'n' &&
			charArray[eTypeNameOffset_MSVC + 2] == 'u' &&
			charArray[eTypeNameOffset_MSVC + 3] == 'm' &&
			charArray[eTypeNameOffset_MSVC + 4] == ' ' ? sizeof("Cry::Type::Utils::SCompileTime_TypeInfo<enum") : eTypeNameOffset_MSVC);
#elif defined (CRY_COMPILER_CLANG)
		return static_cast<uint32>(eTypeNameOffset_CLANG);
#elif defined (CRY_COMPILER_GCC)
		return static_cast<uint32>(eTypeNameOffset_GCC);
#else
		static_assert(false, "Not supported for the active compiler.");
		return 0;
#endif
	}

	template<size_t LENGTH>
	constexpr static uint32 GetTypeNameLength(const char(&charArray)[LENGTH])
	{
		return FindTypeNameEnd(&charArray[GetTypeNameBegin(charArray)], LENGTH - 1 - GetTypeNameBegin(charArray), 0);
	}

	constexpr static uint32 FindTypeNameEnd(const char* szData, size_t length, size_t fromIndex)
	{
#if defined (CRY_COMPILER_MSVC)
		return static_cast<uint32>(length - (szData[length - sizeof(">::TypeName")] != ' ' ? sizeof(">::TypeName") - 1 : sizeof(">::TypeName")));
#elif defined (CRY_COMPILER_GCC)
		return static_cast<uint32>(length - 1);
#elif defined (CRY_COMPILER_CLANG)
		return static_cast<uint32>(
			szData[fromIndex + 0] == '>' &&
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
			szData[fromIndex + 18] == ' ' ? szData[fromIndex - 1] != ' ' ? fromIndex : fromIndex - 1 : FindTypeNameEnd(szData, length, fromIndex + 1));
#else
		static_assert(false, "Not supported for the active compiler.");
		return 0;
#endif
	}
};

}   // ~Utils namespace

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

} // ~Type namespace
} // ~Cry namespace
