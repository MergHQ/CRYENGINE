// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// Use Schematyc::GetTypeDesc<TYPE>() to get descriptor for a specific type.
// Calling Schematyc::GetTypeDesc where TYPE is not reflected will result in the following compile error: 'Type must be reflected, see TypeDesc.h for details!'
//
// In order to reflect a type you must declare a specialized ReflectType function.
// This specialization can be intrusive (a static method) or non-intrusive (a free function declared in the same namespace) and the input parameter must be a
// reference to a template specialization of the CTypeDesc class.
//
//   struct SMyStruct
//   {
//     static Schematyc::SGUID ReflectType(Schematyc::CTypeDesc<SMyStruct>& desc); // Intrusive
//   };
//
//   static inline Schematyc::SGUID ReflectType(Schematyc::CTypeDesc<SMyStruct>& desc); // Non-intrusive
//
// Specializing CTypeDesc in this way serves two purposes:
//   1) It binds the function to the type it's describing at compile time.
//   2) You can use this parameter to reflect information that cannot be detected automatically e.g.desc.AddMember() can be used to reflect class / struct members.

#pragma once

#include <CrySerialization/Enum.h>

#include "Schematyc/FundamentalTypes.h"
#include "Schematyc/Reflection/TypeOperators.h"
#include "Schematyc/Utils/EnumFlags.h"
#include "Schematyc/Utils/GUID.h"
#include "Schematyc/Utils/TypeName.h"

#define SCHEMATYC_DECLARE_STRING_TYPE(type) \
  namespace Helpers                         \
  {                                         \
  template<> struct SIsString<type>         \
  {                                         \
    static const bool value = true;         \
  };                                        \
  }

#define SCHEMATYC_DECLARE_ARRAY_TYPE(type)                                \
  namespace Helpers                                                       \
  {                                                                       \
  template<typename ELEMENT_TYPE> struct SArrayTraits<type<ELEMENT_TYPE>> \
  {                                                                       \
    typedef ELEMENT_TYPE element_type;                                    \
                                                                          \
    static const bool is_array = true;                                    \
  };                                                                      \
  }

#define SCHEMATYC_DECLARE_CUSTOM_CLASS_DESC(base, desc, interface)                                                                                    \
  namespace Helpers                                                                                                                                   \
  {                                                                                                                                                   \
  template<typename TYPE> struct SIsCustomClass<TYPE, typename std::enable_if<std::is_convertible<TYPE, base>::value>::type>                          \
  {                                                                                                                                                   \
    static const bool value = true;                                                                                                                   \
  };                                                                                                                                                  \
  }                                                                                                                                                   \
                                                                                                                                                      \
  template<typename TYPE> class CTypeDesc<TYPE, typename std::enable_if<std::is_convertible<TYPE, base>::value>::type> : public interface<TYPE, desc> \
  {                                                                                                                                                   \
    static_assert(std::is_convertible<desc, CCustomClassDesc>::value, "Descriptor must be derived from CCustomClassDesc!");                           \
  };

#define SCHEMATYC_VERIFY_TYPE_IS_REFLECTED(type) static_assert(IsReflectedType<type>(), "Type must be reflected, see TypeDesc.h for details!")

namespace Schematyc
{

template<typename TYPE, class ENABLE = void> class CTypeDesc;

template<typename TYPE> constexpr bool         IsReflectedType();
template<typename TYPE> const CTypeDesc<TYPE>& GetTypeDesc();

namespace Utils
{

struct IDefaultValue
{
	virtual ~IDefaultValue() {}

	virtual const void* Get() const = 0;
};

typedef std::unique_ptr<IDefaultValue> IDefaultValuePtr;

template<typename TYPE, bool TYPE_IS_DEFAULT_CONSTRUCTIBLE = std::is_default_constructible<TYPE>::value> class CDefaultValue;

template<typename TYPE> class CDefaultValue<TYPE, true> : public IDefaultValue
{
public:

	inline CDefaultValue()
		: m_value()
	{}

	inline CDefaultValue(const TYPE& value)
		: m_value(value)
	{}

	// IDefaultValue

	virtual const void* Get() const override
	{
		return &m_value;
	}

	// ~IDefaultValue

	static inline IDefaultValuePtr MakeUnique()
	{
		return stl::make_unique<CDefaultValue<TYPE>>();
	}

	static inline IDefaultValuePtr MakeUnique(const TYPE& value)
	{
		return stl::make_unique<CDefaultValue<TYPE>>(value);
	}

private:

	TYPE m_value;
};

template<typename TYPE> class CDefaultValue<TYPE, false> : public IDefaultValue
{
public:

	inline CDefaultValue(const TYPE& value)
		: m_value(value)
	{}

	// IDefaultValue

	virtual const void* Get() const override
	{
		return &m_value;
	}

	// ~IDefaultValue

	static inline IDefaultValuePtr MakeUnique()
	{
		return IDefaultValuePtr();
	}

	static inline IDefaultValuePtr MakeUnique(const TYPE& value)
	{
		return stl::make_unique<CDefaultValue<TYPE>>(value);
	}

private:

	TYPE m_value;
};

template<typename TYPE> void EnumToString(IString& output, const void* pInput);
template<typename TYPE> void ArrayToString(IString& output, const void* pInput);
template<typename TYPE> bool SerializeClass(Serialization::IArchive& archive, void* pValue, const char* szName, const char* szLabel);
template<typename TYPE> void ClassToString(IString& output, const void* pInput);

} // Utils

enum class ETypeCategory
{
	Unknown,
	Integral,
	FloatingPoint,
	Enum,
	String,
	Array,
	Class,
	CustomClass
};

enum class ETypeFlags
{
	Switchable = BIT(0) // #SchematycTODO : Set by default for enum types?
};

typedef CEnumFlags<ETypeFlags> TypeFlags;

// Type descriptors.

class CCommonTypeDesc
{
public:

	CCommonTypeDesc();
	CCommonTypeDesc(ETypeCategory category);

	ETypeCategory         GetCategory() const;
	void                  SetName(const CTypeName& name);
	const CTypeName&      GetName() const;
	void                  SetSize(size_t size);
	size_t                GetSize() const;
	void                  SetGUID(const SGUID& guid);
	const SGUID&          GetGUID() const;
	void                  SetLabel(const char* szLabel);
	const char*           GetLabel() const;
	void                  SetDescription(const char* szDescription);
	const char*           GetDescription() const;
	void                  SetFlags(const TypeFlags& flags);
	const TypeFlags&      GetFlags() const;
	const STypeOperators& GetOperators() const;
	const void*           GetDefaultValue() const;

	bool                  operator==(const CCommonTypeDesc& rhs) const;
	bool                  operator!=(const CCommonTypeDesc& rhs) const;

protected:

	ETypeCategory           m_category = ETypeCategory::Unknown;
	CTypeName               m_name;
	size_t                  m_size = 0;
	SGUID                   m_guid;
	const char*             m_szLabel = nullptr;
	const char*             m_szDescription = nullptr;
	TypeFlags               m_flags;
	STypeOperators          m_operators;
	Utils::IDefaultValuePtr m_pDefaultValue;
};

class CIntegralTypeDesc : public CCommonTypeDesc
{
public:

	CIntegralTypeDesc();
};

class CFloatingPointTypeDesc : public CCommonTypeDesc
{
public:

	CFloatingPointTypeDesc();
};

class CEnumTypeDesc : public CCommonTypeDesc
{
public:

	CEnumTypeDesc();
};

class CStringTypeDesc : public CCommonTypeDesc
{
public:

	CStringTypeDesc();

	const SStringOperators& GetStringOperators() const;

protected:

	SStringOperators m_stringOperators;
};

class CArrayTypeDesc : public CCommonTypeDesc
{
public:

	CArrayTypeDesc();

	const CCommonTypeDesc& GetElementTypeDesc() const;
	const SArrayOperators& GetArrayOperators() const;

protected:

	const CCommonTypeDesc* m_pElementTypeDesc = nullptr;
	SArrayOperators        m_arrayOperators;
};

class CClassBaseDesc
{
public:

	CClassBaseDesc(const CCommonTypeDesc& typeDesc, ptrdiff_t offset);

	const CCommonTypeDesc& GetTypeDesc() const;
	ptrdiff_t              GetOffset() const;

protected:

	const CCommonTypeDesc& m_typeDesc;
	ptrdiff_t              m_offset = 0;
};

typedef DynArray<CClassBaseDesc> ClassBaseDescs;

class CClassMemberDesc
{
public:

	CClassMemberDesc(const CCommonTypeDesc& typeDesc, ptrdiff_t offset, uint32 id, const char* szName, const char* szLabel, const char* szDescription, Utils::IDefaultValuePtr&& pDefaultValue);

	const CCommonTypeDesc& GetTypeDesc() const;
	ptrdiff_t              GetOffset() const;
	uint32                 GetId() const;
	const char*            GetName() const;
	const char*            GetLabel() const;
	const char*            GetDescription() const;
	const void*            GetDefaultValue() const;

private:

	const CCommonTypeDesc&  m_typeDesc;
	ptrdiff_t               m_offset = 0;
	uint32                  m_id = InvalidId;
	const char*             m_szName = nullptr;
	const char*             m_szLabel = nullptr;
	const char*             m_szDescription = nullptr;
	Utils::IDefaultValuePtr m_pDefaultValue;
};

typedef DynArray<CClassMemberDesc> ClassMemberDescs;

class CClassDesc : public CCommonTypeDesc
{
public:

	CClassDesc();

	const ClassBaseDescs&   GetBases() const;
	const CClassBaseDesc*   FindBaseByTypeDesc(const CCommonTypeDesc& typeDesc) const;

	const ClassMemberDescs& GetMembers() const;
	const CClassMemberDesc* FindMemberByOffset(ptrdiff_t offset) const;
	const CClassMemberDesc* FindMemberById(uint32 id) const;
	const CClassMemberDesc* FindMemberByName(const char* szName) const;

protected:

	CClassDesc(ETypeCategory category);

	bool AddBase(const CCommonTypeDesc& typeDesc, ptrdiff_t offset);
	bool AddMember(const CCommonTypeDesc& typeDesc, ptrdiff_t offset, uint32 id, const char* szName, const char* szLabel, const char* szDescription, Utils::IDefaultValuePtr&& pDefaultValue);

private:

	ClassBaseDescs   m_bases;
	ClassMemberDescs m_members;
};

class CCustomClassDesc : public CClassDesc
{
public:

	CCustomClassDesc();
};

// Type descriptor interfaces.

template<typename TYPE, typename DESC> class CCommonTypeDescInterface : public DESC
{
public:

	inline CCommonTypeDescInterface() {}

	CCommonTypeDescInterface(const CCommonTypeDescInterface&) = delete;
	CCommonTypeDescInterface& operator=(const CCommonTypeDescInterface&) = delete;

	inline void SetDefaultValue(const TYPE& defaultValue)
	{
		CCommonTypeDesc::m_operators.defaultConstruct = &CCommonTypeDescInterface::ConstructFromDefaultValue;

		CCommonTypeDesc::m_pDefaultValue = Utils::CDefaultValue<TYPE>::MakeUnique(defaultValue);
	}

	template<void(* FUNCTION_PTR)(IString&, const TYPE&)> inline void SetToStringOperator()
	{
		CCommonTypeDesc::m_operators.toString = Adapters::SToString<void (*)(IString&, const TYPE&), FUNCTION_PTR>::Select();
	}

protected:

	inline void Apply()
	{
		CCommonTypeDesc::m_name = GetTypeName<TYPE>();
		CCommonTypeDesc::m_size = sizeof(TYPE);

		CCommonTypeDesc::m_operators.defaultConstruct = Adapters::SDefaultConstruct<TYPE>::Select();
		CCommonTypeDesc::m_operators.destruct = Adapters::SDestruct<TYPE>::Select();
		CCommonTypeDesc::m_operators.copyConstruct = Adapters::SCopyConstruct<TYPE>::Select();
		CCommonTypeDesc::m_operators.copyAssign = Adapters::SCopyAssign<TYPE>::Select();
		CCommonTypeDesc::m_operators.equals = Adapters::SEquals<TYPE>::Select();
		CCommonTypeDesc::m_operators.serialize = Adapters::SSerialize<TYPE>::Select();

		CCommonTypeDesc::m_pDefaultValue = Utils::CDefaultValue<TYPE>::MakeUnique();
	}

private:

	static inline void* ConstructFromDefaultValue(void* pPlacement)
	{
		const TYPE* pDefaultValue = static_cast<const TYPE*>(GetTypeDesc<TYPE>().GetDefaultValue());
		return new(pPlacement) TYPE(*pDefaultValue);
	}
};

template<typename TYPE, typename DESC> class CEnumTypeDescInterface : public CCommonTypeDescInterface<TYPE, DESC>
{
public:

	typedef CCommonTypeDescInterface<TYPE, DESC> CommonTypeDescInterface;

public:

	inline void AddConstant(const TYPE& value, const char* szName, const char* szLabel)
	{
		Serialization::EnumDescription& enumDescription = Serialization::getEnumDescription<TYPE>();
		enumDescription.add(int(value), szName, szLabel);
	}

protected:

	inline void Apply()
	{
		CommonTypeDescInterface::Apply();

		CCommonTypeDesc::m_operators.toString = &Utils::EnumToString<TYPE>;
	}
};

template<typename TYPE, typename DESC> class CStringTypeDescInterface : public CCommonTypeDescInterface<TYPE, DESC>
{
public:

	template<void(* FUNCTION_PTR)(IString&, const TYPE&)> inline void SetToStringOperator()
	{
		CCommonTypeDesc::m_operators.toString = Adapters::SToString<void (*)(IString&, const TYPE&), FUNCTION_PTR>::Select();
	}

	template<void(TYPE::* FUNCTION_PTR)(IString&) const> inline void SetToStringOperator()
	{
		CCommonTypeDesc::m_operators.toString = Adapters::SToString<void (TYPE::*)(IString&) const, FUNCTION_PTR>::Select();
	}

	template<const char*(* FUNCTION_PTR)(const TYPE&)> inline void SetStringGetCharsOperator()
	{
		CStringTypeDesc::m_stringOperators.getChars = Adapters::SStringGetChars<const char*(*)(const TYPE&), FUNCTION_PTR>::Select();
	}

	template<const char*(TYPE::* FUNCTION_PTR)() const> inline void SetStringGetCharsOperator()
	{
		CStringTypeDesc::m_stringOperators.getChars = Adapters::SStringGetChars<const char*(TYPE::*)() const, FUNCTION_PTR>::Select();
	}
};

template<typename TYPE, typename ELEMENT_TYPE, typename DESC> class CArrayTypeDescInterface : public CCommonTypeDescInterface<TYPE, DESC>
{
public:

	typedef CCommonTypeDescInterface<TYPE, DESC> CommonTypeDescInterface;

public:

	template<uint32(* FUNCTION_PTR)(const TYPE&)> inline void SetArraySizeOperator()
	{
		CArrayTypeDesc::m_arrayOperators.size = Adapters::SArraySize<uint32 (*)(const TYPE&), FUNCTION_PTR>::Select();
	}

	template<uint32(TYPE::* FUNCTION_PTR)() const> inline void SetArraySizeOperator()
	{
		CArrayTypeDesc::m_arrayOperators.size = Adapters::SArraySize<uint32 (TYPE::*)() const, FUNCTION_PTR>::Select();
	}

	template<ELEMENT_TYPE& (* FUNCTION_PTR)(TYPE&, uint32)> inline void SetArrayAtOperator()
	{
		CArrayTypeDesc::m_arrayOperators.at = Adapters::SArrayAt<ELEMENT_TYPE& (*)(TYPE&, uint32), FUNCTION_PTR>::Select();
	}

	template<ELEMENT_TYPE& (TYPE::* FUNCTION_PTR)(uint32)> inline void SetArrayAtOperator()
	{
		CArrayTypeDesc::m_arrayOperators.at = Adapters::SArrayAt<ELEMENT_TYPE& (TYPE::*)(uint32), FUNCTION_PTR>::Select();
	}

	template<const ELEMENT_TYPE& (* FUNCTION_PTR)(const TYPE&, uint32)> inline void SetArrayAtConstOperator()
	{
		CArrayTypeDesc::m_arrayOperators.atConst = Adapters::SArrayAtConst<const ELEMENT_TYPE& (*)(const TYPE&, uint32), FUNCTION_PTR>::Select();
	}

	template<const ELEMENT_TYPE& (TYPE::* FUNCTION_PTR)(uint32) const> inline void SetArrayAtConstOperator()
	{
		CArrayTypeDesc::m_arrayOperators.atConst = Adapters::SArrayAtConst<const ELEMENT_TYPE& (TYPE::*)(uint32) const, FUNCTION_PTR>::Select();
	}

	template<void(* FUNCTION_PTR)(TYPE&, const ELEMENT_TYPE&)> inline void SetArrayPushBackOperator()
	{
		CArrayTypeDesc::m_arrayOperators.pushBack = Adapters::SArrayPushBack<void (*)(TYPE&, const ELEMENT_TYPE&), FUNCTION_PTR>::Select();
	}

	template<void(TYPE::* FUNCTION_PTR)(const ELEMENT_TYPE&)> inline void SetArrayPushBackOperator()
	{
		CArrayTypeDesc::m_arrayOperators.pushBack = Adapters::SArrayPushBack<void (TYPE::*)(const ELEMENT_TYPE&), FUNCTION_PTR>::Select();
	}

protected:

	inline void Apply()
	{
		CommonTypeDescInterface::Apply();

		CCommonTypeDesc::m_operators.toString = &Utils::ArrayToString<TYPE>;

		CArrayTypeDesc::m_pElementTypeDesc = &GetTypeDesc<ELEMENT_TYPE>();
	}
};

template<typename TYPE, typename DESC> class CClassDescInterface : public CCommonTypeDescInterface<TYPE, DESC>
{
public:

	typedef CCommonTypeDescInterface<TYPE, DESC> CommonTypeDescInterface;

public:

	template<void(* FUNCTION_PTR)(IString&, const TYPE&)> inline void SetToStringOperator()
	{
		CCommonTypeDesc::m_operators.toString = Adapters::SToString<void (*)(IString&, const TYPE&), FUNCTION_PTR>::Select();
	}

	template<void(TYPE::* FUNCTION_PTR)(IString&) const> inline void SetToStringOperator()
	{
		CCommonTypeDesc::m_operators.toString = Adapters::SToString<void (TYPE::*)(IString&) const, FUNCTION_PTR>::Select();
	}

	template<typename BASE_TYPE> inline bool AddBase()
	{
		SCHEMATYC_VERIFY_TYPE_IS_REFLECTED(BASE_TYPE);

		return CClassDesc::AddBase(GetTypeDesc<BASE_TYPE>(), GetBaseOffset<TYPE, BASE_TYPE>());
	}

	template<typename MEMBER_TYPE> inline bool AddMember(MEMBER_TYPE TYPE::* pMember, uint32 id, const char* szName, const char* szLabel, const char* szDescription)
	{
		SCHEMATYC_VERIFY_TYPE_IS_REFLECTED(MEMBER_TYPE);

		return CClassDesc::AddMember(GetTypeDesc<MEMBER_TYPE>(), GetMemberOffset(pMember), id, szName, szLabel, szDescription, Utils::IDefaultValuePtr());
	}

	template<typename MEMBER_TYPE> inline bool AddMember(MEMBER_TYPE TYPE::* pMember, uint32 id, const char* szName, const char* szLabel, const char* szDescription, const MEMBER_TYPE& defaultValue)
	{
		SCHEMATYC_VERIFY_TYPE_IS_REFLECTED(MEMBER_TYPE);

		return CClassDesc::AddMember(GetTypeDesc<MEMBER_TYPE>(), GetMemberOffset(pMember), id, szName, szLabel, szDescription, stl::make_unique<Utils::CDefaultValue<MEMBER_TYPE>>(defaultValue));
	}

protected:

	inline void Apply()
	{
		CommonTypeDescInterface::Apply();

		if (!CCommonTypeDesc::m_operators.serialize) // #SchematycTODO : Can we make this decision at compile time?
		{
			CCommonTypeDesc::m_operators.serialize = &Utils::SerializeClass<TYPE>;
		}

		if (!CCommonTypeDesc::m_operators.toString) // #SchematycTODO : Can we make this decision at compile time?
		{
			CCommonTypeDesc::m_operators.toString = &Utils::ClassToString<TYPE>;
		}
	}
};

// Type descriptor specializations.

namespace Helpers
{

template<typename TYPE> struct SIsString
{
	static const bool value = false;
};

template<typename TYPE> struct SArrayTraits
{
	typedef void element_type;

	static const bool is_array = false;
};

template<typename TYPE, typename ENABLE = void> struct SIsCustomClass
{
	static const bool value = false;
};

template<typename TYPE> struct SIsGenericClass
{
	static const bool value = std::is_class<TYPE>::value && !SIsString<TYPE>::value && !SArrayTraits<TYPE>::is_array && !SIsCustomClass<TYPE>::value;
};

} // Helpers

template<typename TYPE> class CTypeDesc<TYPE, typename std::enable_if<std::is_integral<TYPE>::value>::type> : public CCommonTypeDescInterface<TYPE, CIntegralTypeDesc>
{
};

template<typename TYPE> class CTypeDesc<TYPE, typename std::enable_if<std::is_floating_point<TYPE>::value>::type> : public CCommonTypeDescInterface<TYPE, CFloatingPointTypeDesc>
{
};

template<typename TYPE> class CTypeDesc<TYPE, typename std::enable_if<std::is_enum<TYPE>::value>::type> : public CEnumTypeDescInterface<TYPE, CEnumTypeDesc>
{
};

template<typename TYPE> class CTypeDesc<TYPE, typename std::enable_if<Helpers::SIsString<TYPE>::value>::type> : public CStringTypeDescInterface<TYPE, CStringTypeDesc>
{
};

template<typename TYPE> class CTypeDesc<TYPE, typename std::enable_if<Helpers::SArrayTraits<TYPE>::is_array>::type> : public CArrayTypeDescInterface<TYPE, typename Helpers::SArrayTraits<TYPE>::element_type, CArrayTypeDesc>
{
};

template<typename TYPE> class CTypeDesc<TYPE, typename std::enable_if<Helpers::SIsGenericClass<TYPE>::value>::type> : public CClassDescInterface<TYPE, CClassDesc>
{
};

} // Schematyc

#include "Schematyc/Reflection/TypeDesc.inl"
#include "Schematyc/Reflection/DefaultTypeReflection.inl"
