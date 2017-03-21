// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/IArchive.h>

#include <functional>

struct CryGUID
{
	constexpr static CryGUID Construct(const uint64& hipart, const uint64& lopart)
	{
		return CryGUID{ hipart, lopart };
	}

	constexpr static CryGUID Construct(uint32 d1, uint16 d2, uint16 d3, uint8 d4[8])
	{
		return CryGUID{
			(((uint64)d3) << 48) | (((uint64)d2) << 32) | ((uint64)d1),  //high part
			*((uint64*)d4)  //low part
		 };
	}
	
	constexpr static CryGUID Construct(uint32 d1, uint16 d2, uint16 d3,
		uint8 d4_0, uint8 d4_1, uint8 d4_2, uint8 d4_3, uint8 d4_4, uint8 d4_5, uint8 d4_6, uint8 d4_7)
	{
		return CryGUID{
			(((uint64)d3) << 48) | (((uint64)d2) << 32) | ((uint64)d1),  //high part
			(((uint64)d4_7) << 56) | (((uint64)d4_6) << 48) | (((uint64)d4_5) << 40) | (((uint64)d4_4) << 32) | (((uint64)d4_3) << 24) | (((uint64)d4_2) << 16) | (((uint64)d4_1) << 8) | (uint64)d4_0   //low part
		};
	}

	inline static CryGUID Create();

	constexpr static CryGUID Null()
	{
		return CryGUID::Construct( 0, 0 );
	}

	bool operator==(const CryGUID& rhs) const { return hipart == rhs.hipart && lopart == rhs.lopart; }
	bool operator!=(const CryGUID& rhs) const { return hipart != rhs.hipart || lopart != rhs.lopart; }
	bool operator<(const CryGUID& rhs) const  { return hipart < rhs.hipart || (hipart == rhs.hipart) && lopart < rhs.lopart; }
	bool operator>(const CryGUID& rhs) const  { return hipart > rhs.hipart || (hipart == rhs.hipart) && lopart > rhs.lopart; }
	bool operator<=(const CryGUID& rhs) const { return hipart < rhs.hipart || (hipart == rhs.hipart) && lopart <= rhs.lopart; }
	bool operator>=(const CryGUID& rhs) const { return hipart > rhs.hipart || (hipart == rhs.hipart) && lopart >= rhs.lopart; }

	void Serialize(Serialization::IArchive& ar)
	{
		if (ar.isInput())
		{
			uint32 dwords[4];
			ar(dwords, "guid");
			lopart = (((uint64)dwords[1]) << 32) | (uint64)dwords[0];
			hipart = (((uint64)dwords[3]) << 32) | (uint64)dwords[2];
		}
		else
		{
			uint32 guid[4] = {
				(uint32)(lopart & 0xFFFFFFFF), (uint32)((lopart >> 32) & 0xFFFFFFFF),
				(uint32)(hipart & 0xFFFFFFFF), (uint32)((hipart >> 32) & 0xFFFFFFFF)
			};
			ar(guid, "guid");
		}
	}

	uint64 hipart;
	uint64 lopart;
};

// !!! Do NOT turn CryGUID into a non-aggregate !!!
// It will prevent inlining and type list unrolling opportunities within
// cryinterface_cast<T>() and cryiidof<T>(). As such prevent constructors,
// non-public members, base classes and virtual functions.

static_assert(std::is_pod<CryGUID>(), "CryGUID has to stay a non-aggragte");

namespace std
{
template<> struct hash<CryGUID>
{
public:
	size_t operator()(const CryGUID& guid) const
	{
		std::hash<uint64> hasher;
		return hasher(guid.lopart) ^ hasher(guid.hipart);
	}
};
}

#define MAKE_CRYGUID(high, low) CryGUID::Construct((uint64) high ## LL, (uint64) low ## LL)
