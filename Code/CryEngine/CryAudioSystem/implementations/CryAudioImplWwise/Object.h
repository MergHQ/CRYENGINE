// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IObject.h>
#include <PoolObject.h>
#include <AK/SoundEngine/Common/AkTypes.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
class CEvent;

enum class EObjectFlags : EnumFlagsType
{
	None                  = 0,
	MovingOrDecaying      = BIT(0),
	TrackAbsoluteVelocity = BIT(1),
	TrackRelativeVelocity = BIT(2),
	UpdateVirtualStates   = BIT(3),
};
CRY_CREATE_ENUM_FLAG_OPERATORS(EObjectFlags);

class CObject final : public IObject, public CPoolObject<CObject, stl::PSyncNone>
{
public:

	CObject() = delete;
	CObject(CObject const&) = delete;
	CObject(CObject&&) = delete;
	CObject& operator=(CObject const&) = delete;
	CObject& operator=(CObject&&) = delete;

	explicit CObject(AkGameObjectID const id, CTransformation const& transformation, char const* const szName);
	virtual ~CObject() override;

	// CryAudio::Impl::IObject
	virtual void                   Update(float const deltaTime) override;
	virtual void                   SetTransformation(CTransformation const& transformation) override;
	virtual CTransformation const& GetTransformation() const override { return m_transformation; }
	virtual void                   SetEnvironment(IEnvironmentConnection const* const pIEnvironmentConnection, float const amount) override;
	virtual void                   SetParameter(IParameterConnection const* const pIParameterConnection, float const value) override;
	virtual void                   SetSwitchState(ISwitchStateConnection const* const pISwitchStateConnection) override;
	virtual void                   SetOcclusion(float const occlusion) override;
	virtual void                   SetOcclusionType(EOcclusionType const occlusionType) override;
	virtual ERequestStatus         ExecuteTrigger(ITriggerConnection const* const pITriggerConnection, IEvent* const pIEvent) override;
	virtual void                   StopAllTriggers() override;
	virtual ERequestStatus         PlayFile(IStandaloneFileConnection* const pIStandaloneFileConnection) override;
	virtual ERequestStatus         StopFile(IStandaloneFileConnection* const pIStandaloneFileConnection) override;
	virtual ERequestStatus         SetName(char const* const szName) override;
	virtual void                   ToggleFunctionality(EObjectFunctionality const type, bool const enable) override;

	// Below data is only used when INCLUDE_WWISE_IMPL_PRODUCTION_CODE is defined!
	virtual void DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter) override;
	// ~CryAudio::Impl::IObject

	void RemoveEvent(CEvent* const pEvent);

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	char const* GetName() const { return m_name.c_str(); }
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

	AkGameObjectID const m_id;

private:

	void PostEnvironmentAmounts();
	void UpdateVelocities(float const deltaTime);
	void TryToSetRelativeVelocity(float const relativeVelocity);

	using AuxSendValues = std::vector<AkAuxSendValue>;

	std::vector<CEvent*> m_events;
	bool                 m_needsToUpdateEnvironments;
	bool                 m_needsToUpdateVirtualStates;
	AuxSendValues        m_auxSendValues;

	EObjectFlags         m_flags;
	float                m_distanceToListener;
	float                m_previousRelativeVelocity;
	float                m_previousAbsoluteVelocity;
	CTransformation      m_transformation;
	Vec3                 m_position;
	Vec3                 m_previousPosition;
	Vec3                 m_velocity;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxObjectNameLength> m_name;
	std::map<char const* const, float>   m_parameterInfo;
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
