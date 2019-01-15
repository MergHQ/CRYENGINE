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
class CImpl;
class CGlobalObject;
class CListener;
class CEvent;

extern CImpl* g_pImpl;
extern CGlobalObject* g_pObject;
extern CListener* g_pListener;

using AcbHandles = std::map<uint32, CriAtomExAcbHn>;
extern AcbHandles g_acbHandles;

extern CriAtomExPlayerConfig g_playerConfig;
extern CriAtomEx3dSourceConfig g_3dSourceConfig;

extern uint32 g_numObjectsWithDoppler;

static constexpr CriChar8 const* s_szAbsoluteVelocityAisacName = "absolute_velocity";

using Events = std::vector<CEvent*>;

struct S3DAttributes final
{
	S3DAttributes() = default;

	CriAtomExVector pos;
	CriAtomExVector fwd;
	CriAtomExVector up;
	CriAtomExVector vel;
};

//////////////////////////////////////////////////////////////////////////
inline void Fill3DAttributeTransformation(CTransformation const& transformation, S3DAttributes& attributes)
{
	attributes.pos.x = static_cast<CriFloat32>(transformation.GetPosition().x);
	attributes.pos.y = static_cast<CriFloat32>(transformation.GetPosition().z);
	attributes.pos.z = static_cast<CriFloat32>(transformation.GetPosition().y);

	attributes.fwd.x = static_cast<CriFloat32>(transformation.GetForward().x);
	attributes.fwd.y = static_cast<CriFloat32>(transformation.GetForward().z);
	attributes.fwd.z = static_cast<CriFloat32>(transformation.GetForward().y);

	attributes.up.x = static_cast<CriFloat32>(transformation.GetUp().x);
	attributes.up.y = static_cast<CriFloat32>(transformation.GetUp().z);
	attributes.up.z = static_cast<CriFloat32>(transformation.GetUp().y);

	attributes.vel.x = 0.0f;
	attributes.vel.y = 0.0f;
	attributes.vel.z = 0.0f;
}

//////////////////////////////////////////////////////////////////////////
inline void Fill3DAttributeVelocity(Vec3 const& velocity, S3DAttributes& attributes)
{
	attributes.vel.x = static_cast<CriFloat32>(velocity.x);
	attributes.vel.y = static_cast<CriFloat32>(velocity.z);
	attributes.vel.z = static_cast<CriFloat32>(velocity.y);
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
