// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include <IObject.h>
#include <PoolObject.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CEvent;
class CBaseParameter;
class CBaseSwitchState;
class CEnvironment;
class CBaseStandaloneFile;

enum class EObjectFlags : EnumFlagsType
{
	None                    = 0,
	MovingOrDecaying        = BIT(0),
	TrackAbsoluteVelocity   = BIT(1),
	TrackVelocityForDoppler = BIT(2),
	UpdateVirtualStates     = BIT(3),
	IsVirtual               = BIT(4),
};
CRY_CREATE_ENUM_FLAG_OPERATORS(EObjectFlags);

class CBaseObject : public IObject
{
public:

	CBaseObject(CBaseObject const&) = delete;
	CBaseObject(CBaseObject&&) = delete;
	CBaseObject& operator=(CBaseObject const&) = delete;
	CBaseObject& operator=(CBaseObject&&) = delete;

	virtual ~CBaseObject() override = default;

	// CryAudio::Impl::IObject
	virtual void                   Update(float const deltaTime) override;
	virtual void                   SetTransformation(CTransformation const& transformation) override {}
	virtual CTransformation const& GetTransformation() const override                                { return CTransformation::GetEmptyObject(); }
	virtual void                   StopAllTriggers() override;
	virtual ERequestStatus         SetName(char const* const szName) override;

	// Below data is only used when INCLUDE_FMOD_IMPL_PRODUCTION_CODE is defined!
	virtual void DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter) override {}
	// ~CryAudio::Impl::IObject

	Events const&       GetEvents() const  { return m_events; }
	Parameters&         GetParameters()    { return m_parameters; }
	Switches&           GetSwitches()      { return m_switches; }
	Environments&       GetEnvironments()  { return m_environments; }
	Events&             GetPendingEvents() { return m_pendingEvents; }
	StandaloneFiles&    GetPendingFiles()  { return m_pendingFiles; }
	FMOD_3D_ATTRIBUTES& GetAttributes()    { return m_attributes; }

	void                StopEvent(uint32 const id);
	void                RemoveParameter(CBaseParameter const* const pParameter);
	void                RemoveSwitch(CBaseSwitchState const* const pSwitch);
	void                RemoveEnvironment(CEnvironment const* const pEnvironment);
	void                RemoveFile(CBaseStandaloneFile const* const pStandaloneFile);

	static FMOD::Studio::System* s_pSystem;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	char const* GetName() const { return m_name.c_str(); }
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

protected:

	CBaseObject();

	bool SetEvent(CEvent* const pEvent);
	void UpdateVelocityTracking();

	EObjectFlags       m_flags;

	Events             m_events;
	StandaloneFiles    m_files;
	Parameters         m_parameters;
	Switches           m_switches;
	Environments       m_environments;

	Events             m_pendingEvents;
	StandaloneFiles    m_pendingFiles;

	FMOD_3D_ATTRIBUTES m_attributes;
	float              m_occlusion = 0.0f;
	float              m_absoluteVelocity = 0.0f;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxObjectNameLength> m_name;
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
