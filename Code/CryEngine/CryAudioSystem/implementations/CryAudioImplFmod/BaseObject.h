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

	// Below data is only used when CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE is defined!
	virtual void DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter) override {}
	// ~CryAudio::Impl::IObject

	EventInstances const& GetEventInstances() const { return m_eventInstances; }
	FMOD_3D_ATTRIBUTES&   GetAttributes()           { return m_attributes; }

	bool                  SetEventInstance(CEventInstance* const pEventInstance);
	void                  AddEventInstance(CEventInstance* const pEventInstance);

	void                  StopEventInstance(uint32 const id);

	void                  SetParameter(uint32 const id, float const value);
	void                  RemoveParameter(uint32 const id);

	void                  SetReturn(CReturn const* const pReturn, float const amount);
	void                  RemoveReturn(CReturn const* const pReturn);

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	char const* GetName() const { return m_name.c_str(); }
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

protected:

	CBaseObject();

	void UpdateVirtualFlag(CEventInstance* const pEventInstance);
	void UpdateVelocityTracking();

	EObjectFlags       m_flags;

	EventInstances     m_eventInstances;
	Parameters         m_parameters;
	Returns            m_returns;

	FMOD_3D_ATTRIBUTES m_attributes;
	float              m_occlusion = 0.0f;
	float              m_absoluteVelocity = 0.0f;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	CryFixedStringT<MaxObjectNameLength> m_name;
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
