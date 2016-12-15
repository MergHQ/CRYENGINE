// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryExtension/CryGUID.h>
#include <CrySerialization/Forward.h>
#include <CryString/CryStringUtils.h>

#include "Schematyc/Utils/StackString.h"
#include "Schematyc/Utils/StringUtils.h"

namespace Schematyc
{
// System GUID structure. This type is aggregate and has.
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef GUID_DEFINED
typedef ::GUID SSysGUID;
#else
struct SSysGUID
{
	unsigned long  Data1;
	unsigned short Data2;
	unsigned short Data3;
	unsigned char  Data4[8];
};
#endif

// Non-aggregate GUID structure.
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SGUID : public SSysGUID
{

	inline SGUID()
		: SSysGUID {0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 }}
	{}

	constexpr SGUID(const SSysGUID& rhs)
		: SSysGUID(rhs)
	{}
};

namespace GUID
{
// Read high 64bits of GUID.
////////////////////////////////////////////////////////////////////////////////////////////////////
inline uint64 High(const SGUID& guid)
{
	return reinterpret_cast<const uint64*>(&guid)[0];
}

// Read low 64bits of GUID.
////////////////////////////////////////////////////////////////////////////////////////////////////
inline uint64 Low(const SGUID& guid)
{
	return reinterpret_cast<const uint64*>(&guid)[1];
}
}   // GUID

// GUID comparison operators.
////////////////////////////////////////////////////////////////////////////////////////////////////

inline bool operator==(const SGUID& lhs, const SGUID& rhs)
{
	return (GUID::High(lhs) == GUID::High(rhs)) && (GUID::Low(lhs) == GUID::Low(rhs));
}

inline bool operator!=(const SGUID& lhs, const SGUID& rhs)
{
	return (GUID::High(lhs) != GUID::High(rhs)) || (GUID::Low(lhs) != GUID::Low(rhs));
}

inline bool operator<(const SGUID& lhs, const SGUID& rhs)
{
	return (GUID::High(lhs) < GUID::High(rhs)) || ((GUID::High(lhs) == GUID::High(rhs)) && (GUID::Low(lhs) < GUID::Low(rhs)));
}

inline bool operator>(const SGUID& lhs, const SGUID& rhs)
{
	return (GUID::High(lhs) > GUID::High(rhs)) || ((GUID::High(lhs) == GUID::High(rhs)) && (GUID::Low(lhs) > GUID::Low(rhs)));
}

inline bool operator<=(const SGUID& lhs, const SGUID& rhs)
{
	return (GUID::High(lhs) < GUID::High(rhs)) || ((GUID::High(lhs) == GUID::High(rhs)) && (GUID::Low(lhs) <= GUID::Low(rhs)));
}

inline bool operator>=(const SGUID& lhs, const SGUID& rhs)
{
	return (GUID::High(lhs) > GUID::High(rhs)) || ((GUID::High(lhs) == GUID::High(rhs)) && (GUID::Low(lhs) >= GUID::Low(rhs)));
}

namespace GUID
{
// Create GUID from string.
////////////////////////////////////////////////////////////////////////////////////////////////////
constexpr SGUID FromString(const char* szInput)
{
	return SSysGUID {
					 StringUtils::HexStringToUnsignedLong(szInput),
					 StringUtils::HexStringToUnsignedShort(szInput + 9),
					 StringUtils::HexStringToUnsignedShort(szInput + 14),
					 {
						 StringUtils::HexStringToUnsignedChar(szInput + 19),
						 StringUtils::HexStringToUnsignedChar(szInput + 21),
						 StringUtils::HexStringToUnsignedChar(szInput + 24),
						 StringUtils::HexStringToUnsignedChar(szInput + 26),
						 StringUtils::HexStringToUnsignedChar(szInput + 28),
						 StringUtils::HexStringToUnsignedChar(szInput + 30),
						 StringUtils::HexStringToUnsignedChar(szInput + 32),
						 StringUtils::HexStringToUnsignedChar(szInput + 34)
					 }
	};
}

// Write GUID to string.
////////////////////////////////////////////////////////////////////////////////////////////////////
inline void ToString(IString& output, const SGUID& guid)
{
	unsigned int Data1;
	unsigned int Data2;
	unsigned int Data3;
	unsigned int Data4[8];
	Data1 = static_cast<unsigned int>(guid.Data1);
	Data2 = static_cast<unsigned int>(guid.Data2);
	Data3 = static_cast<unsigned int>(guid.Data3);
	Data4[0] = static_cast<unsigned int>(guid.Data4[0]);
	Data4[1] = static_cast<unsigned int>(guid.Data4[1]);
	Data4[2] = static_cast<unsigned int>(guid.Data4[2]);
	Data4[3] = static_cast<unsigned int>(guid.Data4[3]);
	Data4[4] = static_cast<unsigned int>(guid.Data4[4]);
	Data4[5] = static_cast<unsigned int>(guid.Data4[5]);
	Data4[6] = static_cast<unsigned int>(guid.Data4[6]);
	Data4[7] = static_cast<unsigned int>(guid.Data4[7]);
	output.Format("%.8x-%.4x-%.4x-%.2x%.2x-%.2x%.2x%.2x%.2x%.2x%.2x", Data1, Data2, Data3, Data4[0], Data4[1], Data4[2], Data4[3], Data4[4], Data4[5], Data4[6], Data4[7]);
}

// Get empty GUID.
// SchematycTODO : Replace with constexpr global variable?
////////////////////////////////////////////////////////////////////////////////////////////////////
constexpr SGUID Empty()
{
	return SSysGUID {
					 0, 0, 0, {
						 0, 0, 0, 0, 0, 0, 0, 0
					 }
	};
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
	return hash(High(guid)) ^ hash(Low(guid));
}
}     // GUID

// Serialize GUID.
////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Serialize(Serialization::IArchive& archive, SGUID& value, const char* szName, const char* szLabel)
{
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
}   // std