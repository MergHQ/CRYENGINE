// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// #SchematycTODO : Can we compress this type somehow? Perhaps by hashing GUIDs?

#pragma once

#include <CrySerialization/Forward.h>

#include "Schematyc/Utils/Assert.h"
#include "Schematyc/Utils/GUID.h"

namespace Schematyc
{

class CGraphPortId
{
public:

	enum class EType
	{
		Empty,
		Idx,
		UniqueId,
		GUID
	};

public:

	inline CGraphPortId()
		: m_type(EType::Empty)
	{}

	inline CGraphPortId(const CGraphPortId& rhs)
		: m_value(rhs.m_value)
		, m_type(rhs.m_type)
	{}

	inline bool IsEmpty() const
	{
		return m_type == EType::Empty;
	}

	inline EType GetType() const
	{
		return m_type;
	}

	inline uint32 AsIdx() const
	{
		SCHEMATYC_CORE_ASSERT(m_type == EType::Idx);
		return static_cast<uint32>(m_value.hipart);
	}

	inline uint32 AsUniqueId() const
	{
		SCHEMATYC_CORE_ASSERT(m_type == EType::UniqueId);
		return static_cast<uint32>(m_value.hipart);
	}

	inline SGUID AsGUID() const
	{
		SCHEMATYC_CORE_ASSERT(m_type == EType::GUID);
		return m_value;
	}

	inline void Serialize(Serialization::IArchive& archive)
	{
		archive(m_type, "type");

		switch (m_type)
		{
		case EType::Idx:
		case EType::UniqueId:
			{
				uint32 temp = static_cast<uint32>(m_value.hipart);
				archive(temp, "value");
				if (archive.isInput())
				{
					m_value.hipart = temp;
				}
				break;
			}
		case EType::GUID:
			{
				archive(m_value, "value");
				break;
			}
		}
	}

	inline bool operator==(const CGraphPortId& rhs) const
	{
		return (m_type == rhs.m_type) && (m_value == rhs.m_value);
	}

	inline bool operator!=(const CGraphPortId& rhs) const
	{
		return (m_type != rhs.m_type) || (m_value != rhs.m_value);
	}

	inline bool operator<(const CGraphPortId& rhs) const
	{
		return m_type == rhs.m_type ? m_value < rhs.m_value : m_type < rhs.m_type;
	}

	inline bool operator>(const CGraphPortId& rhs) const
	{
		return m_type == rhs.m_type ? m_value > rhs.m_value : m_type > rhs.m_type;
	}

	inline bool operator<=(const CGraphPortId& rhs) const
	{
		return m_type == rhs.m_type ? m_value <= rhs.m_value : m_type <= rhs.m_type;
	}

	inline bool operator>=(const CGraphPortId& rhs) const
	{
		return m_type == rhs.m_type ? m_value >= rhs.m_value : m_type >= rhs.m_type;
	}

	static inline CGraphPortId FromIdx(uint32 value)
	{
		return CGraphPortId(EType::Idx, value);
	}

	static inline CGraphPortId FromUniqueId(uint32 value)
	{
		return CGraphPortId(EType::UniqueId, value);
	}

	static inline CGraphPortId FromGUID(const SGUID& value)
	{
		return CGraphPortId(EType::GUID, value);
	}

private:

	explicit inline CGraphPortId(EType type, uint32 value)
		: m_type(type)
	{
		m_value.hipart = value;
	}

	explicit inline CGraphPortId(EType type, const SGUID& value)
		: m_type(type)
		, m_value(value)
	{}

private:

	EType m_type;
	SGUID m_value;
};

} // Schematyc
