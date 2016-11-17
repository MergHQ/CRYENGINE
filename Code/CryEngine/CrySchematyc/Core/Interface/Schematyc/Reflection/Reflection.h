// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// Use Schematyc::GetTypeInfo<TYPE>() to get information for a specific type.
// Calling Schematyc::GetTypeInfo where TYPE is not reflected will result in the following compile error: 'Type must be reflected, see Reflection.h for details!'
//
// In order to reflect a type you must declare a specialization of the ReflectSchematycType function.
// This specialization can be intrusive (a static method) or non-intrusive (a free function declared in the same namespace).
//
// Intrusive reflection:
//
//   struct SMyStruct
//   {
//     static Schematyc::SGUID ReflectSchematycType(Schematyc::CTypeInfo<SMyStruct>& typeInfo);
//   };
//
// Non-intrusive reflection:
//
//   static inline Schematyc::SGUID ReflectSchematycType(Schematyc::CTypeInfoDecorator<SMyStruct>& decorator);
//
// At the bare minimum ReflectSchematycType must return a guid unique to the type. Additional information can be reflected
// via the typeInfo parameter e.g. typeInfo.AddMember() can be used to reflect class/struct members.

#pragma once

#include <type_traits>

#include <CrySerialization/Color.h>
#include <CrySerialization/Enum.h>
#include <CrySerialization/Forward.h>
#include <CrySerialization/Math.h>

#include "Schematyc/FundamentalTypes.h"
#include "Schematyc/Utils/Assert.h"
#include "Schematyc/Utils/EnumFlags.h"
#include "Schematyc/Utils/GUID.h"
#include "Schematyc/Utils/IString.h"
#include "Schematyc/Utils/StackString.h"
#include "Schematyc/Utils/TypeName.h"
#include "Schematyc/Utils/TypeUtils.h"

#define SCHEMATYC_VERIFY_TYPE_IS_REFLECTED(type) static_assert(Schematyc::SReflectionMethod<type>::value != Schematyc::EReflectionMethod::None, "Type must be reflected, see Reflection.h for details!")

namespace Schematyc
{
enum class ETypeClassification
{
	Void,
	Fundamental,
	Enum,
	Class,
	Array // #SchematycTODO : Remove?
};

template<typename TYPE> struct STypeClassification
{
	static const ETypeClassification value = std::is_array<TYPE>::value ? ETypeClassification::Array : std::is_class<TYPE>::value ? ETypeClassification::Class : std::is_enum<TYPE>::value ? ETypeClassification::Enum : ETypeClassification::Fundamental;
};

template<typename TYPE, ETypeClassification TYPE_CLASSIFICATION = STypeClassification<TYPE>::value> class CTypeInfo;

#if CRY_PLATFORM_WINDOWS
template<typename TYPE> void ReflectSchematycType(CTypeInfo<TYPE>& typeInfo);
#else
template<typename TYPE> SGUID ReflectSchematycType(CTypeInfo<TYPE>& typeInfo);
#endif

template<typename TYPE> const CTypeInfo<TYPE>& GetTypeInfo();

enum EReflectionMethod
{
	None,
	NonIntrusive,
	Intrusive
};

template<typename TYPE> struct SReflectionMethod
{
private:

	typedef char Yes[1];
	typedef char No[2];

	template<typename FUNCTION_PTR_TYPE, FUNCTION_PTR_TYPE> struct SVerify;

private:

	template<typename TEST_TYPE> static Yes& Test(SVerify<SGUID (*)(CTypeInfo<TEST_TYPE>&), & TEST_TYPE::ReflectSchematycType>*);
	template<typename> static No&            Test(...);

private:

	static const bool intrusive = sizeof(Test<TYPE>(0)) == sizeof(Yes);

#if CRY_PLATFORM_WINDOWS
	static const bool non_intrusive = std::is_same<decltype(ReflectSchematycType(std::declval<CTypeInfo<TYPE>>())), SGUID>::value;
#else
	static const bool non_intrusive = true; // #SchematycTODO : Fix SFINAE test for non-Windows platforms. Current implementation results in following error in Clang: no matching function for call to 'ReflectSchematycType'.
#endif

public:

	static const EReflectionMethod value = intrusive ? EReflectionMethod::Intrusive : (non_intrusive ? EReflectionMethod::NonIntrusive : EReflectionMethod::None);
};

enum class ETypeFlags
{
	IsString = BIT(0)
};

typedef CEnumFlags<ETypeFlags> TypeFlags;

struct SCommonTypeInfoMethods
{
	typedef void*(* DefaultConstruct)(void* pPlacement);
	typedef void*(* CopyConstruct)(void* pPlacement, const void* pRHS);
	typedef void (* Destruct)(void* pPlacement);
	typedef bool (* Equals)(const void* pLHS, const void* pRHS);
	typedef void (* CopyAssign)(void* pLHS, const void* pRHS);
	typedef void (* ToString)(IString& output, const void* pInput);
	typedef bool (* Serialize)(Serialization::IArchive& archive, void* pValue, const char* szName, const char* szLabel);

	inline SCommonTypeInfoMethods()
		: defaultConstruct(nullptr)
		, copyConstruct(nullptr)
		, destruct(nullptr)
		, equals(nullptr)
		, copyAssign(nullptr)
		, toString(nullptr)
		, serialize(nullptr)
	{}

	DefaultConstruct defaultConstruct;
	CopyConstruct    copyConstruct;
	Destruct         destruct;
	Equals           equals;
	CopyAssign       copyAssign;
	ToString         toString;
	Serialize        serialize;
};

class CCommonTypeInfo
{
	CCommonTypeInfo(const CCommonTypeInfo&) = delete;
	CCommonTypeInfo& operator=(const CCommonTypeInfo&) = delete;

public:

	inline CCommonTypeInfo()
		: m_classification(ETypeClassification::Void)
		, m_size(0)
	{}

	inline ETypeClassification GetClassification() const
	{
		return m_classification;
	}

	inline uint32 GetSize() const
	{
		return m_size;
	}

	inline const CTypeName& GetName() const
	{
		return m_name;
	}

	inline void SetGUID(const SGUID& guid)
	{
		m_guid = guid;
	}

	inline const SGUID& GetGUID() const
	{
		return m_guid;
	}

	inline void SetFlags(const TypeFlags& flags)
	{
		m_flags = flags;
	}

	inline const TypeFlags& GetFlags() const
	{
		return m_flags;
	}

	inline void SetMethods(const SCommonTypeInfoMethods& methods)
	{
		m_methods = methods;
	}

	inline const SCommonTypeInfoMethods& GetMethods() const
	{
		return m_methods;
	}

	inline bool operator==(const CCommonTypeInfo& rhs) const
	{
		return m_guid == rhs.m_guid;
	}

protected:

	explicit inline CCommonTypeInfo(ETypeClassification classification, uint32 size, const CTypeName& name)
		: m_classification(classification)
		, m_size(size)
		, m_name(name)
	{}

protected:

	ETypeClassification    m_classification;
	uint32                 m_size;
	CTypeName              m_name;
	SGUID                  m_guid;
	TypeFlags              m_flags;
	SCommonTypeInfoMethods m_methods;
};

class CEnumTypeInfo : public CCommonTypeInfo
{
protected:

	inline CEnumTypeInfo(uint32 size, const CTypeName& name)
		: CCommonTypeInfo(ETypeClassification::Enum, size, name)
	{}
};

struct SClassTypeInfoBase
{
	inline SClassTypeInfoBase(const CCommonTypeInfo& _typeInfo, ptrdiff_t _offset)
		: typeInfo(_typeInfo)
		, offset(_offset)
	{}

	const CCommonTypeInfo& typeInfo;
	ptrdiff_t              offset;
};

typedef DynArray<SClassTypeInfoBase> ClassTypeInfoBases;

struct SClassTypeInfoMember
{
	inline SClassTypeInfoMember(const CCommonTypeInfo& _typeInfo, ptrdiff_t _offset, uint32 _id, const char* _szLabel, const char* _szDescription)
		: typeInfo(_typeInfo)
		, offset(_offset)
		, id(_id)
		, szLabel(_szLabel)
		, szDescription(_szDescription)
	{}

	const CCommonTypeInfo& typeInfo;
	ptrdiff_t              offset;
	uint32                 id;
	const char*            szLabel;
	const char*            szDescription;
};

typedef DynArray<SClassTypeInfoMember> ClassTypeInfoMembers;

class CClassTypeInfo : public CCommonTypeInfo
{
public:

	inline const ClassTypeInfoBases& GetBases() const
	{
		return m_bases;
	}

	inline const ClassTypeInfoMembers& GetMembers() const
	{
		return m_members;
	}

protected:

	inline CClassTypeInfo(uint32 size, const CTypeName& name)
		: CCommonTypeInfo(ETypeClassification::Class, size, name)
	{}

protected:

	ClassTypeInfoBases   m_bases;
	ClassTypeInfoMembers m_members;
};

class CArrayTypeInfo : public CCommonTypeInfo
{
protected:

	inline CArrayTypeInfo(uint32 size, const CTypeName& name, uint32 length)
		: CCommonTypeInfo(ETypeClassification::Array, size, name)
		, m_length(length)
	{}

public:

	inline uint32 GetLength() const
	{
		return m_length;
	}

protected:

	uint32 m_length;
};

namespace Private
{
template<typename TYPE, bool TYPE_IS_DEFAULT_CONSTRUCTIBLE = std::is_default_constructible<TYPE>::value> struct SDefaultConstructSelector;

template<typename TYPE> struct SDefaultConstructSelector<TYPE, true>
{
	static void* DefaultConstruct(void* pPlacement)
	{
		return new(pPlacement) TYPE();
	}

	static inline SCommonTypeInfoMethods::DefaultConstruct Select()
	{
		return &DefaultConstruct;
	}
};

template<typename TYPE> struct SDefaultConstructSelector<TYPE, false>
{
	static inline SCommonTypeInfoMethods::DefaultConstruct Select()
	{
		return nullptr;
	}
};

template<typename TYPE, bool TYPE_IS_COPY_CONSTRUCTIBLE = std::is_copy_constructible<TYPE>::value> struct SCopyConstructSelector;

template<typename TYPE> struct SCopyConstructSelector<TYPE, true>
{
	static void* CopyConstruct(void* pPlacement, const void* pRHS)
	{
		return new(pPlacement) TYPE(*static_cast<const TYPE*>(pRHS));
	}

	static inline SCommonTypeInfoMethods::CopyConstruct Select()
	{
		return &CopyConstruct;
	}
};

template<typename TYPE> struct SCopyConstructSelector<TYPE, false>
{
	static inline SCommonTypeInfoMethods::CopyConstruct Select()
	{
		return nullptr;
	}
};

template<typename TYPE> struct SDestructSelector
{
	static void Destruct(void* pPlacement)
	{
		return static_cast<TYPE*>(pPlacement)->~TYPE();
	}

	static inline SCommonTypeInfoMethods::Destruct Select()
	{
		return &Destruct;
	}
};

template<typename TYPE, bool TYPE_HAS_EQUALS_OPRATOR = HasOperator::SEquals<TYPE>::value> struct SEqualsSelector;

template<typename TYPE> struct SEqualsSelector<TYPE, true>
{
	static bool Equals(const void* pLHS, const void* pRHS)
	{
		const TYPE& lhs = *static_cast<const TYPE*>(pLHS);
		const TYPE& rhs = *static_cast<const TYPE*>(pRHS);
		return lhs == rhs;
	}

	static inline SCommonTypeInfoMethods::Equals Select()
	{
		return &Equals;
	}
};

template<typename TYPE> struct SEqualsSelector<TYPE, false>
{
	static inline SCommonTypeInfoMethods::Equals Select()
	{
		return nullptr;
	}
};

template<typename TYPE, bool TYPE_HAS_COPY_ASSIGNMENT_OPERATOR = std::is_copy_assignable<TYPE>::value> struct SCopyAssignSelector;

template<typename TYPE> struct SCopyAssignSelector<TYPE, true>
{
	static void CopyAssign(void* pLHS, const void* pRHS)
	{
		TYPE& lhs = *static_cast<TYPE*>(pLHS);
		const TYPE& rhs = *static_cast<const TYPE*>(pRHS);
		lhs = rhs;
	}

	static inline SCommonTypeInfoMethods::CopyAssign Select()
	{
		return &CopyAssign;
	}
};

template<typename TYPE> struct SCopyAssignSelector<TYPE, false>
{
	static inline SCommonTypeInfoMethods::CopyAssign Select()
	{
		return nullptr;
	}
};

template<typename FUNCTION_PTR_TYPE, FUNCTION_PTR_TYPE FUNCTION_PTR> struct SToStringBinder;

template<typename TYPE, void(TYPE::* FUNCTION_PTR)(IString&) const> struct SToStringBinder<void (TYPE::*)(IString&) const, FUNCTION_PTR>
{
	static void ToString(IString& output, const void* pInput)
	{
		(static_cast<const TYPE*>(pInput)->*FUNCTION_PTR)(output);
	}
};

template<typename TYPE, void(* FUNCTION_PTR)(IString&, const TYPE&)> struct SToStringBinder<void (*)(IString&, const TYPE&), FUNCTION_PTR>
{
	static void ToString(IString& output, const void* pInput)
	{
		(*FUNCTION_PTR)(output, *static_cast<const TYPE*>(pInput));
	}
};

template<typename TYPE, ETypeClassification TYPE_CLASSIFICATION = STypeClassification<TYPE>::value> struct SToStringSelector
{
	static inline SCommonTypeInfoMethods::ToString Select()
	{
		return nullptr;
	}
};

template<typename TYPE> struct SToStringSelector<TYPE, ETypeClassification::Enum>
{
	static void ToString(IString& output, const void* pInput)
	{
		const Serialization::EnumDescription& enumDescription = Serialization::getEnumDescription<TYPE>();
		output.assign(enumDescription.labelByIndex(enumDescription.indexByValue(static_cast<int>(*static_cast<const TYPE*>(pInput)))));
	}

	static inline SCommonTypeInfoMethods::ToString Select()
	{
		return &ToString;
	}
};

template<typename TYPE> struct SToStringSelector<TYPE, ETypeClassification::Class>
{
	static void ToString(IString& output, const void* pInput)
	{
		output.assign("{ ");

		const CTypeInfo<TYPE>& typeInfo = GetTypeInfo<TYPE>();

		for (const SClassTypeInfoBase& base : typeInfo.GetBases())
		{
			if (base.typeInfo.GetMethods().toString)
			{
				CStackString temp;
				(*base.typeInfo.GetMethods().toString)(temp, static_cast<const uint8*>(pInput) + base.offset);
				output.append(temp.c_str());
				output.append(", ");
			}
		}

		for (const SClassTypeInfoMember& member : typeInfo.GetMembers())
		{
			if (member.typeInfo.GetMethods().toString)
			{
				CStackString temp;
				(*member.typeInfo.GetMethods().toString)(temp, static_cast<const uint8*>(pInput) + member.offset);

				if (member.typeInfo.GetFlags().Check(ETypeFlags::IsString))
				{
					output.append("'");
					output.append(temp.c_str());
					output.append("', ");
				}
				else
				{
					output.append(temp.c_str());
					output.append(", ");
				}
			}
		}

		output.TrimRight(", ");
		output.append(" }");
	}

	static inline SCommonTypeInfoMethods::ToString Select()
	{
		return &ToString;
	}
};

// #SchematycTODO : Implement default ToString function for arrays!!!

template<typename TYPE> struct SSerializeBinder // #SchematycTODO : Remove once automatic detection of serialize methods supports all cases.
{
	static bool Serialize(Serialization::IArchive& archive, void* pValue, const char* szName, const char* szLabel)
	{
		return archive(*static_cast<TYPE*>(pValue), szName, szLabel);
	};
};

template<typename TYPE, bool TYPE_IS_SERIALIZEABLE = SIsSerializeable<TYPE>::value> struct SSerializeSelector;

template<typename TYPE> struct SSerializeSelector<TYPE, true>
{
	static bool Serialize(Serialization::IArchive& archive, void* pValue, const char* szName, const char* szLabel)
	{
		return archive(*static_cast<TYPE*>(pValue), szName, szLabel);
	};

	static inline SCommonTypeInfoMethods::Serialize Select()
	{
		return &Serialize;
	}
};

template<typename TYPE> struct SSerializeSelector<TYPE, false>
{
	static inline SCommonTypeInfoMethods::Serialize Select()
	{
		return nullptr;
	}
};
} // Private

// #SchematycTODO : Do we really need to duplicate SetToStringMethod and DeclareSerializeable for each partial specialization of CTypeInfo?

template<typename TYPE> class CTypeInfo<TYPE, ETypeClassification::Fundamental> : public CCommonTypeInfo
{
public:

	inline CTypeInfo()
		: CCommonTypeInfo(ETypeClassification::Fundamental, sizeof(TYPE), GetTypeName<TYPE>())
	{}

	template<void(* TO_STRING)(IString&, const TYPE&)> inline void SetToStringMethod()
	{
		m_methods.toString = &Private::SToStringBinder<void (*)(IString&, const TYPE&), TO_STRING>::ToString;
	}

	// Explicitly declare type as serializeable. This is necessary in cases where we can't automatically detect non-intrusive serialization methods.
	// #SchematycTODO : Remove once automatic detection of serialize methods supports all cases.
	inline void DeclareSerializeable()
	{
		m_methods.serialize = &Private::SSerializeBinder<TYPE>::Serialize;
	}
};

template<typename TYPE> class CTypeInfo<TYPE, ETypeClassification::Enum> : public CEnumTypeInfo
{
public:

	inline CTypeInfo()
		: CEnumTypeInfo(sizeof(TYPE), GetTypeName<TYPE>())
	{}

	template<void(* TO_STRING)(IString&, const TYPE&)> inline void SetToStringMethod()
	{
		m_methods.toString = &Private::SToStringBinder<void (*)(IString&, const TYPE&), TO_STRING>::ToString;
	}

	// Explicitly declare type as serializeable. This is necessary in cases where we can't automatically detect non-intrusive serialization methods.
	// #SchematycTODO : Remove once automatic detection of serialize methods supports all cases.
	inline void DeclareSerializeable()
	{
		m_methods.serialize = &Private::SSerializeBinder<TYPE>::Serialize;
	}

	inline void AddConstant(const TYPE& value, const char* szName, const char* szLabel)
	{
		Serialization::EnumDescription& enumDescription = Serialization::getEnumDescription<TYPE>();
		enumDescription.add(int(value), szName, szLabel);
	}
};

template<typename TYPE> class CTypeInfo<TYPE, ETypeClassification::Class> : public CClassTypeInfo
{
public:

	inline CTypeInfo()
		: CClassTypeInfo(sizeof(TYPE), GetTypeName<TYPE>())
	{}

	template<void(TYPE::* TO_STRING)(IString&) const> inline void SetToStringMethod()
	{
		m_methods.toString = &Private::SToStringBinder<void (TYPE::*)(IString&) const, TO_STRING>::ToString;
	}

	template<void(* TO_STRING)(IString&, const TYPE&)> inline void SetToStringMethod()
	{
		m_methods.toString = &Private::SToStringBinder<void (*)(IString&, const TYPE&), TO_STRING>::ToString;
	}

	// Explicitly declare type as serializeable. This is necessary in cases where we can't automatically detect non-intrusive serialization methods.
	// #SchematycTODO : Remove once automatic detection of serialize methods supports all cases.
	inline void DeclareSerializeable()
	{
		m_methods.serialize = &Private::SSerializeBinder<TYPE>::Serialize;
	}

	template<typename BASE_TYPE> inline void AddBase()
	{
		SCHEMATYC_VERIFY_TYPE_IS_REFLECTED(BASE_TYPE);

		m_bases.push_back(SClassTypeInfoBase(GetTypeInfo<BASE_TYPE>(), GetBaseOffset<TYPE, BASE_TYPE>()));
	}

	template<typename MEMBER_TYPE> inline void AddMember(MEMBER_TYPE TYPE::* pMember, uint32 id, const char* szLabel, const char* szDescription = nullptr)
	{
		SCHEMATYC_VERIFY_TYPE_IS_REFLECTED(MEMBER_TYPE);

		m_members.push_back(SClassTypeInfoMember(GetTypeInfo<MEMBER_TYPE>(), GetMemberOffset(pMember), id, szLabel, szDescription));
	}
};

template<typename TYPE> class CTypeInfo<TYPE, ETypeClassification::Array> : public CArrayTypeInfo
{
public:

	inline CTypeInfo()
		: CArrayTypeInfo(sizeof(TYPE), GetTypeName<TYPE>(), SArrayLength<TYPE>::value)
	{}

	template<void(* TO_STRING)(IString&, const TYPE&)> inline void SetToStringMethod()
	{
		m_methods.toString = &Private::SToStringBinder<void (*)(IString&, const TYPE&), TO_STRING>::ToString;
	}

	// Explicitly declare type as serializeable. This is necessary in cases where we can't automatically detect non-intrusive serialization methods.
	// #SchematycTODO : Remove once automatic detection of serialize methods supports all cases.
	inline void DeclareSerializeable()
	{
		m_methods.serialize = &Private::SSerializeBinder<TYPE>::Serialize;
	}
};

// Get type information for a reflected type.
////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename TYPE> inline const CTypeInfo<TYPE>& GetTypeInfo();

namespace Private
{
template<typename TYPE, EReflectionMethod REFLECTION_METHOD = SReflectionMethod<TYPE>::value> struct SReflectionHelper;

template<typename TYPE> struct SReflectionHelper<TYPE, EReflectionMethod::None>
{
	static inline SGUID Reflect(CTypeInfo<TYPE>& typeInfo)
	{
		return SGUID();
	}
};

template<typename TYPE> struct SReflectionHelper<TYPE, EReflectionMethod::NonIntrusive>
{
	static inline SGUID Reflect(CTypeInfo<TYPE>& typeInfo)
	{
		return ReflectSchematycType(typeInfo);
	}
};

template<typename TYPE> struct SReflectionHelper<TYPE, EReflectionMethod::Intrusive>
{
	static inline SGUID Reflect(CTypeInfo<TYPE>& typeInfo)
	{
		return TYPE::ReflectSchematycType(typeInfo);
	}
};
} // Private

// Get type information for a reflected type.
////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename TYPE> inline const CTypeInfo<TYPE>& GetTypeInfo()
{
	// #SchematycTODO : Store type info in a global registry so that it is guaranteed to be same across all dlls? This would also mean reflection of bool, int32 etc would not need to part of the public interface.

	SCHEMATYC_VERIFY_TYPE_IS_REFLECTED(TYPE);

	static CTypeInfo<TYPE> typeInfo;

	static bool bInitTypeInfo = true;
	if (bInitTypeInfo)
	{
		SCommonTypeInfoMethods methods;
		methods.defaultConstruct = Private::SDefaultConstructSelector<TYPE>::Select();
		methods.copyConstruct = Private::SCopyConstructSelector<TYPE>::Select();
		methods.destruct = Private::SDestructSelector<TYPE>::Select();
		methods.equals = Private::SEqualsSelector<TYPE>::Select();
		methods.copyAssign = Private::SCopyAssignSelector<TYPE>::Select();
		methods.toString = Private::SToStringSelector<TYPE>::Select();
		methods.serialize = Private::SSerializeSelector<TYPE>::Select();
		typeInfo.SetMethods(methods);

		typeInfo.SetGUID(Private::SReflectionHelper<TYPE>::Reflect(typeInfo));

		bInitTypeInfo = false;
	}

	return typeInfo;
}

// Get empty type information.
////////////////////////////////////////////////////////////////////////////////////////////////////
inline const CCommonTypeInfo& GetEmptyTypeInfo()
{
	static const CCommonTypeInfo typeInfo;
	return typeInfo;
}
} // Schematyc

#define SCHEMATYC_REFLECTION_HEADER
#include "Schematyc/Reflection/ReflectionCast.h"
#include "Schematyc/Reflection/ReflectionUtils.h"
#include "Schematyc/Reflection/ReflectionTypes.h"
#undef SCHEMATYC_REFLECTION_HEADER
