// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryType/TypeUtils.h>
#include <CryType/TypeTraits.h>

#include <CryString/StringUtils.h>
#include <CrySTL/type_traits.h>

namespace Cry {
namespace Type {

// TODO: Move this to a better location (maybe TypeTraits.h?)
namespace Helpers
{

template<class T>
using HasToStringFunc = decltype(std::declval<T>().ToString());

template<class TYPE>
struct HasToString
{
	static constexpr bool value = stl::is_detected_exact<string, HasToStringFunc, TYPE>::value;
};

}
// ~TODO

// Function: Cast to Base
class CBaseCastFunction
{
public:
	typedef void*(* FuncPtr)(void* pClassInstance);
	constexpr CBaseCastFunction(FuncPtr ptr = nullptr)
		: m_ptr(ptr)
	{}

	operator bool() const { return m_ptr != nullptr; }
	void* operator()(void* pClassInstance)
	{
		return m_ptr(pClassInstance);
	}

private:
	FuncPtr m_ptr;
};

template<typename TYPE, typename BASE_TYPE, bool IS_BASE = std::is_base_of<BASE_TYPE, typename Type::PureType<TYPE>::Type>::value>
class CAdapter_BaseCast : public CBaseCastFunction
{

};

template<typename TYPE, typename BASE_TYPE>
class CAdapter_BaseCast<TYPE, BASE_TYPE, true> : public CBaseCastFunction
{
public:
	CAdapter_BaseCast()
		: CBaseCastFunction(&CAdapter_BaseCast::Execute)
	{}

private:
	static void* Execute(void* pClassInstance)
	{
		return static_cast<BASE_TYPE*>(static_cast<TYPE*>(pClassInstance));
	}
};

// Function: Serialize
class CSerializeFunction
{
public:
	typedef bool (* FuncPtr)(Serialization::IArchive& archive, void* pValue, const char* szName, const char* szLabel);
	constexpr CSerializeFunction(FuncPtr ptr = nullptr)
		: m_ptr(ptr)
	{}

	operator bool() const { return m_ptr != nullptr; }
	bool operator()(Serialization::IArchive& archive, void* pValue, const char* szName, const char* szLabel)
	{
		return m_ptr(archive, pValue, szName, szLabel);
	}

private:
	FuncPtr m_ptr;
};

template<typename TYPE, bool IS_SUPPORTED = Traits::IsSerializable<TYPE>::value>
class CAdapter_Serialize : public CSerializeFunction
{

};

template<typename TYPE>
class CAdapter_Serialize<TYPE, true> : public CSerializeFunction
{
public:
	CAdapter_Serialize()
		: CSerializeFunction(&CAdapter_Serialize::Execute)
	{}

private:
	static bool Execute(Serialization::IArchive& archive, void* pValue, const char* szName, const char* szLabel)
	{
		TYPE& value = *static_cast<TYPE*>(pValue);
		return archive(value, szName, szLabel);
	}
};

// Function: toString
class CToStringFunction
{
public:
	typedef string (* FuncPtr)(const void* pClassInstance);
	constexpr CToStringFunction(FuncPtr ptr = nullptr)
		: m_ptr(ptr)
	{}

	operator bool() const { return m_ptr != nullptr; }
	string operator()(const void* pClassInstance)
	{
		return m_ptr(pClassInstance);
	}

private:
	FuncPtr m_ptr;
};

template<typename TYPE, typename = void>
class CAdapter_ToString : public CToStringFunction
{

};

// unsigned int
template<>
class CAdapter_ToString<unsigned int> : public CToStringFunction
{
public:
	CAdapter_ToString()
		: CToStringFunction(&CAdapter_ToString::Execute)
	{}

private:
	static string Execute(const void* pInteger)
	{
		return CryStringUtils::toString(*reinterpret_cast<const unsigned int*>(pInteger));
	}
};

// signed int
template<>
class CAdapter_ToString<signed int> : public CToStringFunction
{
public:
	CAdapter_ToString()
		: CToStringFunction(&CAdapter_ToString::Execute)
	{}

private:
	static string Execute(const void* pInteger)
	{
		return CryStringUtils::toString(*reinterpret_cast<const signed int*>(pInteger));
	}
};

// float
template<>
class CAdapter_ToString<float> : public CToStringFunction
{
public:
	CAdapter_ToString()
		: CToStringFunction(&CAdapter_ToString::Execute)
	{}

private:
	static string Execute(const void* pInteger)
	{
		return CryStringUtils::toString(*reinterpret_cast<const float*>(pInteger));
	}
};

// bool
template<>
class CAdapter_ToString<bool> : public CToStringFunction
{
public:
	CAdapter_ToString()
		: CToStringFunction(&CAdapter_ToString::Execute)
	{}

private:
	static string Execute(const void* pInteger)
	{
		return CryStringUtils::toString(*reinterpret_cast<const bool*>(pInteger));
	}
};

// ->ToString()
template<typename TYPE>
class CAdapter_ToString<TYPE, typename std::enable_if<Helpers::HasToString<TYPE>::value>::type> : public CToStringFunction
{
public:
	CAdapter_ToString()
		: CToStringFunction(&CAdapter_ToString::Execute)
	{}

private:
	static string Execute(const void* pClassInstance)
	{
		return reinterpret_cast<const TYPE*>(pClassInstance)->ToString();
	}
};

} // ~Cry namespace
} // ~Reflection namespace
