// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved. 

// -------------------------------------------------------------------------
//  Created:     24/09/2014 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include <CryMath/FinalizingSpline.h>
#include <CrySerialization/Enum.h>
#include <CryCore/DynamicEnum.h>
#include "ParticleCommon.h"
#include "ParticleMath.h"

namespace pfx2
{

struct SChaosKey
{
public:
	explicit SChaosKey(uint32 key) : m_key(key) {}
	SChaosKey(const SChaosKey& key) : m_key(key.m_key) {}
	SChaosKey(SChaosKey key1, SChaosKey key2);
	SChaosKey(SChaosKey key1, SChaosKey key2, SChaosKey key3);

	uint32 Rand();
	uint32 Rand(uint32 range);
	float  RandUNorm();
	float  RandSNorm();

	struct Range
	{
		float scale, bias;
		Range(float lo, float hi);
	};
	float Rand(Range range);

	Vec2  RandCircle();
	Vec2  RandDisc();
	Vec3  RandSphere();

private:
	uint32 m_key;
};

#ifdef CRY_PFX2_USE_SSE

struct SChaosKeyV
{
	explicit SChaosKeyV(SChaosKey key);
	explicit SChaosKeyV(uint32 key);
	explicit SChaosKeyV(uint32v keys) : m_keys(keys) {}
	uint32v Rand();
	uint32v Rand(uint32 range);
	floatv  RandUNorm();
	floatv  RandSNorm();

	struct Range
	{
		floatv scale, bias;
		Range(float lo, float hi);
	};
	floatv Rand(Range range);

private:
	uint32v m_keys;
};

#else

typedef SChaosKey SChaosKeyV;

#endif

enum EState
{
	ESB_Alive   = BIT(0),   // this particle is alive
	ESB_Dead    = BIT(1),   // this particle is either dead or expired
	ESB_NewBorn = BIT(2),   // this particle appeared this frame
	ESB_Update  = BIT(7),   // this particle can be updated

	ES_Empty    = 0,                                    // nobody home
	ES_Dead     = ESB_Dead,                             // particle allocated but dead
	ES_Alive    = ESB_Update | ESB_Alive,               // regular living particle
	ES_Expired  = ESB_Update | ESB_Dead,                // passed the age time but will only be dead next frame
	ES_NewBorn  = ESB_Update | ESB_NewBorn | ESB_Alive, // particle appeared this frame
};

///////////////////////////////////////////////////////////////////////////////
// Implement EParticleDataTypes (size, position, etc) with DynamicEnum.
// Data type entries can be declared in multiple source files, with info about the data type (float, Vec3, etc)

enum EDataFlags
{
	BHasInit    = 1, // indicates data type has an extra init field
	BNeedsClear = 2  // indicates data type requires clearing after editing
};

//! Info associated with each EParticleDataType
struct SDataInfo
{
	typedef yasli::TypeID TypeID;

	TypeID     type;
	size_t     typeSize   = 0;
	uint       dimension  = 0;
	bool       hasInit    = false;
	bool       needsClear = false;

	SDataInfo() {}

	template<typename T>
	SDataInfo(T*, uint dim = 1, EDataFlags flags = EDataFlags(0))
		: type(TypeID::get<T>())
		, typeSize(sizeof(T)), dimension(dim)
		, hasInit(!!(flags & BHasInit)), needsClear(!!(flags & BNeedsClear))
	{}

	template<typename T>
	bool isType() const         { return type == TypeID::get<T>(); }

	template<typename T>
	bool isType(uint dim) const { return type == TypeID::get<T>() && dimension == dim; }

	uint step() const           { return dimension * (1 + hasInit); }
};

//! EParticleDataType implemented as a DynamicEnum, with SDataInfo
typedef DynamicEnum<SDataInfo, uint, SDataInfo>
  EParticleDataType;

ILINE EParticleDataType InitType(EParticleDataType type)
{
	return type + type.info().dimension;
}

// Convenient initialization of EParticleDataType
//  EParticleDataType PDT(EPDT_SpawnID, TParticleID)
//  EParticleDataType PDT(EPVF_Velocity, float[3])

inline EDataFlags PDT_FLAGS(EDataFlags flags = EDataFlags(0)) { return flags; } // Helper function for variadic macro

#define PDT(Name, T, ...) \
  Name(SkipPrefix(#Name), 0, SDataInfo((typename std::remove_extent<T>::type*)0, std::max<uint>(std::extent<T>::value, 1), PDT_FLAGS(__VA_ARGS__)))

inline cstr SkipPrefix(cstr name)
{
	for (int i = 0; name[i]; ++i)
		if (name[i] == '_')
			return name + i + 1;
	return name;
}

// Standard data types
extern EParticleDataType
  EPDT_SpawnId,
  EPDT_ParentId,
  EPDT_State,
  EPDT_NormalAge,
  EPDT_LifeTime,
  EPDT_InvLifeTime,
  EPDT_SpawnFraction,
  EPDT_Random,
  EPDT_Size,
  EPDT_Angle2D,
  EPDT_Spin2D;

// pose data types
extern EParticleDataType
  EPVF_Position,
  EPVF_Velocity,
  EPQF_Orientation,
  EPVF_AngularVelocity,
  EPVF_LocalPosition,
  EPVF_LocalVelocity,
  EPQF_LocalOrientation;

}

#include "ParticleDataTypesImpl.h"
