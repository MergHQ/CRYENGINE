// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>
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

	explicit CObject(AkGameObjectID const id, CObjectTransformation const& transformation);
	virtual ~CObject() override;

	// CryAudio::Impl::IObject
	virtual void                         Update(float const deltaTime) override;
	virtual void                         SetTransformation(CObjectTransformation const& transformation) override;
	virtual CObjectTransformation const& GetTransformation() const override { return m_transformation; }
	virtual void                         SetEnvironment(IEnvironment const* const pIEnvironment, float const amount) override;
	virtual void                         SetParameter(IParameter const* const pIParameter, float const value) override;
	virtual void                         SetSwitchState(ISwitchState const* const pISwitchState) override;
	virtual void                         SetObstructionOcclusion(float const obstruction, float const occlusion) override;
	virtual void                         SetOcclusionType(EOcclusionType const occlusionType) override;
	virtual ERequestStatus               ExecuteTrigger(ITrigger const* const pITrigger, IEvent* const pIEvent) override;
	virtual void                         StopAllTriggers() override;
	virtual ERequestStatus               PlayFile(IStandaloneFile* const pIStandaloneFile) override;
	virtual ERequestStatus               StopFile(IStandaloneFile* const pIStandaloneFile) override;
	virtual ERequestStatus               SetName(char const* const szName) override;
	virtual void                         ToggleFunctionality(EObjectFunctionality const type, bool const enable) override;

	// Below data is only used when INCLUDE_WWISE_IMPL_PRODUCTION_CODE is defined!
	virtual void DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter) override;
	// ~CryAudio::Impl::IObject

	void RemoveEvent(CEvent* const pEvent);

	AkGameObjectID const m_id;

private:

	void PostEnvironmentAmounts();
	void UpdateVelocities(float const deltaTime);
	void TryToSetRelativeVelocity(float const relativeVelocity);

	using AuxSendValues = std::vector<AkAuxSendValue>;

	std::vector<CEvent*>  m_events;
	bool                  m_needsToUpdateEnvironments;
	bool                  m_needsToUpdateVirtualStates;
	AuxSendValues         m_auxSendValues;

	EObjectFlags          m_flags;
	float                 m_distanceToListener;
	float                 m_previousRelativeVelocity;
	float                 m_previousAbsoluteVelocity;
	CObjectTransformation m_transformation;
	Vec3                  m_position;
	Vec3                  m_previousPosition;
	Vec3                  m_velocity;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	std::map<char const* const, float> m_parameterInfo;
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
