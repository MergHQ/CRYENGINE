// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryType/TypeUtils.h>
#include <CryType/TypeOperators.h>
#include <CryType/TypeFunctions.h>
#include <CryType/TypeTraits.h>

// TODO: Move with CIdList
#include <initializer_list>
// ~TODO

namespace Cry {
namespace Type {

class CTypeId
{
public:
	typedef uint32 ValueType;
	static constexpr ValueType Invalid = 0;

	template<typename TYPE>
	constexpr static CTypeId Create()
	{
		return CTypeId(Utils::SExplicit<typename PureType<TYPE>::Type>());
	}

	constexpr CTypeId() = default;

	constexpr bool IsValid() const
	{
		return m_typeNameCrc != Invalid;
	}

	constexpr static CTypeId Empty()
	{
		return CTypeId(Invalid);
	}

	constexpr ValueType GetValue() const
	{
		return m_typeNameCrc;
	}

	constexpr bool operator==(CTypeId rhs) const
	{
		return m_typeNameCrc == rhs.m_typeNameCrc;
	}

	constexpr bool operator!=(CTypeId rhs) const
	{
		return m_typeNameCrc != rhs.m_typeNameCrc;
	}

	constexpr operator ValueType() const
	{
		return m_typeNameCrc;
	}

protected:
	friend class CTypeDesc;

	template<typename TYPE>
	friend constexpr inline CTypeId IdOf();

	constexpr CTypeId(ValueType typeNameCrc)
		: m_typeNameCrc(typeNameCrc)
	{}

	template<typename TYPE>
	constexpr CTypeId(Utils::SExplicit<TYPE> )
		: m_typeNameCrc(Utils::SCompileTime_TypeInfo<TYPE>::GetCrc32())
	{}

private:
	ValueType m_typeNameCrc = 0;
};

// TODO: Move to a separate file.
class CTypeDesc
{
public:
	template<typename TYPE>
	static CTypeDesc Create()
	{
		return CTypeDesc(Utils::SExplicit<typename PureType<TYPE>::Type>(), 0);
	}

	template<typename TYPE, size_t ARRAY_LENGTH>
	static CTypeDesc Create()
	{
		return CTypeDesc(Utils::SExplicit<typename PureType<TYPE>::Type>(), ARRAY_LENGTH);
	}

	CTypeDesc()
		: m_typeId()
		, m_arrayLength(0)
		, m_size(0)
		, m_alignment(0)
		, m_isAbstract(false)
		, m_isArray(false)
		, m_isClass(false)
		, m_isEnum(false)
		, m_isFundamental(false)
		, m_isUnion(false)
	{}

	CTypeDesc(const CTypeDesc& desc)
		: m_fullQualifiedName(desc.m_fullQualifiedName)
		, m_typeId(desc.m_typeId)
		, m_arrayLength(desc.m_arrayLength)
		, m_size(desc.m_size)
		, m_alignment(desc.m_alignment)
		, m_isAbstract(desc.m_isAbstract)
		, m_isArray(desc.m_isArray)
		, m_isClass(desc.m_isClass)
		, m_isEnum(desc.m_isEnum)
		, m_isFundamental(desc.m_isFundamental)
		, m_isUnion(desc.m_isUnion)
		, m_constructorDefault(desc.m_constructorDefault)
		, m_constructorCopy(desc.m_constructorCopy)
		, m_constructorMove(desc.m_constructorMove)
		, m_destructor(desc.m_destructor)
		, m_operatorDefaultNew(desc.m_operatorDefaultNew)
		, m_operatorDelete(desc.m_operatorDelete)
		, m_operatorNewArray(desc.m_operatorNewArray)
		, m_operatorDeleteArray(desc.m_operatorDeleteArray)
		, m_operatorCopyAssign(desc.m_operatorCopyAssign)
		, m_operatorMoveAssign(desc.m_operatorMoveAssign)
		, m_operatorEqual(desc.m_operatorEqual)
		, m_functionSerialize(desc.m_functionSerialize)
		, m_functionToString(desc.m_functionToString)
	{}

	~CTypeDesc()
	{

	}

	CTypeId              GetTypeId() const              { return m_typeId; }
	const char*          GetRawName() const             { return m_fullQualifiedName.c_str(); }
	uint16               GetSize() const                { return m_size; }
	uint16               GetAlignment() const           { return m_alignment; }

	bool                 IsAbstract() const             { return m_isAbstract; }
	bool                 IsArray() const                { return m_isArray; }
	bool                 IsClass() const                { return m_isClass; }
	bool                 IsEnum() const                 { return m_isEnum; }
	bool                 IsFundamental() const          { return m_isFundamental; }
	bool                 IsUnion() const                { return m_isUnion; }

	CDefaultConstructor  GetDefaultConstructor() const  { return m_constructorDefault; }
	CCopyConstructor     GetCopyConstructor() const     { return m_constructorCopy; }
	CMoveConstructor     GetMoveConstructor() const     { return m_constructorMove; }

	CDestructor          GetDestructor() const          { return m_destructor; }

	CDefaultNewOperator  GetDefaultNewOperator() const  { return m_operatorDefaultNew; }
	CDeleteOperator      GetDeleteOperator() const      { return m_operatorDelete; }

	CNewArrayOperator    GetNewArrayOperator() const    { return m_operatorNewArray; }
	CDeleteArrayOperator GetDeleteArrayOperator() const { return m_operatorDeleteArray; }

	CCopyAssignOperator  GetCopyAssignOperator() const  { return m_operatorCopyAssign; }
	CMoveAssignOperator  GetMoveAssignOperator() const  { return m_operatorMoveAssign; }
	CEqualOperator       GetEqualOperator() const       { return m_operatorEqual; }

	CSerializeFunction   GetSerializeFunction() const   { return m_functionSerialize; }
	CToStringFunction    GetToStringFunction() const    { return m_functionToString; }

private:
	template<typename TYPE>
	CTypeDesc(Utils::SExplicit<TYPE>, size_t arrayLength)
		: m_fullQualifiedName(Utils::SCompileTime_TypeInfo<TYPE>::GetName().GetBegin(), Utils::SCompileTime_TypeInfo<TYPE>::GetName().GetLength())
		, m_typeId(CTypeId::Create<TYPE>())
		, m_arrayLength(arrayLength)
		, m_size(sizeof(TYPE))
		, m_alignment(AlignOf<TYPE>::Value)
		, m_isAbstract(std::is_abstract<TYPE>::value)
		, m_isArray(arrayLength != 0)
		, m_isClass(std::is_class<TYPE>::value)
		, m_isEnum(std::is_enum<TYPE>::value)
		, m_isFundamental(std::is_fundamental<TYPE>::value)
		, m_isUnion(std::is_union<TYPE>::value)
		, m_constructorDefault(CAdapater_DefaultConstructor<TYPE>())
		, m_constructorCopy(CAdapater_CopyConstructor<TYPE>())
		, m_constructorMove(CAdapter_MoveConstructor<TYPE>())
		, m_destructor(CAdapter_Destructor<TYPE>())
		, m_operatorDefaultNew(CAdapter_DefaultNew<TYPE>())
		, m_operatorDelete(CAdapter_Delete<TYPE>())
		, m_operatorNewArray(CAdapter_NewArray<TYPE>())
		, m_operatorDeleteArray(CAdapter_DeleteArray<TYPE>())
		, m_operatorCopyAssign(CAdapter_CopyAssign<TYPE>())
		, m_operatorMoveAssign(CAdapter_MoveAssign<TYPE>())
		, m_operatorEqual(CAdapter_Equal<TYPE>())
		, m_functionSerialize(CAdapter_Serialize<TYPE>())
		, m_functionToString(CAdapter_ToString<TYPE>())
	{
		static_assert(sizeof(TYPE) <= UINT32_MAX, "Size of type exceeds limit of uint32.");
		static_assert(AlignOf<TYPE>::Value <= UINT16_MAX, "Alignment requirement of type exceeds limit of uint16.");
	}

private:
	string               m_fullQualifiedName;
	CTypeId              m_typeId;
	uint32               m_arrayLength;
	uint32               m_size;
	uint16               m_alignment;

	bool                 m_isAbstract;
	bool                 m_isArray;
	bool                 m_isClass;
	bool                 m_isEnum;
	bool                 m_isFundamental;
	bool                 m_isUnion;

	CDefaultConstructor  m_constructorDefault;
	CCopyConstructor     m_constructorCopy;
	CMoveConstructor     m_constructorMove;

	CDestructor          m_destructor;

	CDefaultNewOperator  m_operatorDefaultNew;
	CDeleteOperator      m_operatorDelete;

	CNewArrayOperator    m_operatorNewArray;
	CDeleteArrayOperator m_operatorDeleteArray;

	CCopyAssignOperator  m_operatorCopyAssign;
	CMoveAssignOperator  m_operatorMoveAssign;
	CEqualOperator       m_operatorEqual;

	CSerializeFunction   m_functionSerialize;
	CToStringFunction    m_functionToString;
};
// ~TODO

inline bool operator==(const CTypeDesc& lh, CTypeId rh)
{
	return lh.GetTypeId() == rh;
}

inline bool operator!=(const CTypeDesc& lh, CTypeId rh)
{
	return lh.GetTypeId() != rh;
}

inline bool operator==(CTypeId lh, const CTypeDesc& rh)
{
	return lh == rh.GetTypeId();
}

inline bool operator!=(CTypeId lh, const CTypeDesc& rh)
{
	return lh != rh.GetTypeId();
}

template<typename TYPE>
constexpr CTypeId IdOf()
{
	typedef typename PureType<TYPE>::Type T;
	return CTypeId(Utils::SCompileTime_TypeInfo<T>::GetCrc32());
}

template<typename TYPE>
static const CTypeDesc& DescOf()
{
	static const CTypeDesc typeDesc = CTypeDesc::Create<TYPE>();
	return typeDesc;
}

// TODO: Move to a separate file.
class CIdList
{
public:
	CIdList(std::initializer_list<CTypeId> typeIds)
		: m_typeIds(typeIds)
	{}

	size_t  GetIdCount() const { return m_typeIds.size(); }
	CTypeId GetIdByIndex(size_t index) const
	{
		if (index < m_typeIds.size())
		{
			return m_typeIds[index];
		}
		return CTypeId();
	}

private:
	std::vector<CTypeId> m_typeIds;
};
// ~TODO

} // ~Type namespace
} // ~Cry namespace

namespace Cry {

using CTypeId = Type::CTypeId;
using CTypeDesc = Type::CTypeDesc;

}
