// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
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
	IsVirtual             = BIT(4),
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
	virtual void                   SetOcclusion(float const occlusion) override;
	virtual void                   SetOcclusionType(EOcclusionType const occlusionType) override;
	virtual void                   StopAllTriggers() override;
	virtual ERequestStatus         SetName(char const* const szName) override;
	virtual void                   ToggleFunctionality(EObjectFunctionality const type, bool const enable) override;

	// Below data is only used when INCLUDE_WWISE_IMPL_PRODUCTION_CODE is defined!
	virtual void DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter) override;
	// ~CryAudio::Impl::IObject

	void           AddEvent(CEvent* const pEvent);
	void           SetNeedsToUpdateEnvironments(bool const needsUpdate) { m_needsToUpdateEnvironments = needsUpdate; }
	void           PostEnvironmentAmounts();
	void           SetDistanceToListener();
	float          GetDistanceToListener() const { return m_distanceToListener; }
	AkGameObjectID GetId() const                 { return m_id; }

	Events const&  GetEvents() const             { return m_events; }

	using AuxSendValues = std::vector<AkAuxSendValue>;
	AuxSendValues& GetAuxSendValues() { return m_auxSendValues; }

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	char const* GetName() const { return m_name.c_str(); }
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

private:

	void UpdateVelocities(float const deltaTime);
	void TryToSetRelativeVelocity(float const relativeVelocity);

	Events               m_events;
	bool                 m_needsToUpdateEnvironments;
	AuxSendValues        m_auxSendValues;

	AkGameObjectID const m_id;
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
