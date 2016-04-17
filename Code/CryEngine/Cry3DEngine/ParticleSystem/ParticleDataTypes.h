// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     24/09/2014 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef PARTICLEDATATYPES_H
#define PARTICLEDATATYPES_H

#pragma once

#include <CryMath/FinalizingSpline.h>
#include <CrySerialization/Enum.h>
#include "ParticleCommon.h"
#include "ParticleMath.h"

namespace pfx2
{

struct SChaosKey
{
public:
	SChaosKey() : m_key(cry_random_uint32()) {}
	explicit SChaosKey(uint32 key) : m_key(key) {}
	explicit SChaosKey(float key) : m_key((uint32)(key * float(0xFFFFFFFF))) {}
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
	SChaosKeyV();
	explicit SChaosKeyV(uint32v keys) : m_keys(keys) {}
	explicit SChaosKeyV(floatv keys) : m_keys(ToUint32v(Mul(keys, ToFloatv(0xFFFFFFFF)))) {}
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

#define PFX2_DATA_TYPES(f)                                                                                \
  f(ParentId, TParticleId)                                                                                \
  f(SpawnId, uint32) f(SpawnFraction, float)                                                              \
  f(State, uint8)                                                                                         \
  f(LifeTime, float)                                                                                      \
  f(InvLifeTime, float)                                                                                   \
  f(NormalAge, float)                                                                                     \
  f(PositionX, float)        f(PositionY, float)        f(PositionZ, float)                               \
  f(AuxPositionX, float)     f(AuxPositionY, float)     f(AuxPositionZ, float)                            \
  f(VelocityX, float)        f(VelocityY, float)        f(VelocityZ, float)                               \
  f(AngularVelocityX, float) f(AngularVelocityY, float) f(AngularVelocityZ, float)                        \
  f(VelocityFieldX, float)   f(VelocityFieldY, float)   f(VelocityFieldZ, float)                          \
  f(AccelerationX, float)    f(AccelerationY, float)    f(AccelerationZ, float)                           \
  f(OrientationX, float)     f(OrientationY, float)     f(OrientationZ, float)     f(OrientationW, float) \
  f(Tile, uint8)                                                                                          \
  f(Color, UCol)  f(ColorInit, UCol)                                                                      \
  f(Size, float) f(SizeInit, float)                                                                       \
  f(Alpha, float) f(AlphaInit, float)                                                                     \
  f(Gravity, float) f(GravityInit, float)                                                                 \
  f(Drag, float) f(DragInit, float)                                                                       \
  f(Angle2D, float) f(Spin2D, float)                                                                      \
  f(UNormRand, float)                                                                                     \
  f(RibbonId, uint32)                                                                                     \
  f(PhysicalEntity, IPhysicalEntity*)                                                                     \
  f(MeshGeometry, IMeshObj*)                                                                              \
  f(AudioProxy, IAudioProxy*)

#define PFX2_FLOAT_FIELDS(f) \
  f(Size)                    \
  f(Alpha)                   \
  f(Gravity)                 \
  f(Drag)

enum EParticleDataType
{
#define PFX2_DATA_ENUM(NAME, TYPE) EPDT_ ## NAME,
	PFX2_DATA_TYPES(PFX2_DATA_ENUM)
	EPDT_Count,
};

enum EParticleFloatField
{
#define PFX2_FLOAT_FIELD_ENUM(NAME) EPFF_ ## NAME,
	PFX2_FLOAT_FIELDS(PFX2_FLOAT_FIELD_ENUM)
	EPFF_Count
};

enum EParticleVec3Field
{
	EPVF_Position        = EPDT_PositionX,
	EPVF_AuxPosition     = EPDT_AuxPositionX,
	EPVF_Velocity        = EPDT_VelocityX,
	EPVF_AngularVelocity = EPDT_AngularVelocityX,
	EPVF_VelocityField   = EPDT_VelocityFieldX, // PFX2_TODO : remove velocity field and acceleration, they are not supposed to be persistent
	EPVF_Acceleration    = EPDT_AccelerationX,
};

enum EParticleQuatField
{
	EPQF_Orientation = EPDT_OrientationX,
};

const size_t gParticleDataStrides[EPDT_Count] =
{
#define PFX2_DATA_STRIDE(NAME, TYPE) sizeof(TYPE),
	PFX2_DATA_TYPES(PFX2_DATA_STRIDE)
};

struct SFieldToData
{
	EParticleDataType m_data;
	EParticleDataType m_init;
};

const SFieldToData gFieldToData[EPFF_Count] =
{
#define PFX2_FLOAT_FIELD_TO_DATA(NAME) { EPDT_ ## NAME, EPDT_ ## NAME ## Init \
  },
	PFX2_FLOAT_FIELDS(PFX2_FLOAT_FIELD_TO_DATA)
};

}

#include "ParticleDataTypesImpl.h"

#endif // PARTICLEDATATYPES_H
