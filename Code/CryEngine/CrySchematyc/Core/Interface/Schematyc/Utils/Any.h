// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// #SchematycTODO : Create cast functions for CAnyValue&, CAnyValue* and CAnyValuePtr?
// #SchematycTODO : Create typed version of CAnyValue for faster construction (avoid type info lookup)?

#pragma once

#include <CrySerialization/Forward.h>

#include "Schematyc/Reflection/Reflection.h"
#include "Schematyc/Utils/Assert.h"

namespace Schematyc
{
// Forward declare classes.
class CAnyValue;
class CAnyRef;
class CAnyConstRef;
class CAnyPtr;
class CAnyConstPtr;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(CAnyValue)

// Specialized allocator for CAnyValue, allocates additional memory for value storage immediately after object.
template<typename T> struct SAnyValueAllocator
{
	typedef T        value_type;
	typedef T*       pointer;
	typedef const T& const_pointer;
	typedef T&       reference;
	typedef const T& const_reference;
	typedef size_t   size_type;

	template<class U> struct rebind
	{
		typedef SAnyValueAllocator<U> other;
	};

	inline SAnyValueAllocator(const CCommonTypeInfo& _typeInfo)
		: typeInfo(_typeInfo)
	{}

	template<typename OTHER_TYPE> inline SAnyValueAllocator(const SAnyValueAllocator<OTHER_TYPE>& rhs)
		: typeInfo(rhs.typeInfo)
	{}

	pointer address(reference x)
	{
		return &x;
	}

	const_pointer address(const_reference x) const
	{
		return &x;
	}

	inline pointer allocate(size_type n)
	{
		return static_cast<pointer>(CryModuleMalloc((sizeof(value_type) + typeInfo.GetSize()) * n));
	}

	inline void deallocate(pointer p, size_type n)
	{
		CryModuleFree(p);
	}

	size_type max_size() const
	{
		return INT_MAX;
	}

	template<class U>
	void destroy(U* p) noexcept
	{
		p->~U();
	}

	const CCommonTypeInfo& typeInfo;
};

// Owning container for value of any reflected type. It is assumed that value follows immediately after object in memory therefore instances must be allocated using Make functions provided.
class CAnyValue
{
	CAnyValue() = delete;
	CAnyValue(const CAnyValue& rhs) = delete;
	CAnyValue& operator=(const CAnyValue& rhs) = delete;

public:

	~CAnyValue();

	const CCommonTypeInfo&                      GetTypeInfo() const;
	void*                                       GetValue();
	const void*                                 GetValue() const;

	static CAnyValuePtr                         MakeShared(const CCommonTypeInfo& typeInfo, const void* pValue);
	template<typename TYPE> static CAnyValuePtr MakeShared(const TYPE& value);
	static CAnyValuePtr                         CloneShared(const CAnyConstRef& value);

protected:

	CAnyValue(const CCommonTypeInfo& typeInfo, const void* pValue);

private:

	const CCommonTypeInfo& m_typeInfo;
};

// Reference to value of any reflected type.
class CAnyRef
{
	friend class CAnyPtr;

	CAnyRef& operator=(const CAnyRef& rhs) = delete;

public:

	CAnyRef(const CCommonTypeInfo& typeInfo, void* pValue);
	template<typename TYPE> CAnyRef(TYPE& value);
	CAnyRef(CAnyValue& rhs);
	CAnyRef(const CAnyRef& rhs);

	const CCommonTypeInfo& GetTypeInfo() const;
	void*                  GetValue() const;

protected:

	CAnyRef();

	bool IsValid() const;
	void Reset(const CAnyRef& rhs);

private:

	const CCommonTypeInfo* m_pTypeInfo;
	void*                  m_pValue;
};

// Reference to constant value of any reflected type.
class CAnyConstRef
{
	friend class CAnyConstPtr;

	CAnyConstRef& operator=(const CAnyConstRef& rhs) = delete;

public:

	CAnyConstRef(const CCommonTypeInfo& typeInfo, const void* pValue);
	template<typename TYPE> CAnyConstRef(const TYPE& value);
	CAnyConstRef(const CAnyValue& rhs);
	CAnyConstRef(const CAnyRef& rhs);
	CAnyConstRef(const CAnyConstRef& rhs);

	const CCommonTypeInfo& GetTypeInfo() const;
	const void*            GetValue() const;

protected:

	CAnyConstRef();

	bool IsValid() const;
	void Reset(const CAnyConstRef& rhs);

private:

	const CCommonTypeInfo* m_pTypeInfo;
	const void*            m_pValue;
};

// Pointer to value of any reflected type.
class CAnyPtr
{
public:

	CAnyPtr();
	CAnyPtr(const CCommonTypeInfo& typeInfo, void* pValue);
	template<typename TYPE> CAnyPtr(TYPE* pValue);
	CAnyPtr(CAnyValue* pValue);
	CAnyPtr(const CAnyValuePtr& pValue);
	CAnyPtr(const CAnyRef* rhs);
	CAnyPtr(const CAnyPtr& rhs);

	explicit       operator bool() const;
	const CAnyRef* operator->() const;
	const CAnyRef& operator*() const;
	CAnyPtr&       operator=(const CAnyPtr& rhs);

private:

	CAnyRef m_ref;
};

// Pointer to constant value of any reflected type.
class CAnyConstPtr
{
public:

	CAnyConstPtr();
	CAnyConstPtr(const CCommonTypeInfo& typeInfo, const void* pValue);
	template<typename TYPE> CAnyConstPtr(const TYPE* pValue);
	CAnyConstPtr(const CAnyValue* pValue);
	CAnyConstPtr(const CAnyValuePtr& pValue);
	CAnyConstPtr(const CAnyValueConstPtr& pValue);
	CAnyConstPtr(const CAnyConstRef* rhs);
	CAnyConstPtr(const CAnyPtr& rhs);
	CAnyConstPtr(const CAnyConstPtr& rhs);

	explicit            operator bool() const;
	const CAnyConstRef* operator->() const;
	const CAnyConstRef& operator*() const;
	CAnyConstPtr&       operator=(const CAnyConstPtr& rhs);

private:

	CAnyConstRef m_ref;
};

// CAnyValue functions.

inline CAnyValue::~CAnyValue()
{
	m_typeInfo.GetMethods().destruct(GetValue());
}

inline const CCommonTypeInfo& CAnyValue::GetTypeInfo() const
{
	return m_typeInfo;
}

inline void* CAnyValue::GetValue()
{
	return this + 1;
}

inline const void* CAnyValue::GetValue() const
{
	return this + 1;
}

inline CAnyValuePtr CAnyValue::MakeShared(const CCommonTypeInfo& typeInfo, const void* pValue)
{
	class CAnyValueImpl : public CAnyValue
	{
	public:

		inline CAnyValueImpl(const CCommonTypeInfo& typeInfo, const void* pValue)
			: CAnyValue(typeInfo, pValue)
		{}
	};

	SCHEMATYC_CORE_ASSERT_MESSAGE(typeInfo.GetMethods().copyConstruct, "Type must provide copy constructor!");
	if (typeInfo.GetMethods().copyConstruct)
	{
		SCHEMATYC_CORE_ASSERT(pValue);
		if (pValue)
		{
			SAnyValueAllocator<CAnyValueImpl> allocator(typeInfo);
			return std::static_pointer_cast<CAnyValue>(std::allocate_shared<CAnyValueImpl, SAnyValueAllocator<CAnyValueImpl>>(allocator, typeInfo, pValue));
		}
	}
	return CAnyValuePtr();
}

template<typename TYPE> inline CAnyValuePtr CAnyValue::MakeShared(const TYPE& value)
{
	SCHEMATYC_VERIFY_TYPE_IS_REFLECTED(TYPE);
	static_assert(std::is_copy_constructible<TYPE>::value, "Type must provide copy constructor!");
	return MakeShared(Schematyc::GetTypeInfo<TYPE>(), &value);
}

inline CAnyValuePtr CAnyValue::CloneShared(const CAnyConstRef& value)
{
	return MakeShared(value.GetTypeInfo(), value.GetValue());
}

inline CAnyValue::CAnyValue(const CCommonTypeInfo& typeInfo, const void* pValue)
	: m_typeInfo(typeInfo)
{
	m_typeInfo.GetMethods().copyConstruct(GetValue(), pValue);
}

// CAnyRef functions.

inline CAnyRef::CAnyRef(const CCommonTypeInfo& typeInfo, void* pValue)
	: m_pTypeInfo(&typeInfo)
	, m_pValue(pValue)
{
	SCHEMATYC_CORE_ASSERT(IsValid());
}

template<typename TYPE> inline CAnyRef::CAnyRef(TYPE& value)
	: m_pTypeInfo(&Schematyc::GetTypeInfo<TYPE>())
	, m_pValue(&value)
{}

inline CAnyRef::CAnyRef(CAnyValue& rhs)
	: m_pTypeInfo(&rhs.GetTypeInfo())
	, m_pValue(rhs.GetValue())
{}

inline CAnyRef::CAnyRef(const CAnyRef& rhs)
	: m_pTypeInfo(rhs.m_pTypeInfo)
	, m_pValue(rhs.m_pValue)
{}

inline const CCommonTypeInfo& CAnyRef::GetTypeInfo() const
{
	SCHEMATYC_CORE_ASSERT(IsValid());
	return *m_pTypeInfo;
}

inline void* CAnyRef::GetValue() const
{
	SCHEMATYC_CORE_ASSERT(IsValid());
	return m_pValue;
}

inline CAnyRef::CAnyRef()
	: m_pTypeInfo(nullptr)
	, m_pValue(nullptr)
{}

inline bool CAnyRef::IsValid() const
{
	return m_pValue != nullptr;
}

inline void CAnyRef::Reset(const CAnyRef& rhs)
{
	m_pTypeInfo = rhs.m_pTypeInfo;
	m_pValue = rhs.m_pValue;
}

// CAnyConstRef functions.

inline CAnyConstRef::CAnyConstRef(const CCommonTypeInfo& typeInfo, const void* pValue)
	: m_pTypeInfo(&typeInfo)
	, m_pValue(pValue)
{
	SCHEMATYC_CORE_ASSERT(IsValid());
}

template<typename TYPE> inline CAnyConstRef::CAnyConstRef(const TYPE& value)
	: m_pTypeInfo(&Schematyc::GetTypeInfo<TYPE>())
	, m_pValue(&value)
{}

inline CAnyConstRef::CAnyConstRef(const CAnyValue& rhs)
	: m_pTypeInfo(&rhs.GetTypeInfo())
	, m_pValue(rhs.GetValue())
{}

inline CAnyConstRef::CAnyConstRef(const CAnyRef& rhs)
	: m_pTypeInfo(&rhs.GetTypeInfo())
	, m_pValue(rhs.GetValue())
{}

inline CAnyConstRef::CAnyConstRef(const CAnyConstRef& rhs)
	: m_pTypeInfo(rhs.m_pTypeInfo)
	, m_pValue(rhs.m_pValue)
{}

inline const CCommonTypeInfo& CAnyConstRef::GetTypeInfo() const
{
	SCHEMATYC_CORE_ASSERT(IsValid());
	return *m_pTypeInfo;
}

inline const void* CAnyConstRef::GetValue() const
{
	return m_pValue;
}

inline CAnyConstRef::CAnyConstRef()
	: m_pTypeInfo(nullptr)
	, m_pValue(nullptr)
{}

inline bool CAnyConstRef::IsValid() const
{
	return m_pValue != nullptr;
}

inline void CAnyConstRef::Reset(const CAnyConstRef& rhs)
{
	m_pTypeInfo = rhs.m_pTypeInfo;
	m_pValue = rhs.m_pValue;
}

// CAnyPtr functions.

inline CAnyPtr::CAnyPtr() {}

inline CAnyPtr::CAnyPtr(const CCommonTypeInfo& typeInfo, void* pValue)
	: m_ref(typeInfo, pValue)
{}

template<typename TYPE> inline CAnyPtr::CAnyPtr(TYPE* pValue)
	: m_ref(Schematyc::GetTypeInfo<TYPE>(), pValue)
{}

inline CAnyPtr::CAnyPtr(CAnyValue* pValue)
	: m_ref(pValue ? CAnyRef(*pValue) : CAnyRef())
{}

inline CAnyPtr::CAnyPtr(const CAnyValuePtr& pValue)
	: m_ref(pValue ? CAnyRef(*pValue) : CAnyRef())
{}

inline CAnyPtr::CAnyPtr(const CAnyRef* rhs)
	: m_ref(rhs ? *rhs : CAnyRef())
{}

inline CAnyPtr::CAnyPtr(const CAnyPtr& rhs)
	: m_ref(rhs.m_ref)
{}

inline CAnyPtr::operator bool() const
{
	return m_ref.GetValue() != nullptr;
}

inline const CAnyRef* CAnyPtr::operator->() const
{
	SCHEMATYC_CORE_ASSERT(m_ref.IsValid());
	return &m_ref;
}

inline const CAnyRef& CAnyPtr::operator*() const
{
	SCHEMATYC_CORE_ASSERT(m_ref.IsValid());
	return m_ref;
}

inline CAnyPtr& CAnyPtr::operator=(const CAnyPtr& rhs)
{
	m_ref.Reset(rhs.m_ref);
	return *this;
}

// CAnyConstPtr functions.

inline CAnyConstPtr::CAnyConstPtr() {}

inline CAnyConstPtr::CAnyConstPtr(const CCommonTypeInfo& typeInfo, const void* pValue)
	: m_ref(typeInfo, pValue)
{}

template<typename TYPE> inline CAnyConstPtr::CAnyConstPtr(const TYPE* pValue)
	: m_ref(Schematyc::GetTypeInfo<TYPE>(), pValue)
{}

inline CAnyConstPtr::CAnyConstPtr(const CAnyValue* rhs)
	: m_ref(rhs ? CAnyConstRef(*rhs) : CAnyConstRef())
{}

inline CAnyConstPtr::CAnyConstPtr(const CAnyConstRef* pValue)
	: m_ref(pValue ? *pValue : CAnyConstRef())
{}

inline CAnyConstPtr::CAnyConstPtr(const CAnyValuePtr& pValue)
	: m_ref(pValue ? *pValue : CAnyConstRef())
{}

inline CAnyConstPtr::CAnyConstPtr(const CAnyValueConstPtr& pValue)
	: m_ref(pValue ? *pValue : CAnyConstRef())
{}

inline CAnyConstPtr::CAnyConstPtr(const CAnyPtr& rhs)
	: m_ref(*rhs)
{}

inline CAnyConstPtr::CAnyConstPtr(const CAnyConstPtr& rhs)
	: m_ref(rhs.m_ref)
{}

inline CAnyConstPtr::operator bool() const
{
	return m_ref.GetValue() != nullptr;
}

inline const CAnyConstRef* CAnyConstPtr::operator->() const
{
	SCHEMATYC_CORE_ASSERT(m_ref.IsValid());
	return &m_ref;
}

inline const CAnyConstRef& CAnyConstPtr::operator*() const
{
	SCHEMATYC_CORE_ASSERT(m_ref.IsValid());
	return m_ref;
}

inline CAnyConstPtr& CAnyConstPtr::operator=(const CAnyConstPtr& rhs)
{
	m_ref.Reset(rhs.m_ref);
	return *this;
}

// Utility functions.

namespace Any
{
inline bool CopyAssign(const CAnyRef& lhs, const CAnyConstRef& rhs)
{
	const CCommonTypeInfo& typeInfo = lhs.GetTypeInfo();
	SCHEMATYC_CORE_ASSERT_MESSAGE(typeInfo.GetMethods().copyAssign, "Type must provide copy assignment operator!");
	if (typeInfo.GetMethods().copyAssign)
	{
		const void* pRHSValue = Private::DynamicCast(rhs.GetTypeInfo(), rhs.GetValue(), typeInfo);
		SCHEMATYC_CORE_ASSERT_MESSAGE(pRHSValue, "Type mismatch!");
		if (pRHSValue)
		{
			(*typeInfo.GetMethods().copyAssign)(lhs.GetValue(), pRHSValue);
			return true;
		}
	}
	return false;
}

inline bool Equals(const CAnyConstRef& lhs, const CAnyConstRef& rhs)
{
	const CCommonTypeInfo& typeInfo = lhs.GetTypeInfo();
	SCHEMATYC_CORE_ASSERT_MESSAGE(typeInfo.GetMethods().equals, "Type must provide equals operator!");
	if (typeInfo.GetMethods().equals)
	{
		const void* pRHSValue = Private::DynamicCast(rhs.GetTypeInfo(), rhs.GetValue(), typeInfo);
		SCHEMATYC_CORE_ASSERT_MESSAGE(pRHSValue, "Type mismatch!");
		if (pRHSValue)
		{
			return (*typeInfo.GetMethods().equals)(lhs.GetValue(), pRHSValue);
		}
	}
	return false;
}

inline bool ToString(IString& output, const CAnyConstRef& input)
{
	const CCommonTypeInfo& typeInfo = input.GetTypeInfo();
	SCHEMATYC_CORE_ASSERT_MESSAGE(typeInfo.GetMethods().toString, "Type must provide 'ToString' method!");
	if (typeInfo.GetMethods().toString)
	{
		(*typeInfo.GetMethods().toString)(output, input.GetValue());
		return true;
	}
	return false;
}
} // Any

// Serialization functions.

inline bool Serialize(Serialization::IArchive& archive, const CAnyRef& value, const char* szName, const char* szLabel)
{
	const CCommonTypeInfo& typeInfo = value.GetTypeInfo();
	SCHEMATYC_CORE_ASSERT_MESSAGE(typeInfo.GetMethods().toString, "Type must provide 'Serialize' method!");
	if (typeInfo.GetMethods().serialize)
	{
		return (*typeInfo.GetMethods().serialize)(archive, value.GetValue(), szName, szLabel);
	}
	return false;
}

inline bool Serialize(Serialization::IArchive& archive, CAnyValue& value, const char* szName, const char* szLabel)
{
	const CCommonTypeInfo& typeInfo = value.GetTypeInfo();
	SCHEMATYC_CORE_ASSERT_MESSAGE(typeInfo.GetMethods().serialize, "Type must provide 'Serialize' method!");
	if (typeInfo.GetMethods().serialize)
	{
		return (*typeInfo.GetMethods().serialize)(archive, value.GetValue(), szName, szLabel);
	}
	return false;
}

// Dynamic cast functions.

template<typename TO_TYPE> TO_TYPE& DynamicCast(const CAnyRef& from)
{
	TO_TYPE* pResult = Private::DynamicCast<TO_TYPE>(from.GetTypeInfo(), from.GetValue());
	SCHEMATYC_CORE_ASSERT(pResult);
	return *pResult;
}

template<typename TO_TYPE> const TO_TYPE& DynamicCast(const CAnyConstRef& from)
{
	const TO_TYPE* pResult = Private::DynamicCast<TO_TYPE>(from.GetTypeInfo(), from.GetValue());
	SCHEMATYC_CORE_ASSERT(pResult);
	return *pResult;
}

template<typename TO_TYPE> TO_TYPE* DynamicCast(const CAnyPtr& pFrom)
{
	return pFrom ? Private::DynamicCast<TO_TYPE>(pFrom->GetTypeInfo(), pFrom->GetValue()) : nullptr;
}

template<typename TO_TYPE> const TO_TYPE* DynamicCast(const CAnyConstPtr& pFrom)
{
	return pFrom ? Private::DynamicCast<TO_TYPE>(pFrom->GetTypeInfo(), pFrom->GetValue()) : nullptr;
}
} // Schematyc
