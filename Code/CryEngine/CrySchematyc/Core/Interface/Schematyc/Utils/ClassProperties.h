// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryCore/Containers/CryArray.h>

#include <Schematyc/Reflection/TypeDesc.h>
#include <Schematyc/Utils/Scratchpad.h>

namespace Schematyc
{

class CClassProperties
{
private:

	typedef DynArray<uint32> Properties;

public:

	inline CClassProperties() {}

	inline CClassProperties(const CClassProperties& rhs)
		: m_pDesc(rhs.m_pDesc)
		, m_properties(rhs.m_properties)
		, m_scratchpad(rhs.m_scratchpad)
	{}

	inline bool IsEmpty() const
	{
		return m_properties.empty();
	}

	inline bool Set(const CClassDesc& desc)
	{
		Clear();

		m_pDesc = &desc;

		const ClassMemberDescs& memberDescs = desc.GetMembers();
		m_properties.reserve(memberDescs.size());
		for (const CClassMemberDesc& memberDesc : memberDescs)
		{
			const void* pDefaultValue = memberDesc.GetDefaultValue();
			if (!pDefaultValue)
			{
				Clear();
				return false;
			}

			const uint32 pos = m_scratchpad.Add(CAnyConstRef(memberDesc.GetTypeDesc(), pDefaultValue));
			m_properties.emplace_back(pos);
		}

		return true;
	}

	inline void Clear()
	{
		m_properties.clear();
		m_scratchpad.Clear();
		m_pDesc = nullptr;
	}

	inline bool Apply(const CClassDesc& desc, void* pValue) const
	{
		if (!m_pDesc || (desc != *m_pDesc))
		{
			return false;
		}

		const ClassMemberDescs& memberDescs = desc.GetMembers();
		for (uint32 propertyIdx = 0, propertyCount = m_properties.size(); propertyIdx < propertyCount; ++propertyIdx)
		{
			const CClassMemberDesc& memberDesc = memberDescs[propertyIdx];
			Any::CopyAssign(CAnyRef(memberDesc.GetTypeDesc(), static_cast<uint8*>(pValue) + memberDesc.GetOffset()), *m_scratchpad.Get(m_properties[propertyIdx]));
		}
		return true;
	}

	inline void Serialize(Serialization::IArchive& archive)
	{
		if (m_pDesc)
		{
			const ClassMemberDescs& memberDescs = m_pDesc->GetMembers();
			for (uint32 propertyIdx = 0, propertyCount = m_properties.size(); propertyIdx < propertyCount; ++propertyIdx)
			{
				const CClassMemberDesc& memberDesc = memberDescs[propertyIdx];
				archive(*m_scratchpad.Get(m_properties[propertyIdx]), memberDesc.GetName(), memberDesc.GetLabel());
				const char* szDescription = memberDesc.GetDescription();
				if (szDescription && (szDescription[0] != '\0'))
				{
					archive.doc(szDescription);
				}
			}
		}
	}

private:

	const CClassDesc* m_pDesc = nullptr;
	HeapScratchpad    m_scratchpad;
	Properties        m_properties;
};

} // Schematyc
