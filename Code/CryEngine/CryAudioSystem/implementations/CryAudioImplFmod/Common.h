// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>
#include <fmod_common.h>

#define FMOD_IMPL_INFO_STRING "Fmod Studio - "

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	#define ASSERT_FMOD_OK                    CRY_ASSERT(fmodResult == FMOD_OK)
	#define ASSERT_FMOD_OK_OR_INVALID_HANDLE  CRY_ASSERT(fmodResult == FMOD_OK || fmodResult == FMOD_ERR_INVALID_HANDLE)
	#define ASSERT_FMOD_OK_OR_NOT_LOADED      CRY_ASSERT(fmodResult == FMOD_OK || fmodResult == FMOD_ERR_STUDIO_NOT_LOADED)
	#define ASSERT_FMOD_OK_OR_EVENT_NOT_FOUND CRY_ASSERT(fmodResult == FMOD_OK || fmodResult == FMOD_ERR_EVENT_NOTFOUND)
#else
	#define ASSERT_FMOD_OK                    (void)fmodResult
	#define ASSERT_FMOD_OK_OR_INVALID_HANDLE  (void)fmodResult
	#define ASSERT_FMOD_OK_OR_NOT_LOADED      (void)fmodResult
	#define ASSERT_FMOD_OK_OR_EVENT_NOT_FOUND (void)fmodResult
#endif // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

#define FMOD_IMPL_INVALID_INDEX (-1)

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CImpl;
class CBaseObject;
class CEvent;
class CBaseParameter;
class CBaseStandaloneFile;
class CBaseSwitchState;
class CEnvironment;
class CListener;
class CTrigger;
class CGlobalObject;

extern CImpl* g_pImpl;
extern CGlobalObject* g_pObject;
extern CListener* g_pListener;
extern uint32 g_numObjectsWithDoppler;

static constexpr char const* s_szOcclusionParameterName = "occlusion";
static constexpr char const* s_szAbsoluteVelocityParameterName = "absolute_velocity";

using Objects = std::vector<CBaseObject*>;
using Events = std::vector<CEvent*>;
using Parameters = std::map<CBaseParameter const* const, float>;
using Switches = std::map<uint32 const, CBaseSwitchState const*>;
using Environments = std::map<CEnvironment const* const, float>;
using StandaloneFiles = std::vector<CBaseStandaloneFile*>;

using ParameterIdToIndex = std::map<uint32, int>;
using TriggerToParameterIndexes = std::map<CTrigger const* const, ParameterIdToIndex>;

extern Objects g_constructedObjects;
extern TriggerToParameterIndexes g_triggerToParameterIndexes;

///////////////////////////////////////////////////////////////////////////
inline void Fill3DAttributeTransformation(CTransformation const& transformation, FMOD_3D_ATTRIBUTES& attributes)
{
	attributes.forward.x = transformation.GetForward().x;
	attributes.forward.z = transformation.GetForward().y;
	attributes.forward.y = transformation.GetForward().z;

	attributes.position.x = transformation.GetPosition().x;
	attributes.position.z = transformation.GetPosition().y;
	attributes.position.y = transformation.GetPosition().z;

	attributes.up.x = transformation.GetUp().x;
	attributes.up.z = transformation.GetUp().y;
	attributes.up.y = transformation.GetUp().z;

	attributes.velocity.x = 0.0f;
	attributes.velocity.z = 0.0f;
	attributes.velocity.y = 0.0f;
}

///////////////////////////////////////////////////////////////////////////
inline void Fill3DAttributeVelocity(Vec3 const& velocity, FMOD_3D_ATTRIBUTES& attributes)
{
	attributes.velocity.x = velocity.x;
	attributes.velocity.z = velocity.y;
	attributes.velocity.y = velocity.z;
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
