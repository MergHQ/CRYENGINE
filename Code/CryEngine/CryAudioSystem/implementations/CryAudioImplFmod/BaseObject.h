// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IObject.h>
#include <SharedData.h>
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
};
CRY_CREATE_ENUM_FLAG_OPERATORS(EObjectFlags);

class CBaseObject : public IObject
{
public:

	CBaseObject(CBaseObject const&) = delete;
	CBaseObject(CBaseObject&&) = delete;
	CBaseObject& operator=(CBaseObject const&) = delete;
	CBaseObject& operator=(CBaseObject&&) = delete;

	CBaseObject();
	virtual ~CBaseObject() override;

	void RemoveEvent(CEvent* const pEvent);
	void RemoveParameter(CBaseParameter const* const pParameter);
	void RemoveSwitch(CBaseSwitchState const* const pSwitch);
	void RemoveEnvironment(CEnvironment const* const pEnvironment);
	void RemoveFile(CBaseStandaloneFile const* const pStandaloneFile);

	// CryAudio::Impl::IObject
	virtual void                   Update(float const deltaTime) override;
	virtual void                   SetTransformation(CTransformation const& transformation) override {}
	virtual CTransformation const& GetTransformation() const override                                { return CTransformation::GetEmptyObject(); }
	virtual void                   SetParameter(IParameterConnection const* const pIParameterConnection, float const value) override;
	virtual void                   SetSwitchState(ISwitchStateConnection const* const pISwitchStateConnection) override;
	virtual ERequestStatus         ExecuteTrigger(ITriggerConnection const* const pITriggerConnection, IEvent* const pIEvent) override;
	virtual void                   StopAllTriggers() override;
	virtual ERequestStatus         PlayFile(IStandaloneFileConnection* const pIStandaloneFileConnection) override;
	virtual ERequestStatus         StopFile(IStandaloneFileConnection* const pIStandaloneFileConnection) override;
	virtual ERequestStatus         SetName(char const* const szName) override;

	// Below data is only used when INCLUDE_FMOD_IMPL_PRODUCTION_CODE is defined!
	virtual void DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter) override {}
	// ~CryAudio::Impl::IObject

	static FMOD::Studio::System* s_pSystem;

protected:

	void StopEvent(uint32 const id);
	bool SetEvent(CEvent* const pEvent);

	void UpdateVelocityTracking();

	EObjectFlags                                    m_flags;

	std::vector<CEvent*>                            m_events;
	std::vector<CBaseStandaloneFile*>               m_files;
	std::map<CBaseParameter const* const, float>    m_parameters;
	std::map<uint32 const, CBaseSwitchState const*> m_switches;
	std::map<CEnvironment const* const, float>      m_environments;

	std::vector<CEvent*>                            m_pendingEvents;
	std::vector<CBaseStandaloneFile*>               m_pendingFiles;

	FMOD_3D_ATTRIBUTES                              m_attributes;
	float m_occlusion = 0.0f;
	float m_absoluteVelocity = 0.0f;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
