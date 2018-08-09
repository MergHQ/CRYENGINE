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
using Objects = std::vector<CBaseObject*>;

using AcbHandles = std::map<uint32, CriAtomExAcbHn>;
extern AcbHandles g_acbHandles;

extern CriAtomEx3dListenerHn g_pListener;

extern CriAtomExPlayerConfig g_playerConfig;
extern CriAtomEx3dSourceConfig g_3dSourceConfig;

struct STransformation final
{
	STransformation() = default;

	CriAtomExVector pos;
	CriAtomExVector fwd;
	CriAtomExVector up;
};

//////////////////////////////////////////////////////////////////////////
inline void TranslateTransformation(CObjectTransformation const& inTransformation, STransformation& outTransformation)
{
	outTransformation.pos.x = static_cast<CriFloat32>(inTransformation.GetPosition().x);
	outTransformation.pos.z = static_cast<CriFloat32>(inTransformation.GetPosition().y);
	outTransformation.pos.y = static_cast<CriFloat32>(inTransformation.GetPosition().z);

	outTransformation.fwd.x = static_cast<CriFloat32>(inTransformation.GetForward().x);
	outTransformation.fwd.z = static_cast<CriFloat32>(inTransformation.GetForward().y);
	outTransformation.fwd.y = static_cast<CriFloat32>(inTransformation.GetForward().z);

	outTransformation.up.x = static_cast<CriFloat32>(inTransformation.GetUp().x);
	outTransformation.up.z = static_cast<CriFloat32>(inTransformation.GetUp().y);
	outTransformation.up.y = static_cast<CriFloat32>(inTransformation.GetUp().z);
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
