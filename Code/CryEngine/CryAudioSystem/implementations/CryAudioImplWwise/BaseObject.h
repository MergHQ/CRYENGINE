// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include <IObject.h>
#include <AK/SoundEngine/Common/AkTypes.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
class CEventInstance;

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

class CBaseObject : public IObject
{
public:

	CBaseObject() = delete;
	CBaseObject(CBaseObject const&) = delete;
	CBaseObject(CBaseObject&&) = delete;
	CBaseObject& operator=(CBaseObject const&) = delete;
	CBaseObject& operator=(CBaseObject&&) = delete;

	virtual ~CBaseObject() override = default;

	// CryAudio::Impl::IObject
	virtual void                   Update(float const deltaTime) override;
	virtual void                   SetTransformation(CTransformation const& transformation) override {}
	virtual CTransformation const& GetTransformation() const override                                { return CTransformation::GetEmptyObject(); }
	virtual void                   SetOcclusion(float const occlusion) override                      {}
	virtual void                   SetOcclusionType(EOcclusionType const occlusionType) override     {}
	virtual void                   StopAllTriggers() override;
	virtual ERequestStatus         SetName(char const* const szName) override;
	virtual void                   ToggleFunctionality(EObjectFunctionality const type, bool const enable) override {}

	// Below data is only used when CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE is defined!
	virtual void DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter) override {}
	// ~CryAudio::Impl::IObject

	virtual void   SetAuxSendValues() {}

	AkGameObjectID GetId() const      { return m_id; }

	void           AddEventInstance(CEventInstance* const pEventInstance);
	void           StopEvent(AkUniqueID const eventId);

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	char const* GetName() const { return m_name.c_str(); }
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

protected:

	explicit CBaseObject(
		AkGameObjectID const id,
		char const* const szName = nullptr,
		Vec3 const position = { 0.0f, 0.0f, 0.0f });

	void SetDistanceToListener();

	AkGameObjectID const m_id;
	EObjectFlags         m_flags;
	Vec3                 m_position;
	float                m_distanceToListener;
	EventInstances       m_eventInstances;

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	CryFixedStringT<MaxObjectNameLength> m_name;
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
