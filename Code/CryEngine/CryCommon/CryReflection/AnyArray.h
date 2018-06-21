// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/Framework/Common.h>

namespace Cry {
namespace Reflection {

class CAny;
struct ITypeDesc;

class CAnyArray
{
public:
	CAnyArray();
	~CAnyArray();

	CAnyArray(const CAny& any);
	CAnyArray(TypeIndex typeIndex, bool isPointer = false);
	CAnyArray(const ITypeDesc& typeDesc, bool isPointer = false);

	void             SetType(TypeIndex typeIndex, bool isPointer = false);
	void             SetType(const ITypeDesc& typeDesc, bool isPointer = false);

	void             SetConst(bool isConst) { m_isConst = isConst; }
	bool             IsConst() const        { return m_isConst; }
	bool             IsPointer() const      { return m_isPointer; }

	uint32           GetSize() const        { return m_size; }
	uint32           GetCapacity() const    { return m_capacity; }
	TypeIndex        GetTypeIndex() const   { return m_typeIndex; }
	const ITypeDesc* GetTypeDesc() const;

	bool             IsEmpty() const { return m_size == 0; }

	void             Resize(uint32 size);
	void             Reserve(uint32 capacity);

	template<typename VALUE_TYPE>
	void AddValue(const VALUE_TYPE& source);
	void AddValue(const CAny& source);
	void AddValue(const void* pSource, TypeIndex typeIndex);
	void AddValue(const void* pSource, const ITypeDesc& typeDesc);

	template<typename VALUE_TYPE>
	void AddPointer(VALUE_TYPE* pPointer);
	void AddPointer(const CAny& source);
	void AddPointer(const void* pPointer, TypeIndex typeIndex);
	void AddPointer(const void* pPointer, const ITypeDesc& typeDesc);

	template<typename VALUE_TYPE>
	void InsertValueAt(uint32 index, const VALUE_TYPE& source);
	void InsertValueAt(uint32 index, CAny& source);
	void InsertValueAt(uint32 index, const void* pSource, TypeIndex typeIndex);
	void InsertValueAt(uint32 index, const void* pSource, const ITypeDesc& typeDesc);

	template<typename VALUE_TYPE>
	void InsertPointerAt(uint32 index, const VALUE_TYPE* pPointer);
	void InsertPointerAt(uint32 index, const CAny& source);
	void InsertPointerAt(uint32 index, const void* pPointer, TypeIndex typeIndex);
	void InsertPointerAt(uint32 index, const void* pPointer, const ITypeDesc& typeDesc);

	template<typename VALUE_TYPE>
	void AssignCopyValueAt(uint32 index, const VALUE_TYPE& source);
	void AssignCopyValueAt(uint32 index, const CAny& source);
	void AssignCopyValueAt(uint32 index, const void* pSource, TypeIndex typeIndex);
	void AssignCopyValueAt(uint32 index, const void* pSource, const ITypeDesc& typeDesc);

	template<typename VALUE_TYPE>
	void AssignMoveValueAt(uint32 index, VALUE_TYPE& source);
	void AssignMoveValueAt(uint32 index, CAny& source);
	void AssignMoveValueAt(uint32 index, void* pSource, TypeIndex typeindex);
	void AssignMoveValueAt(uint32 index, void* pSource, const ITypeDesc& typeDesc);

	template<typename VALUE_TYPE>
	void AssignPointerAt(uint32 index, const VALUE_TYPE* pPointer);
	void AssignPointerAt(uint32 index, const CAny& source);
	void AssignPointerAt(uint32 index, const void* pPointer, TypeIndex typeIndex);
	void AssignPointerAt(uint32 index, const void* pPointer, const ITypeDesc& typeDesc);

	template<typename VALUE_TYPE>
	VALUE_TYPE* GetPointer(uint32 index);
	void*       GetPointer(uint32 index);

	template<typename VALUE_TYPE>
	const VALUE_TYPE* GetConstPointer(uint32 index) const;
	const void*       GetConstPointer(uint32 index) const;

	// Clear & Remove
	void   Remove(uint32 index);

	void   ClearAll();
	void   DestructAll();

	bool   Serialize(Serialization::IArchive& archive);
	string ToString() const;

	void*  GetBuffer() const;

private:
	size_t GetAlignedSize() const;
	void*  GetPointerInternal(uint32 index) const;

	void   CallDestructors(uint32 begin, uint32 end);
	void   CallDefaultConstructors(uint32 begin, uint32 end);

	void   AddValueInternal(const void* pValue, const ITypeDesc& typeDesc);
	void   AddPointerInternal(const void* pPointer, const ITypeDesc& typeDesc);
	void   AssignPointerInternal(uint32 index, const void* pPointer, const ITypeDesc& typeDesc);

	void   ShiftLeftInternal(uint32 index);
	void   ShiftRightInternal(uint32 index);

private:
	unsigned char* m_pData = nullptr;

	uint32         m_size = 0;
	uint32         m_capacity = 0;
	TypeIndex      m_typeIndex = TypeIndex::Invalid;

	bool           m_isPointer : 1; // True if values in array are pointers.
	bool           m_isConst   : 1; // True if values in array are const.
	// 24 bytes
};

static_assert(sizeof(CAnyArray) <= 24, "Size of CAnyArray should be <= 24 bytes.");

} // ~Reflection namespace

using CAnyArray = Reflection::CAnyArray;

} // ~Cry namespace

#include "AnyArray_impl.inl"
