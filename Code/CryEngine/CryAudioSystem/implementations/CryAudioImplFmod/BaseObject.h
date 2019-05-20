// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include "ParameterInfo.h"
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
	virtual void                   AddListener(IListener* const pIListener) override;
	virtual void                   RemoveListener(IListener* const pIListener) override;

	// Below data is only used when CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE is defined!
	virtual void DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter) override {}
	// ~CryAudio::Impl::IObject

	int                   GetListenerMask() const   { return m_listenerMask; }

	EventInstances const& GetEventInstances() const { return m_eventInstances; }
	FMOD_3D_ATTRIBUTES&   GetAttributes()           { return m_attributes; }

	bool                  SetEventInstance(CEventInstance* const pEventInstance);
	void                  AddEventInstance(CEventInstance* const pEventInstance);

	void                  StopEventInstance(uint32 const id);

	void                  SetParameter(CParameterInfo& parameterInfo, float const value);
	void                  RemoveParameter(CParameterInfo const& parameterInfo);

	void                  SetReturn(CReturn const* const pReturn, float const amount);
	void                  RemoveReturn(CReturn const* const pReturn);

#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)
	char const* GetName() const          { return m_name.c_str(); }
	char const* GetListenerNames() const { return m_listenerNames.c_str(); }
#endif  // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE

protected:

	CBaseObject(int const listenerMask, Listeners const& listeners);

	void UpdateVirtualFlag(CEventInstance* const pEventInstance);
	void UpdateVelocityTracking();

#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)
	void UpdateListenerNames();
#endif  // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE

	int                m_listenerMask;
	EObjectFlags       m_flags;

	EventInstances     m_eventInstances;
	Parameters         m_parameters;
	Returns            m_returns;

	FMOD_3D_ATTRIBUTES m_attributes;
	float              m_occlusion;
	float              m_absoluteVelocity;

	Listeners          m_listeners;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)
	CryFixedStringT<MaxObjectNameLength> m_name;
	CryFixedStringT<MaxMiscStringLength> m_listenerNames;
#endif  // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
