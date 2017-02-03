// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryExtension/CryGUID.h>
#include <CrySerialization/Forward.h>
#include <CryString/CryStringUtils.h>

#include "Schematyc/Utils/StackString.h"
#include "Schematyc/Utils/StringUtils.h"

namespace Schematyc
{

struct SGUID : public CryGUID  //separate class (for now) because we want a more standard serialize method
{
	inline constexpr SGUID()  //remark: this custom constructor will turn SGUID into a non-aggregate
		: CryGUID(CryGUID::Null())
	{}

	constexpr SGUID(const CryGUID& rhs)
		: CryGUID(rhs)
	{}
};

struct SGUID_mapped
{
	uint32 Data1;
	uint16 Data2;
	uint16 Data3;
	uint8  Data4[8];
};

namespace GUID
{
// Create GUID from string.
////////////////////////////////////////////////////////////////////////////////////////////////////
constexpr SGUID FromString(const char* szInput)
{
	return SGUID::Construct(
	  StringUtils::HexStringToUnsignedLong(szInput),        // uint32 data1
	  StringUtils::HexStringToUnsignedShort(szInput + 9),   // uint16 data2
	  StringUtils::HexStringToUnsignedShort(szInput + 14),  // uint16 data3
	  StringUtils::HexStringToUnsignedChar(szInput + 19),   // uint8  data4[8]
	  StringUtils::HexStringToUnsignedChar(szInput + 21),
	  StringUtils::HexStringToUnsignedChar(szInput + 24),
	  StringUtils::HexStringToUnsignedChar(szInput + 26),
	  StringUtils::HexStringToUnsignedChar(szInput + 28),
	  StringUtils::HexStringToUnsignedChar(szInput + 30),
	  StringUtils::HexStringToUnsignedChar(szInput + 32),
	  StringUtils::HexStringToUnsignedChar(szInput + 34)
	  );
};

// Write GUID to string.
////////////////////////////////////////////////////////////////////////////////////////////////////
inline void ToString(IString& output, const SGUID& guid)
{
	static_assert(sizeof(SGUID_mapped) == sizeof(SGUID), "SGUID_mapped and SGUID must be the same size");

	const SGUID_mapped* pMappedGUID = alias_cast<SGUID_mapped*>(&guid);

	unsigned int Data1;
	unsigned int Data2;
	unsigned int Data3;
	unsigned int Data4[8];

	Data1 = static_cast<unsigned int>(pMappedGUID->Data1);
	Data2 = static_cast<unsigned int>(pMappedGUID->Data2);
	Data3 = static_cast<unsigned int>(pMappedGUID->Data3);

	Data4[0] = static_cast<unsigned int>(pMappedGUID->Data4[0]);
	Data4[1] = static_cast<unsigned int>(pMappedGUID->Data4[1]);
	Data4[2] = static_cast<unsigned int>(pMappedGUID->Data4[2]);
	Data4[3] = static_cast<unsigned int>(pMappedGUID->Data4[3]);
	Data4[4] = static_cast<unsigned int>(pMappedGUID->Data4[4]);
	Data4[5] = static_cast<unsigned int>(pMappedGUID->Data4[5]);
	Data4[6] = static_cast<unsigned int>(pMappedGUID->Data4[6]);
	Data4[7] = static_cast<unsigned int>(pMappedGUID->Data4[7]);

	output.Format("%.8x-%.4x-%.4x-%.2x%.2x-%.2x%.2x%.2x%.2x%.2x%.2x", Data1, Data2, Data3, Data4[0], Data4[1], Data4[2], Data4[3], Data4[4], Data4[5], Data4[6], Data4[7]);
}

// Get empty GUID.
// SchematycTODO : Replace with constexpr global variable?
////////////////////////////////////////////////////////////////////////////////////////////////////
constexpr SGUID Empty()
{
	return SGUID(CryGUID::Null());
}

// Is GUID empty?
////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool IsEmpty(const SGUID& guid)
{
	return guid == Empty();
}

// Hash GUID.
////////////////////////////////////////////////////////////////////////////////////////////////////
inline size_t Hash(const SGUID& guid)
{
	std::hash<uint64> hash;
	return hash(guid.hipart) ^ hash(guid.lopart);
}

} // GUID

// Serialize GUID.
////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Serialize(Serialization::IArchive& archive, SGUID& value, const char* szName, const char* szLabel)
{
	// SchematycTODO: Optimized version for binary archives? just save the uint64s there
	CStackString temp;
	if (archive.isInput())
	{
		archive(temp, szName, szLabel);
		if (!temp.empty())
		{
			value = GUID::FromString(temp.c_str());
		}
		else
		{
			value = GUID::Empty();
			return false;
		}
	}
	else if (archive.isOutput())
	{
		if (!GUID::IsEmpty(value))
		{
			GUID::ToString(temp, value);
		}
		archive(temp, szName, szLabel);
	}
	return true;
}

} // Schematyc

// GUID string literal.
////////////////////////////////////////////////////////////////////////////////////////////////////
constexpr Schematyc::SGUID operator"" _schematyc_guid(const char* szInput, size_t)
{
	return Schematyc::GUID::FromString(szInput);
}

// Specialize std::hash for GUID.
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace std
{
template<> struct hash<Schematyc::SGUID>
{
	inline size_t operator()(const Schematyc::SGUID& value) const
	{
		return Schematyc::GUID::Hash(value);
	}
};
} // std
