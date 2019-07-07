// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include <CryReflection/Framework/Common.h>
#include <CryReflection/Any.h>
#include <CryReflection/ITypeDesc.h>
#include <CryReflection/IBaseTypeDesc.h>
#include <CryReflection/IVariableDesc.h>

namespace Cry {
namespace Reflection {

class CMemberVariablePointer
{
public:
	// Class members
	template<typename TYPE>
	CMemberVariablePointer(TYPE* pClassInstance, size_t index);
	CMemberVariablePointer(void* pClassInstance, const ITypeDesc& typeDesc, size_t index);
	template<typename TYPE>
	CMemberVariablePointer(TYPE* pClassInstance, const IVariableDesc& variableDesc);
	CMemberVariablePointer(void* pClassInstance, const IVariableDesc& variableDesc);

	// Base class members
	template<typename TYPE>
	CMemberVariablePointer(TYPE* pClassInstance, CTypeId baseClassTypeId, size_t index);
	CMemberVariablePointer(void* pClassInstance, CTypeId baseClassTypeId, const ITypeDesc& classTypeDesc, size_t index);
	CMemberVariablePointer(void* pClassInstance, const IBaseTypeDesc& baseClassTypeDesc, const ITypeDesc& classTypeDesc, size_t index);
	template<typename TYPE>
	CMemberVariablePointer(TYPE* pClassInstance, const IBaseTypeDesc& baseClassTypeDesc, const IVariableDesc& variableDesc);
	CMemberVariablePointer(void* pClassInstance, const IBaseTypeDesc& baseClassTypeDesc, const IVariableDesc& variableDesc);

	void*            GetPointer()  const { return m_pPointer; }
	const ITypeDesc* GetTypeDesc() const { return m_pTypeDesc; }
	CAny             ToAny() const;

	operator bool() const { return m_pPointer && m_pTypeDesc; }

private:
	const IBaseTypeDesc* GetBaseClassDesc(CTypeId typeId, const ITypeDesc& typeDesc) const;
private:
	void*            m_pPointer;
	const ITypeDesc* m_pTypeDesc;
};

template<typename TYPE>
inline CMemberVariablePointer::CMemberVariablePointer(TYPE* pClassInstance, size_t index)
	: m_pPointer(nullptr)
	, m_pTypeDesc(nullptr)
{
	constexpr CTypeId classTypeId = Type::IdOf<TYPE>();
	const ITypeDesc* pClassTypeDesc = CoreRegistry::GetTypeById(classTypeId);
	if (pClassTypeDesc)
	{
		const IVariableDesc* pVariableDesc = pClassTypeDesc->GetVariableByIndex(index);
		if (pVariableDesc)
		{
			m_pPointer = static_cast<void*>(reinterpret_cast<char*>(pClassInstance) + pVariableDesc->GetOffset());
			m_pTypeDesc = pVariableDesc->GetTypeDesc();
			return;
		}
		CRY_ASSERT(pVariableDesc, "Variable with index '%i' not found in class type description.", index);
	}
	CRY_ASSERT(pClassTypeDesc, "Class type description not found in reflection registry.");
}

inline CMemberVariablePointer::CMemberVariablePointer(void* pClassInstance, const ITypeDesc& classTypeDesc, size_t index)
	: m_pPointer(nullptr)
	, m_pTypeDesc(nullptr)
{
	const IVariableDesc* pVariableDesc = classTypeDesc.GetVariableByIndex(index);
	if (pVariableDesc)
	{
		m_pPointer = static_cast<void*>(reinterpret_cast<char*>(pClassInstance) + pVariableDesc->GetOffset());
		m_pTypeDesc = pVariableDesc->GetTypeDesc();
		return;
	}
	CRY_ASSERT(pVariableDesc, "Variable with index '%i' not found in class type description.", index);
}

template<typename TYPE>
inline CMemberVariablePointer::CMemberVariablePointer(TYPE* pClassInstance, const IVariableDesc& variableDesc)
{
	m_pPointer = static_cast<void*>(reinterpret_cast<char*>(pClassInstance) + variableDesc.GetOffset());
	m_pTypeDesc = variableDesc.GetTypeDesc();
}

inline CMemberVariablePointer::CMemberVariablePointer(void* pClassInstance, const IVariableDesc& variableDesc)
{
	m_pPointer = static_cast<void*>(reinterpret_cast<char*>(pClassInstance) + variableDesc.GetOffset());
	m_pTypeDesc = variableDesc.GetTypeDesc();
}

template<typename TYPE>
inline CMemberVariablePointer::CMemberVariablePointer(TYPE* pClassInstance, CTypeId baseClassTypeId, size_t index)
	: m_pPointer(nullptr)
	, m_pTypeDesc(nullptr)
{
	constexpr CTypeId classTypeId = Type::IdOf<TYPE>();
	const ITypeDesc* pClassTypeDesc = CoreRegistry::GetTypeById(classTypeId);
	if (pClassTypeDesc)
	{
		const IBaseTypeDesc* pBaseTypeDesc = GetBaseClassDesc(baseClassTypeId, *pClassTypeDesc);
		if (pBaseTypeDesc)
		{
			const ITypeDesc* pBaseClassTypeDesc = pBaseTypeDesc->GetTypeDesc();
			if (pBaseClassTypeDesc)
			{
				Type::CBaseCastFunction baseCastFunction = pBaseTypeDesc->GetBaseCastFunction();
				if (baseCastFunction)
				{
					void* pBaseClass = baseCastFunction(static_cast<void*>(pClassInstance));
					if (pBaseClass)
					{
						const IVariableDesc* pVariableDesc = pBaseClassTypeDesc->GetVariableByIndex(index);
						if (pVariableDesc)
						{
							m_pPointer = static_cast<void*>(reinterpret_cast<char*>(pBaseClass) + pVariableDesc->GetOffset());
							m_pTypeDesc = pVariableDesc->GetTypeDesc();
							return;
						}
						CRY_ASSERT(pVariableDesc, "Variable with index '%i' not found in class type description.", index);
					}
					CRY_ASSERT(pBaseClass, "Base class cast must be non-null.");
				}
				CRY_ASSERT(baseCastFunction, "Base cast function must be set.");
			}
			CRY_ASSERT(pBaseClassTypeDesc, "Base class type description not found in reflection registry.");
		}
		CRY_ASSERT(pBaseTypeDesc, "Base class type description not found in class description.");
	}
	CRY_ASSERT(pClassTypeDesc, "Class type description not found in reflection registry.");
}

inline CMemberVariablePointer::CMemberVariablePointer(void* pClassInstance, CTypeId baseClassTypeId, const ITypeDesc& classTypeDesc, size_t index)
	: m_pPointer(nullptr)
	, m_pTypeDesc(nullptr)
{
	const IBaseTypeDesc* pBaseTypeDesc = GetBaseClassDesc(baseClassTypeId, classTypeDesc);
	if (pBaseTypeDesc)
	{
		const ITypeDesc* pBaseClassTypeDesc = pBaseTypeDesc->GetTypeDesc();
		if (pBaseClassTypeDesc)
		{
			Type::CBaseCastFunction baseCastFunction = pBaseTypeDesc->GetBaseCastFunction();
			if (baseCastFunction)
			{
				void* pBaseClass = baseCastFunction(static_cast<void*>(pClassInstance));
				if (pBaseClass)
				{
					const IVariableDesc* pVariableDesc = pBaseClassTypeDesc->GetVariableByIndex(index);
					if (pVariableDesc)
					{
						m_pPointer = static_cast<void*>(reinterpret_cast<char*>(pBaseClass) + pVariableDesc->GetOffset());
						m_pTypeDesc = pVariableDesc->GetTypeDesc();
						return;
					}
					CRY_ASSERT(pVariableDesc, "Variable with index '%i' not found in class type description.", index);
				}
				CRY_ASSERT(pBaseClass, "Base class cast must be non-null.");
			}
			CRY_ASSERT(baseCastFunction, "Base cast function must be set.");
		}
		CRY_ASSERT(pBaseClassTypeDesc, "Base class type description not found in reflection registry.");
	}
	CRY_ASSERT(pBaseTypeDesc, "Base class type description not found in class description.");
}

inline CMemberVariablePointer::CMemberVariablePointer(void* pClassInstance, const IBaseTypeDesc& baseClassTypeDesc, const ITypeDesc& classTypeDesc, size_t index)
{
	const ITypeDesc* pBaseClassTypeDesc = baseClassTypeDesc.GetTypeDesc();
	if (pBaseClassTypeDesc)
	{
		Type::CBaseCastFunction baseCastFunction = baseClassTypeDesc.GetBaseCastFunction();
		if (baseCastFunction)
		{
			void* pBaseClass = baseCastFunction(static_cast<void*>(pClassInstance));
			if (pBaseClass)
			{
				const IVariableDesc* pVariableDesc = pBaseClassTypeDesc->GetVariableByIndex(index);
				if (pVariableDesc)
				{
					m_pPointer = static_cast<void*>(reinterpret_cast<char*>(pBaseClass) + pVariableDesc->GetOffset());
					m_pTypeDesc = pVariableDesc->GetTypeDesc();
					return;
				}
				CRY_ASSERT(pVariableDesc, "Variable with index '%i' not found in class type description.", index);
			}
			CRY_ASSERT(pBaseClass, "Base class cast must be non-null.");
		}
		CRY_ASSERT(baseCastFunction, "Base cast function must be set.");
	}
	CRY_ASSERT(pBaseClassTypeDesc, "Base class type description not found in reflection registry.");
}

template<typename TYPE>
inline CMemberVariablePointer::CMemberVariablePointer(TYPE* pClassInstance, const IBaseTypeDesc& baseClassTypeDesc, const IVariableDesc& variableDesc)
{
	Type::CBaseCastFunction baseCastFunction = baseClassTypeDesc.GetBaseCastFunction();
	if (baseCastFunction)
	{
		void* pBaseClass = baseCastFunction(static_cast<void*>(pClassInstance));
		if (pBaseClass)
		{
			m_pPointer = static_cast<void*>(reinterpret_cast<char*>(pBaseClass) + variableDesc.GetOffset());
			m_pTypeDesc = variableDesc.GetTypeDesc();
			return;
		}
		CRY_ASSERT(pBaseClass, "Base class cast must be non-null.");
	}
	CRY_ASSERT(baseCastFunction, "Base cast function must be set.");
}

inline CMemberVariablePointer::CMemberVariablePointer(void* pClassInstance, const IBaseTypeDesc& baseClassTypeDesc, const IVariableDesc& variableDesc)
{
	Type::CBaseCastFunction baseCastFunction = baseClassTypeDesc.GetBaseCastFunction();
	if (baseCastFunction)
	{
		void* pBaseClass = baseCastFunction(static_cast<void*>(pClassInstance));
		if (pBaseClass)
		{
			m_pPointer = static_cast<void*>(reinterpret_cast<char*>(pBaseClass) + variableDesc.GetOffset());
			m_pTypeDesc = variableDesc.GetTypeDesc();
			return;
		}
		CRY_ASSERT(pBaseClass, "Base class cast must be non-null.");
	}
	CRY_ASSERT(baseCastFunction, "Base cast function must be set.");
}

inline CAny CMemberVariablePointer::ToAny() const
{
	CAny any;
	any.AssignPointer(m_pPointer, *m_pTypeDesc);
	return any;
}

inline const IBaseTypeDesc* CMemberVariablePointer::GetBaseClassDesc(CTypeId baseTypeId, const ITypeDesc& typeDesc) const
{
	size_t baseClassCount = typeDesc.GetBaseTypeCount();
	for (int i = 0; i < baseClassCount; ++i)
	{
		const IBaseTypeDesc* pBaseClassTypeDesc = typeDesc.GetBaseTypeByIndex(i);
		if (pBaseClassTypeDesc->GetTypeId() == baseTypeId)
		{
			return pBaseClassTypeDesc;
		}
	}
	return nullptr;
}

} // ~Reflection namespace
} // ~Cry namespace
