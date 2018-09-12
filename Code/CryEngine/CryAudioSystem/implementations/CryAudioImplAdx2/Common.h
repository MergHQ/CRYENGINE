// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioSystem.h>

#include <cri_atom_ex.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
class CBaseObject;
class CListener;

extern CListener* g_pListener;

using Objects = std::vector<CBaseObject*>;

using AcbHandles = std::map<uint32, CriAtomExAcbHn>;
extern AcbHandles g_acbHandles;

extern CriAtomExPlayerConfig g_playerConfig;
extern CriAtomEx3dSourceConfig g_3dSourceConfig;

extern uint32 g_numObjectsWithDoppler;

static constexpr CriChar8 const* s_szAbsoluteVelocityAisacName = "absolute_velocity";

struct S3DAttributes final
{
	S3DAttributes() = default;

	CriAtomExVector pos;
	CriAtomExVector fwd;
	CriAtomExVector up;
	CriAtomExVector vel;
};

//////////////////////////////////////////////////////////////////////////
inline void Fill3DAttributeTransformation(CObjectTransformation const& inTransformation, S3DAttributes& outAttributes)
{
	outAttributes.pos.x = static_cast<CriFloat32>(inTransformation.GetPosition().x);
	outAttributes.pos.y = static_cast<CriFloat32>(inTransformation.GetPosition().z);
	outAttributes.pos.z = static_cast<CriFloat32>(inTransformation.GetPosition().y);

	outAttributes.fwd.x = static_cast<CriFloat32>(inTransformation.GetForward().x);
	outAttributes.fwd.y = static_cast<CriFloat32>(inTransformation.GetForward().z);
	outAttributes.fwd.z = static_cast<CriFloat32>(inTransformation.GetForward().y);

	outAttributes.up.x = static_cast<CriFloat32>(inTransformation.GetUp().x);
	outAttributes.up.y = static_cast<CriFloat32>(inTransformation.GetUp().z);
	outAttributes.up.z = static_cast<CriFloat32>(inTransformation.GetUp().y);

	outAttributes.vel.x = 0.0f;
	outAttributes.vel.y = 0.0f;
	outAttributes.vel.z = 0.0f;
}

//////////////////////////////////////////////////////////////////////////
inline void Fill3DAttributeVelocity(Vec3 const& inVelocity, S3DAttributes& outAttributes)
{
	outAttributes.vel.x = static_cast<CriFloat32>(inVelocity.x);
	outAttributes.vel.y = static_cast<CriFloat32>(inVelocity.z);
	outAttributes.vel.z = static_cast<CriFloat32>(inVelocity.y);
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
