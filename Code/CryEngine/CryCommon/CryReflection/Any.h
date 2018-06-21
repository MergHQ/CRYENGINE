// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/Framework/Common.h>

namespace Cry {
namespace Reflection {

struct ITypeDesc;

class CAny
{
public:
	CAny();
	CAny(const CAny& other);
	CAny(const ITypeDesc& typeDesc);

	template<typename VALUE_TYPE>
	CAny(const VALUE_TYPE& source);

	template<typename VALUE_TYPE>
	CAny(VALUE_TYPE* pSource);

	~CAny();

	const ITypeDesc* GetTypeDesc() const;
	TypeIndex        GetTypeIndex() const;

	void             CreateDefaultValue(const ITypeDesc& typeDesc);

	template<typename VALUE_TYPE>
	void AssignValue(const VALUE_TYPE& source, bool isConst = false);
	template<typename VALUE_TYPE>
	void AssignValue(const VALUE_TYPE& source, TypeIndex typeIndex, bool isConst = false);
	template<typename VALUE_TYPE>
	void AssignValue(const VALUE_TYPE& source, const ITypeDesc& srcTypeDesc, bool isConst = false);
	void AssignValue(const CAny& source, bool isConst = false);
	void AssignValue(const void* pSource, TypeIndex typeIndex, bool isConst = false);
	void AssignValue(const void* pSource, const ITypeDesc& srcTypeDesc, bool isConst = false);

	template<typename VALUE_TYPE>
	void AssignPointer(VALUE_TYPE* pPointer);
	template<typename VALUE_TYPE>
	void AssignPointer(VALUE_TYPE* pPointer, TypeIndex typeIndex);
	template<typename VALUE_TYPE>
	void AssignPointer(VALUE_TYPE* pPointer, const ITypeDesc& srcTypeDesc);
	void AssignPointer(void* pPointer, TypeIndex typeIndex);
	void AssignPointer(void* pPointer, const ITypeDesc& srcTypeDesc);
	void AssignPointer(const void* pPointer, TypeIndex typeIndex);
	void AssignPointer(const void* pPointer, const ITypeDesc& srcTypeDesc);

	template<typename VALUE_TYPE>
	bool CopyValue(VALUE_TYPE& target) const;
	bool CopyValue(void* pTarget, TypeIndex typeIndex) const;
	bool CopyValue(void* pTarget, const ITypeDesc& dstTypeDesc) const;

	template<typename VALUE_TYPE>
	void AssignToPointer(const VALUE_TYPE& source);
	template<typename VALUE_TYPE>
	void AssignToPointer(const VALUE_TYPE& source, TypeIndex typeIndex);
	template<typename VALUE_TYPE>
	void AssignToPointer(const VALUE_TYPE& source, const ITypeDesc& srcTypeDesc);
	void AssignToPointer(void* pSource, TypeIndex typeIndex);
	void AssignToPointer(void* pSource, const ITypeDesc& srcTypeDesc);
	void AssignToPointer(const void* pSource, TypeIndex typeIndex);
	void AssignToPointer(const void* pSource, const ITypeDesc& srcTypeDesc);

	template<typename VALUE_TYPE>
	VALUE_TYPE* GetPointer();
	void*       GetPointer();

	template<typename VALUE_TYPE>
	const VALUE_TYPE* GetConstPointer() const;
	const void*       GetConstPointer() const;

	template<typename VALUE_TYPE>
	bool   IsEqual(const VALUE_TYPE& other) const;
	bool   IsEqual(const CAny& other) const;
	bool   IsEqualPointer(const void* pOther) const;

	bool   IsEmpty() const               { return (m_typeIndex.IsValid() == false); }
	bool   IsPointer() const         { return m_isPointer; }

	bool   IsConst() const               { return m_isConst; }
	void   SetConst(bool isConst)        { m_isConst = isConst; }

	bool   IsType(TypeIndex index) const { return (m_typeIndex == index); }
	bool   IsType(const ITypeDesc& typeDesc) const;

	void   Destruct();

	bool   Serialize(Serialization::IArchive& archive);
	string ToString() const;

	// Operators
	CAny& operator=(const CAny& other);
	CAny& operator=(CAny&& other);

	// Size of internal buffer.
	static constexpr size_t GetFixedBufferSize() { return sizeof(decltype(m_data)); }

private:
	union
	{
		void*  m_pData;
		uint64 m_data = 0;
	};
	// 8 bytes

	TypeIndex m_typeIndex = TypeIndex::Invalid;
	bool      m_isSmallValue : 1; // True if value is small enough to fit into m_data.
	bool      m_isPointer    : 1; // True if value is a pointer (uses m_pData).
	bool      m_isConst      : 1; // True if value or pointer is const.
	// 16 bytes
};

static_assert(sizeof(CAny) <= 16, "Size of CAny should be <= 16 bytes.");

} // ~Reflection namespace

using CAny = Reflection::CAny;

} // ~Cry namespace

#include "Any_impl.inl"
