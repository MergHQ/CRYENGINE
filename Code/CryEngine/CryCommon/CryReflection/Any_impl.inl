// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include <CryReflection/IModule.h>
#include <CryReflection/ITypeDesc.h>

#include <CrySerialization/yasli/Archive.h>

namespace Cry {
namespace Reflection {

inline CAny::CAny()
	: m_isSmallValue(false)
	, m_isPointer(false)
	, m_isConst(false)
{}

inline CAny::CAny(const CAny& other)
	: m_isSmallValue(false)
	, m_isPointer(false)
	, m_isConst(false)
{
	if (other.IsPointer())
	{
		AssignPointer(other.GetConstPointer(), other.GetTypeIndex());
		// NOTE: We have to use AssignPointer with const pointer here, therefore adjust const afterwards.
		m_isConst = other.IsConst();
	}
	else
	{
		AssignValue(other, other.IsConst());
	}
}

inline CAny::CAny(const ITypeDesc& typeDesc)
	: m_isSmallValue(false)
	, m_isPointer(false)
	, m_isConst(false)
{
	CreateDefaultValue(typeDesc);
}

template<typename VALUE_TYPE>
inline CAny::CAny(const VALUE_TYPE& source)
{
	constexpr bool isPointer = std::is_pointer<decltype(source)>::value;
	static_assert(isPointer == false, "Type of 'source' parameter is a pointer type.");

	if (isPointer == false)
	{
		constexpr CTypeId typeId = Type::IdOf<VALUE_TYPE>();
		const Reflection::ITypeDesc* pTypeDesc = CoreRegistry::GetTypeById(typeId);
		if (pTypeDesc)
		{
			m_isPointer = std::is_pointer<decltype(source)>::value;
			m_isSmallValue = (sizeof(VALUE_TYPE) <= sizeof(m_data));
			m_typeIndex = pTypeDesc->GetIndex();
			m_isPointer = isPointer;
			m_isConst = false;

			Type::CCopyConstructor copyConstructor = pTypeDesc->GetCopyConstructor();
			if (copyConstructor)
			{
				if (m_isSmallValue == false)
				{
					m_pData = CryModuleMemalign(pTypeDesc->GetSize(), pTypeDesc->GetAlignment());
					copyConstructor(m_pData, &source);
					return;
				}
				else
				{
					copyConstructor(&m_data, &source);
					return;
				}
			}
			CRY_ASSERT_MESSAGE(copyConstructor, "Type doesn't have a copy constructor.");
		}
		CRY_ASSERT_MESSAGE(pTypeDesc, "Couldn't find type in reflection registry.");
	}
}

template<typename VALUE_TYPE>
inline CAny::CAny(VALUE_TYPE* pSource)
{
	constexpr bool isPointer = std::is_pointer<decltype(pSource)>::value;
	static_assert(isPointer, "Type of 'pSource' parameter is not a pointer type.");

	if (isPointer)
	{
		constexpr CTypeId typeId = Type::IdOf<VALUE_TYPE>();
		const Reflection::ITypeDesc* pTypeDesc = CoreRegistry::GetTypeById(typeId);
		if (pTypeDesc)
		{
			m_typeIndex = pTypeDesc->GetIndex();
			m_isPointer = isPointer;
			m_isSmallValue = true;
			m_pData = const_cast<typename std::remove_const<VALUE_TYPE>::type*>(pSource);
			m_isConst = std::is_const<decltype(pSource)>::value;

			return;
		}
		CRY_ASSERT_MESSAGE(pTypeDesc, "Type not found in reflection registry.");
	}
}

inline CAny::~CAny()
{
	Destruct();
}

inline const ITypeDesc* CAny::GetTypeDesc() const
{
	if (m_typeIndex.IsValid())
	{
		const ITypeDesc* pTypeDesc = CoreRegistry::GetTypeByIndex(m_typeIndex);
		CRY_ASSERT_MESSAGE(pTypeDesc, "CTypeDesc for type index not found in reflection registry.");

		return pTypeDesc;
	}
	return nullptr;
}

inline TypeIndex CAny::GetTypeIndex() const
{
	return m_typeIndex;
}

inline void CAny::CreateDefaultValue(const ITypeDesc& typeDesc)
{
	if (m_typeIndex.IsValid() == false)
	{
		m_typeIndex = typeDesc.GetIndex();
		m_isSmallValue = (typeDesc.GetSize() <= sizeof(m_data));
		m_isConst = false;
		m_isPointer = false;

		Type::CDefaultConstructor defaultConstructor = typeDesc.GetDefaultConstructor();
		if (defaultConstructor)
		{
			if (m_isSmallValue == false)
			{
				m_pData = CryModuleMemalign(typeDesc.GetSize(), typeDesc.GetAlignment());
				defaultConstructor(m_pData);
			}
			else
			{
				defaultConstructor(&m_data);
			}
			return;
		}
		CRY_ASSERT_MESSAGE(defaultConstructor, "Default constructor for type '%s' not available!", typeDesc.GetFullQualifiedName());
	}
}

template<typename VALUE_TYPE>
inline void CAny::AssignValue(const VALUE_TYPE& source, bool isConst)
{
	constexpr bool isPointer = std::is_pointer<VALUE_TYPE>::value;
	static_assert(isPointer == false, "Type of 'source' parameter is a pointer type.");

	if ((isPointer == false) && (m_isConst == false) && (m_isPointer == false))
	{
		constexpr CTypeId typeId = Type::IdOf<VALUE_TYPE>();
		const Reflection::ITypeDesc* pTypeDesc = CoreRegistry::GetTypeById(typeId);
		if (pTypeDesc)
		{
			Type::CCopyAssignOperator copyAssignConstructor = pTypeDesc->GetCopyAssignOperator();
			if (copyAssignConstructor)
			{
				if (IsEmpty() == false)
				{
					Destruct();
				}

				m_isPointer = isPointer;
				m_isSmallValue = (sizeof(VALUE_TYPE) <= sizeof(m_data));
				m_typeIndex = pTypeDesc->GetIndex();
				m_isConst = isConst;

				if (m_isSmallValue == false)
				{
					if (m_pData == nullptr)
					{
						m_pData = CryModuleMemalign(pTypeDesc->GetSize(), pTypeDesc->GetAlignment());
					}

					copyAssignConstructor(m_pData, &source);
					return;
				}
				else
				{
					copyAssignConstructor(&m_data, &source);
					return;
				}
			}
			CRY_ASSERT_MESSAGE(copyAssignConstructor, "Type doesn't have a copy assign operator.");
		}
		CRY_ASSERT_MESSAGE(pTypeDesc, "Type not found in reflection registry.");
	}
	CRY_ASSERT_MESSAGE(m_isConst == false, "Type is a const value.");
	CRY_ASSERT_MESSAGE(m_isPointer == false, "Can't assign 'source' to pointer type.");
}

template<typename VALUE_TYPE>
inline void CAny::AssignValue(const VALUE_TYPE& source, TypeIndex typeIndex, bool isConst)
{
	constexpr bool isPointer = std::is_pointer<VALUE_TYPE>::value;
	static_assert(isPointer == false, "Type of 'source' parameter is a pointer type.");

	if ((isPointer == false) && (m_isConst == false) && (m_isPointer == false))
	{
		const Reflection::ITypeDesc* pTypeDesc = CoreRegistry::GetTypeByIndex(typeIndex);
		if (pTypeDesc)
		{
			Type::CCopyAssignOperator copyAssignConstructor = pTypeDesc->GetCopyAssignOperator();
			if (copyAssignConstructor)
			{
				if (IsEmpty() == false)
				{
					Destruct();
				}

				m_isPointer = isPointer;
				m_isSmallValue = (sizeof(VALUE_TYPE) <= sizeof(m_data));
				m_typeIndex = typeIndex;
				m_isConst = isConst;

				if (m_isSmallValue == false)
				{
					if (m_pData == nullptr)
					{
						m_pData = CryModuleMemalign(pTypeDesc->GetSize(), pTypeDesc->GetAlignment());
					}

					copyAssignConstructor(m_pData, &source);
					return;
				}
				else
				{
					copyAssignConstructor(&m_data, &source);
					return;
				}
			}
			CRY_ASSERT_MESSAGE(copyAssignConstructor, "Type doesn't have a copy assign operator.");
		}
		CRY_ASSERT_MESSAGE(pTypeDesc, "Type not found in reflection registry.");
	}
	CRY_ASSERT_MESSAGE(m_isConst == false, "Type is a const value.");
	CRY_ASSERT_MESSAGE(m_isPointer == false, "Can't assign 'source' to pointer type.");
}

template<typename VALUE_TYPE>
inline void CAny::AssignValue(const VALUE_TYPE& source, const ITypeDesc& srcTypeDesc, bool isConst)
{
	constexpr bool isPointer = std::is_pointer<VALUE_TYPE>::value;
	static_assert(isPointer == false, "Type of 'source' parameter is a pointer type.");

	if ((isPointer == false) && (m_isConst == false) && (m_isPointer == false))
	{
		Type::CCopyAssignOperator copyAssignConstructor = srcTypeDesc.GetCopyAssignOperator();
		if (copyAssignConstructor)
		{
			if (IsEmpty() == false)
			{
				Destruct();
			}

			m_isPointer = isPointer;
			m_isSmallValue = (sizeof(VALUE_TYPE) <= sizeof(m_data));
			m_typeIndex = srcTypeDesc.GetIndex();
			m_isConst = isConst;

			if (m_isSmallValue == false)
			{
				if (m_pData == nullptr)
				{
					m_pData = CryModuleMemalign(srcTypeDesc.GetSize(), srcTypeDesc.GetAlignment());
				}

				copyAssignConstructor(m_pData, &source);
				return;
			}
			else
			{
				copyAssignConstructor(&m_data, &source);
				return;
			}
		}
		CRY_ASSERT_MESSAGE(copyAssignConstructor, "Type doesn't have a copy assign operator.");
	}
	CRY_ASSERT_MESSAGE(m_isConst == false, "Type is a const value.");
	CRY_ASSERT_MESSAGE(m_isPointer == false, "Can't assign 'source' to pointer type.");
}

inline void CAny::AssignValue(const CAny& source, bool isConst)
{
	if (source.IsEmpty() == false)
	{
		AssignValue(source.GetConstPointer(), source.GetTypeIndex(), isConst);
	}
}

inline void CAny::AssignValue(const void* pSource, TypeIndex typeIndex, bool isConst)
{
	if (pSource && (m_isConst == false) && (m_isPointer == false))
	{
		const Reflection::ITypeDesc* pTypeDesc = CoreRegistry::GetTypeByIndex(typeIndex);
		if (pTypeDesc)
		{
			Type::CCopyAssignOperator copyAssignOperator = pTypeDesc->GetCopyAssignOperator();
			if (copyAssignOperator)
			{
				if (IsEmpty() == false)
				{
					Destruct();
				}

				m_isPointer = false;
				m_isSmallValue = (pTypeDesc->GetSize() <= sizeof(m_data));
				m_typeIndex = pTypeDesc->GetIndex();
				m_isConst = isConst;

				if (m_isSmallValue == false)
				{
					if (m_pData == nullptr)
					{
						m_pData = CryModuleMemalign(pTypeDesc->GetSize(), pTypeDesc->GetAlignment());
					}

					copyAssignOperator(m_pData, pSource);
					return;
				}
				else
				{
					copyAssignOperator(&m_data, pSource);
					return;
				}
			}
			CRY_ASSERT_MESSAGE(copyAssignOperator, "Type doesn't have a copy assign operator.");
		}
		CRY_ASSERT_MESSAGE(pTypeDesc, "Couldn't get type desc for index.");
	}
	CRY_ASSERT_MESSAGE(pSource, "Parameter must be non-null.");
	CRY_ASSERT_MESSAGE(m_isConst == false, "Type is a const value.");
	CRY_ASSERT_MESSAGE(m_isPointer == false, "Can't assign 'pSource' to pointer type.");
}

inline void CAny::AssignValue(const void* pSource, const ITypeDesc& srcTypeDesc, bool isConst)
{
	if (pSource && (m_isConst == false) && (m_isPointer == false))
	{
		Type::CCopyAssignOperator copyAssignOperator = srcTypeDesc.GetCopyAssignOperator();
		if (copyAssignOperator)
		{
			if (IsEmpty() == false)
			{
				Destruct();
			}

			m_isPointer = false;
			m_isSmallValue = (srcTypeDesc.GetSize() <= sizeof(m_data));
			m_typeIndex = srcTypeDesc.GetIndex();
			m_isConst = isConst;

			if (m_isSmallValue == false)
			{
				if (m_pData == nullptr)
				{
					m_pData = CryModuleMemalign(srcTypeDesc.GetSize(), srcTypeDesc.GetAlignment());
				}

				copyAssignOperator(m_pData, pSource);
				return;
			}
			else
			{
				copyAssignOperator(&m_data, pSource);
				return;
			}
		}
		CRY_ASSERT_MESSAGE(copyAssignOperator, "Type doesn't have a copy assign operator.");
	}
	CRY_ASSERT_MESSAGE(pSource, "Parameter must be non-null.");
	CRY_ASSERT_MESSAGE(m_isConst == false, "Type is a const value.");
	CRY_ASSERT_MESSAGE(m_isPointer == false, "Can't assign 'pSource' to pointer type.");
}

template<typename VALUE_TYPE>
inline void CAny::AssignPointer(VALUE_TYPE* pPointer)
{
	constexpr bool isPointer = std::is_pointer<decltype(pPointer)>::value;
	static_assert(isPointer, "Type of 'pPointer' parameter is not a pointer type.");

	if (isPointer && (m_isPointer || IsEmpty()))
	{
		constexpr CTypeId typeId = Type::IdOf<VALUE_TYPE>();
		const Reflection::ITypeDesc* pTypeDesc = CoreRegistry::GetTypeById(typeId);
		if (pTypeDesc)
		{
			m_typeIndex = pTypeDesc->GetIndex();
			m_isPointer = isPointer;
			m_isSmallValue = true;
			m_pData = const_cast<typename std::remove_const<VALUE_TYPE>::type*>(pPointer);
			m_isConst = std::is_const<VALUE_TYPE>::value;

			return;
		}
		CRY_ASSERT_MESSAGE(pTypeDesc, "Type not found in reflection registry.");
	}
	CRY_ASSERT_MESSAGE(m_isPointer, "Can't assign 'pPointer' to value type.");
}

template<typename VALUE_TYPE>
inline void CAny::AssignPointer(VALUE_TYPE* pPointer, TypeIndex typeIndex)
{
	constexpr bool isPointer = std::is_pointer<decltype(pPointer)>::value;
	static_assert(isPointer, "Type of 'pPointer' parameter is not a pointer type.");

	if (isPointer && (m_isPointer || IsEmpty()))
	{
		const Reflection::ITypeDesc* pTypeDesc = CoreRegistry::GetTypeByIndex(m_typeIndex);
		if (pTypeDesc)
		{
			m_typeIndex = pTypeDesc->GetIndex();
			m_isPointer = isPointer;
			m_isSmallValue = true;
			m_pData = const_cast<typename  std::remove_const<VALUE_TYPE>::type*>(pPointer);
			m_isConst = std::is_const<VALUE_TYPE>::value;

			return;
		}
		CRY_ASSERT_MESSAGE(pTypeDesc, "Type not found in reflection registry.");
	}
	CRY_ASSERT_MESSAGE(m_isPointer, "Can't assign 'pPointer' to value type.");
}

template<typename VALUE_TYPE>
inline void CAny::AssignPointer(VALUE_TYPE* pPointer, const ITypeDesc& srcTypeDesc)
{
	constexpr bool isPointer = std::is_pointer<decltype(pPointer)>::value;
	static_assert(isPointer, "Type of 'pPointer' parameter is not a pointer type.");

	if (isPointer && (m_isPointer || IsEmpty()))
	{
		m_typeIndex = srcTypeDesc.GetIndex();
		m_isPointer = isPointer;
		m_isSmallValue = true;
		m_pData = const_cast<typename std::remove_const<VALUE_TYPE>::type*>(pPointer);
		m_isConst = std::is_const<VALUE_TYPE>::value;

		return;
	}
	CRY_ASSERT_MESSAGE(m_isPointer, "Can't assign 'pPointer' to value type.");
}

inline void CAny::AssignPointer(void* pPointer, TypeIndex typeIndex)
{
	if (m_isPointer || IsEmpty())
	{
		const Reflection::ITypeDesc* pTypeDesc = CoreRegistry::GetTypeByIndex(typeIndex);
		if (pTypeDesc)
		{
			m_typeIndex = pTypeDesc->GetIndex();
			m_isPointer = true;
			m_isSmallValue = true;
			m_pData = pPointer;
			m_isConst = false;

			return;
		}
		CRY_ASSERT_MESSAGE(pTypeDesc, "Type not found in reflection registry.");
	}
	CRY_ASSERT_MESSAGE(m_isPointer, "Can't assign 'pPointer' to value type.");
}

inline void CAny::AssignPointer(void* pPointer, const ITypeDesc& srcTypeDesc)
{
	if (m_isPointer || IsEmpty())
	{
		m_typeIndex = srcTypeDesc.GetIndex();
		m_isPointer = true;
		m_isSmallValue = true;
		m_pData = pPointer;
		m_isConst = false;

		return;
	}
	CRY_ASSERT_MESSAGE(m_isPointer, "Can't assign 'pPointer' to value type.");
}

inline void CAny::AssignPointer(const void* pPointer, TypeIndex typeIndex)
{
	if (m_isPointer || IsEmpty())
	{
		const Reflection::ITypeDesc* pTypeDesc = CoreRegistry::GetTypeByIndex(typeIndex);
		if (pTypeDesc)
		{
			m_typeIndex = pTypeDesc->GetIndex();
			m_isPointer = true;
			m_isSmallValue = true;
			m_pData = const_cast<void*>(pPointer);
			m_isConst = true;

			return;
		}
		CRY_ASSERT_MESSAGE(pTypeDesc, "Type not found in reflection registry.");
	}
	CRY_ASSERT_MESSAGE(m_isPointer, "Can't assign 'pPointer' to value type.");
}

inline void CAny::AssignPointer(const void* pPointer, const ITypeDesc& srcTypeDesc)
{
	if (m_isPointer || IsEmpty())
	{
		m_typeIndex = srcTypeDesc.GetIndex();
		m_isPointer = true;
		m_isSmallValue = true;
		m_pData = const_cast<void*>(pPointer);
		m_isConst = true;

		return;
	}
	CRY_ASSERT_MESSAGE(m_isPointer, "Can't assign 'pPointer' to value type.");
}

template<typename VALUE_TYPE>
inline bool CAny::CopyValue(VALUE_TYPE& target) const
{
	constexpr CTypeId typeId = Type::IdOf<VALUE_TYPE>();
	const ITypeDesc* pDstTypeDesc = CoreRegistry::GetTypeById(typeId);
	if (pDstTypeDesc)
	{
		const ITypeDesc* pSrcTypeDesc = CoreRegistry::GetTypeByIndex(m_typeIndex);
		if (pSrcTypeDesc)
		{
			if (pDstTypeDesc == pSrcTypeDesc)
			{
				target = *static_cast<const VALUE_TYPE*>(GetConstPointer());
				return true;
			}
			else
			{
				Type::CConversionOperator conversionOperator = pDstTypeDesc->GetConversionOperator(pSrcTypeDesc->GetTypeId());
				if (conversionOperator)
				{
					conversionOperator(&target, GetConstPointer());
					return true;
				}
			}
		}
		CRY_ASSERT_MESSAGE(pSrcTypeDesc, "Source type description not found.");
	}
	CRY_ASSERT_MESSAGE(pDstTypeDesc, "Target type description not found.");

	return false;
}

inline bool CAny::CopyValue(void* pTarget, TypeIndex typeIndex) const
{
	if (pTarget)
	{
		const ITypeDesc* pDstTypeDesc = CoreRegistry::GetTypeByIndex(typeIndex);
		if (pDstTypeDesc)
		{
			const ITypeDesc* pSrcTypeDesc = CoreRegistry::GetTypeByIndex(m_typeIndex);
			if (pSrcTypeDesc)
			{
				if (pDstTypeDesc == pSrcTypeDesc)
				{
					Type::CCopyAssignOperator copyOperator = pDstTypeDesc->GetCopyAssignOperator();
					copyOperator(pTarget, GetConstPointer());
					return true;
				}
				else
				{
					Type::CConversionOperator conversionOperator = pDstTypeDesc->GetConversionOperator(pSrcTypeDesc->GetTypeId());
					if (conversionOperator)
					{
						conversionOperator(pTarget, GetConstPointer());
						return true;
					}
				}
			}
			CRY_ASSERT_MESSAGE(pSrcTypeDesc, "Source type description not found.");
		}
		CRY_ASSERT_MESSAGE(pDstTypeDesc, "Target type description not found.");
	}
	CRY_ASSERT_MESSAGE(pTarget, "Parameter must be non-null.");

	return false;
}

inline bool CAny::CopyValue(void* pTarget, const ITypeDesc& dstTypeDesc) const
{
	if (pTarget)
	{
		const ITypeDesc* pSrcTypeDesc = CoreRegistry::GetTypeByIndex(m_typeIndex);
		if (pSrcTypeDesc)
		{
			if (&dstTypeDesc == pSrcTypeDesc)
			{
				Type::CCopyAssignOperator copyOperator = dstTypeDesc.GetCopyAssignOperator();
				copyOperator(pTarget, GetConstPointer());
				return true;
			}
			else
			{
				Type::CConversionOperator conversionOperator = dstTypeDesc.GetConversionOperator(pSrcTypeDesc->GetTypeId());
				if (conversionOperator)
				{
					conversionOperator(pTarget, GetConstPointer());
					return true;
				}
			}
		}
		CRY_ASSERT_MESSAGE(pSrcTypeDesc, "Source type description not found.");
	}
	CRY_ASSERT_MESSAGE(pTarget, "Parameter must be non-null.");

	return false;
}

template<typename VALUE_TYPE>
inline void CAny::AssignToPointer(const VALUE_TYPE& source)
{
	static_assert(std::is_copy_assignable<VALUE_TYPE>::value, "'source' is not copy assignable.");

	if (m_isPointer && (GetPointer() != nullptr))
	{
		constexpr CTypeId typeId = Type::IdOf<VALUE_TYPE>();
		const Reflection::ITypeDesc* pTypeDesc = CoreRegistry::GetTypeById(typeId);
		if (pTypeDesc && (m_typeIndex == pTypeDesc->GetIndex()))
		{
			*static_cast<VALUE_TYPE*>(m_pData) = source;
			return;
		}
		CRY_ASSERT_MESSAGE(pTypeDesc, "Type not found in reflection registry.");
		CRY_ASSERT_MESSAGE(m_typeIndex == pTypeDesc->GetIndex(), "Can't assign 'source' type to different pointer type.");
	}
	CRY_ASSERT_MESSAGE(GetPointer() != nullptr, "Pointer type must be non-null.");
	CRY_ASSERT_MESSAGE(m_isPointer, "Can't assign 'source' to value type.");
}

template<typename VALUE_TYPE>
inline void CAny::AssignToPointer(const VALUE_TYPE& source, TypeIndex typeIndex)
{
	static_assert(std::is_copy_assignable<VALUE_TYPE>::value, "'source' is not copy assignable.");

	if (m_isPointer && (GetPointer() != nullptr))
	{
		const Reflection::ITypeDesc* pTypeDesc = CoreRegistry::GetTypeByIndex(typeIndex);
		if (pTypeDesc && (m_typeIndex == typeIndex))
		{
			*static_cast<VALUE_TYPE*>(m_pData) = source;
			return;
		}
		CRY_ASSERT_MESSAGE(pTypeDesc, "Type not found in reflection registry.");
		CRY_ASSERT_MESSAGE(m_typeIndex == typeIndex, "Can't assign 'source' type to different pointer type.");
	}
	CRY_ASSERT_MESSAGE(GetPointer() != nullptr, "Pointer type must be non-null.");
	CRY_ASSERT_MESSAGE(m_isPointer, "Can't assign 'source' to value type.");
}

template<typename VALUE_TYPE>
inline void CAny::AssignToPointer(const VALUE_TYPE& source, const ITypeDesc& srcTypeDesc)
{
	static_assert(std::is_copy_assignable<VALUE_TYPE>::value, "'source' is not copy assignable.");

	if (m_isPointer && (GetPointer() != nullptr))
	{
		if (m_typeIndex == srcTypeDesc.GetIndex())
		{
			*static_cast<VALUE_TYPE*>(m_pData) = source;
			return;
		}
		CRY_ASSERT_MESSAGE(m_typeIndex == srcTypeDesc.GetIndex(), "Can't assign 'source' type to different pointer type.");
	}
	CRY_ASSERT_MESSAGE(GetPointer() != nullptr, "Pointer type must be non-null.");
	CRY_ASSERT_MESSAGE(m_isPointer, "Can't assign 'source' to value type.");
}

inline void CAny::AssignToPointer(void* pSource, TypeIndex typeIndex)
{
	if (m_isPointer && (GetPointer() != nullptr))
	{
		const Reflection::ITypeDesc* pTypeDesc = CoreRegistry::GetTypeByIndex(typeIndex);
		if (pTypeDesc && (m_typeIndex == typeIndex))
		{
			Type::CCopyAssignOperator copyAssignConstructor = pTypeDesc->GetCopyAssignOperator();
			if (copyAssignConstructor)
			{
				copyAssignConstructor(m_pData, pSource);
				return;
			}
			CRY_ASSERT_MESSAGE(copyAssignConstructor, "Type doesn't have a copy assign operator.");
		}
		CRY_ASSERT_MESSAGE(pTypeDesc, "Type not found in reflection registry.");
		CRY_ASSERT_MESSAGE(m_typeIndex == typeIndex, "Can't assign 'source' type to different pointer type.");
	}
	CRY_ASSERT_MESSAGE(GetPointer() != nullptr, "Pointer type must be non-null.");
	CRY_ASSERT_MESSAGE(m_isPointer, "Can't assign 'source' to value type.");
}

inline void CAny::AssignToPointer(void* pSource, const ITypeDesc& srcTypeDesc)
{
	if (m_isPointer && (GetPointer() != nullptr))
	{
		if (m_typeIndex == srcTypeDesc.GetIndex())
		{
			Type::CCopyAssignOperator copyAssignConstructor = srcTypeDesc.GetCopyAssignOperator();
			if (copyAssignConstructor)
			{
				copyAssignConstructor(m_pData, pSource);
				return;
			}
			CRY_ASSERT_MESSAGE(copyAssignConstructor, "Type doesn't have a copy assign operator.");
		}
		CRY_ASSERT_MESSAGE(m_typeIndex == srcTypeDesc.GetIndex(), "Can't assign 'source' type to different pointer type.");
	}
	CRY_ASSERT_MESSAGE(GetPointer() != nullptr, "Pointer type must be non-null.");
	CRY_ASSERT_MESSAGE(m_isPointer, "Can't assign 'source' to value type.");
}

inline void CAny::AssignToPointer(const void* pSource, TypeIndex typeIndex)
{
	if (m_isPointer && (GetPointer() != nullptr))
	{
		const Reflection::ITypeDesc* pTypeDesc = CoreRegistry::GetTypeByIndex(typeIndex);
		if (pTypeDesc && (m_typeIndex == typeIndex))
		{
			Type::CCopyAssignOperator copyAssignConstructor = pTypeDesc->GetCopyAssignOperator();
			if (copyAssignConstructor)
			{
				copyAssignConstructor(m_pData, pSource);
				return;
			}
			CRY_ASSERT_MESSAGE(copyAssignConstructor, "Type doesn't have a copy assign operator.");
		}
		CRY_ASSERT_MESSAGE(pTypeDesc, "Type not found in reflection registry.");
		CRY_ASSERT_MESSAGE(m_typeIndex == typeIndex, "Can't assign 'source' type to different pointer type.");
	}
	CRY_ASSERT_MESSAGE(GetPointer() != nullptr, "Pointer type must be non-null.");
	CRY_ASSERT_MESSAGE(m_isPointer, "Can't assign 'source' to value type.");
}

inline void CAny::AssignToPointer(const void* pSource, const ITypeDesc& srcTypeDesc)
{
	if (m_isPointer && (GetPointer() != nullptr))
	{
		if (m_typeIndex == srcTypeDesc.GetIndex())
		{
			Type::CCopyAssignOperator copyAssignConstructor = srcTypeDesc.GetCopyAssignOperator();
			if (copyAssignConstructor)
			{
				copyAssignConstructor(m_pData, pSource);
				return;
			}
			CRY_ASSERT_MESSAGE(copyAssignConstructor, "Type doesn't have a copy assign operator.");
		}
		CRY_ASSERT_MESSAGE(m_typeIndex == srcTypeDesc.GetIndex(), "Can't assign 'source' type to different pointer type.");
	}
	CRY_ASSERT_MESSAGE(GetPointer() != nullptr, "Pointer type must be non-null.");
	CRY_ASSERT_MESSAGE(m_isPointer, "Can't assign 'source' to value type.");
}

template<typename VALUE_TYPE>
inline VALUE_TYPE* CAny::GetPointer()
{
	if (!m_isConst)
	{
		constexpr CTypeId typeId = Type::IdOf<VALUE_TYPE>();
		const ITypeDesc* pTypeDesc = CoreRegistry::GetTypeByIndex(m_typeIndex);
		if (pTypeDesc)
		{
			if (pTypeDesc->GetTypeId() == typeId)
			{
				return reinterpret_cast<VALUE_TYPE*>(GetPointer());
			}
		}
		CRY_ASSERT_MESSAGE(pTypeDesc, "Type not found in reflection registry.");

	}
	return nullptr;
}

inline void* CAny::GetPointer()
{
	if (!m_isConst)
	{
		return (m_isSmallValue && !m_isPointer ? &m_data : m_pData);
	}
	return nullptr;
}

template<typename VALUE_TYPE>
inline const VALUE_TYPE* CAny::GetConstPointer() const
{
	constexpr CTypeId typeId = Type::IdOf<VALUE_TYPE>();
	const ITypeDesc* pTypeDesc = CoreRegistry::GetTypeByIndex(m_typeIndex);
	if (pTypeDesc)
	{
		if (pTypeDesc->GetTypeId() == typeId)
		{
			return reinterpret_cast<const VALUE_TYPE*>(GetConstPointer());
		}
	}
	CRY_ASSERT_MESSAGE(pTypeDesc, "Type not found in reflection registry.");

	return nullptr;
}

inline const void* CAny::GetConstPointer() const
{
	return (m_isSmallValue && !m_isPointer ? &m_data : m_pData);
}

template<typename VALUE_TYPE>
inline bool CAny::IsEqual(const VALUE_TYPE& other) const
{
	constexpr CTypeId typeId = Type::IdOf<VALUE_TYPE>();
	const ITypeDesc* pTypeDesc = CoreRegistry::GetTypeByIndex(m_typeIndex);
	if (pTypeDesc)
	{
		if (pTypeDesc->GetTypeId() == typeId)
		{
			Type::CEqualOperator equalOperator = pTypeDesc->GetEqualOperator();
			if (equalOperator)
			{
				return equalOperator(GetConstPointer(), &other);
			}
			CRY_ASSERT_MESSAGE(equalOperator, "No equal operator available for type '%s'.", pTypeDesc->GetFullQualifiedName());
		}
		return false;
	}
	CRY_ASSERT_MESSAGE(pTypeDesc, "Type not found in reflection registry.");

	return false;
}

inline bool CAny::IsEqual(const CAny& other) const
{
	if (other.m_typeIndex == m_typeIndex)
	{
		if (!m_isPointer && !other.m_isPointer)
		{
			const ITypeDesc* pTypeDesc = CoreRegistry::GetTypeByIndex(m_typeIndex);
			if (pTypeDesc)
			{
				Type::CEqualOperator equalOperator = pTypeDesc->GetEqualOperator();
				if (equalOperator)
				{
					return equalOperator(GetConstPointer(), other.GetConstPointer());
				}
				CRY_ASSERT_MESSAGE(equalOperator, "No equal operator available for type '%s'.", pTypeDesc->GetFullQualifiedName());
			}
			CRY_ASSERT_MESSAGE(pTypeDesc, "Type not found in reflection registry.");
		}
		else if (m_isPointer && other.m_isPointer)
		{
			return IsEqualPointer(other.GetConstPointer());
		}
		CRY_ASSERT_MESSAGE(m_isPointer && other.m_isPointer, "Pointer with value comparison is not valid.");
	}
	return false;
}

inline bool CAny::IsEqualPointer(const void* pSource) const
{
	if (m_isPointer)
	{
		return (GetConstPointer() == pSource);
	}
	return false;
}

inline bool CAny::IsType(const ITypeDesc& typeDesc) const
{
	return (m_typeIndex == typeDesc.GetIndex());
}

inline void CAny::Destruct()
{
	if (m_isPointer == false)
	{
		if (m_typeIndex.IsValid())
		{
			const ITypeDesc* pTypeDesc = CoreRegistry::GetTypeByIndex(m_typeIndex);
			if (pTypeDesc)
			{
				Type::CDestructor destructor = pTypeDesc->GetDestructor();
				if (destructor)
				{
					if (m_isSmallValue)
					{
						destructor(&m_data);
					}
					else
					{
						destructor(m_pData);
						CryModuleMemalignFree(m_pData);
					}
				}
				else
				{
					CRY_ASSERT_MESSAGE(destructor, "Type '%s' must have a destructor.", pTypeDesc->GetFullQualifiedName());
				}
			}
			else
			{
				CRY_ASSERT_MESSAGE(pTypeDesc, "Type is not registered in reflection database.");
			}
		}
	}

	m_isSmallValue = false;
	m_isPointer = false;
	m_isConst = false;
	m_typeIndex = TypeIndex::Invalid;
	m_data = 0;
}

inline bool CAny::Serialize(Serialization::IArchive& archive)
{
	const bool isLoad = archive.isInput();

	bool isPointer = m_isPointer;
	bool isConst = m_isConst;

	archive(isPointer, "isPointer");
	archive(isConst, "isConst");

	if (isLoad)
	{
		CGuid loadedGuid;
		archive(loadedGuid, "guid");

		const Reflection::ITypeDesc* pTypeDesc = CoreRegistry::GetTypeByGuid(loadedGuid);
		if (pTypeDesc)
		{
			m_typeIndex = pTypeDesc->GetIndex();
			m_isPointer = isPointer;
			m_isConst = isConst;

			// Account for possible changes in the implementation.
			m_isSmallValue = (pTypeDesc->GetSize() <= sizeof(m_data));
		}
		// TODO: This is a data error. Replace assert with a log message when available.
		CRY_ASSERT_MESSAGE(pTypeDesc, "Type '%s' doesn't exist in the type registry anymore.", pTypeDesc->GetLabel());

		// This case should never occur. It means pointer doesn't fit in internal buffer size anymore
		CRY_ASSERT_MESSAGE(!(m_isPointer && !m_isSmallValue), "Type '%s' doesn't exist in the type registry anymore.", pTypeDesc->GetLabel());
		if (m_isPointer && !m_isSmallValue)
		{
			return false;
		}
		// ~TODO
	}
	else
	{
		const Reflection::ITypeDesc* pTypeDesc = GetTypeDesc();
		if (pTypeDesc)
		{
			archive(pTypeDesc->GetGuid(), "guid");
		}
		CRY_ASSERT_MESSAGE(pTypeDesc, "Any is typeless or type doesn't exist anymore.");
	}

	if (m_isSmallValue)
	{
		archive(m_data, "data");
		return true;
	}
	else
	{
		const Reflection::ITypeDesc* pTypeDesc = GetTypeDesc();
		if (pTypeDesc)
		{
			if (isLoad)
			{
				Type::CDefaultConstructor defaultConstructor = pTypeDesc->GetDefaultConstructor();
				if (defaultConstructor)
				{
					m_pData = CryModuleMemalign(pTypeDesc->GetSize(), pTypeDesc->GetAlignment());
					defaultConstructor(m_pData);
				}
				CRY_ASSERT_MESSAGE(defaultConstructor, "Type '%s' doesn't have a default constructor.", pTypeDesc->GetLabel());
			}

			Type::CSerializeFunction serializeFunction = pTypeDesc->GetSerializeFunction();
			if (serializeFunction)
			{
				return serializeFunction(archive, m_pData, "data", nullptr);
			}
			CRY_ASSERT_MESSAGE(serializeFunction, "Type '%s' doesn't have a serialize function.", pTypeDesc->GetLabel());
		}
		CRY_ASSERT_MESSAGE(pTypeDesc, "Any is typeless or type doesn't exist anymore.");
	}
	return false;
}

// TODO: We may want a user friendly string here instead in the future
inline string CAny::ToString() const
{
	bool isSmallValue = m_isSmallValue;
	bool isPointer = m_isPointer;
	bool isConst = m_isConst;

	string anyArrayString;
	anyArrayString.Format("{ \"m_pData\": %p, \"m_typeIndex\": %u, \"m_isSmallValue\": %u, \"m_isPointer\": %u, \"m_isConst\": %u }",
	                      m_pData, static_cast<TypeIndex::ValueType>(m_typeIndex), isSmallValue, isPointer, isConst);
	return anyArrayString;
}
// ~TODO

inline CAny& CAny::operator=(const CAny& other)
{
	if (this != &other)
	{
		Destruct();
		if (other.IsPointer())
		{
			AssignPointer(other.GetConstPointer(), other.GetTypeIndex());
			// NOTE: We have to use AssignPointer with const pointer here, therefore adjust const afterwards.
			m_isConst = other.IsConst();
		}
		else
		{
			AssignValue(other, other.IsConst());
		}
	}
	return *this;
}

inline CAny& CAny::operator=(CAny&& other)
{
	if (this != &other)
	{
		m_data = other.m_data;
		m_isSmallValue = other.m_isSmallValue;
		m_isPointer = other.m_isPointer;
		m_isConst = other.m_isConst;
		m_typeIndex = other.m_typeIndex;

		other.m_data = 0;
		other.m_isSmallValue = false;
		other.m_isPointer = false;
		other.m_isConst = false;
		other.m_typeIndex = TypeIndex::Invalid;
	}
	return *this;
}

template<typename VALUE_TYPE>
inline VALUE_TYPE* DynamicCast(CAny& anyValue)
{
	return anyValue.GetPointer<VALUE_TYPE>();
}

template<typename VALUE_TYPE>
inline const VALUE_TYPE* DynamicCast(const CAny& anyValue)
{
	return anyValue.GetPointer<VALUE_TYPE>();
}

} // ~Reflection namespace
} // ~Cry namespace
