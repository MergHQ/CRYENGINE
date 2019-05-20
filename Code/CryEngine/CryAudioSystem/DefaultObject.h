// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "TriggerInstance.h"

#if defined(CRY_AUDIO_USE_DEBUG_CODE)
	#include "Debug.h"

struct IRenderAuxGeom;
#endif // CRY_AUDIO_USE_DEBUG_CODE

namespace CryAudio
{
class CDefaultObject final
{
public:

	CDefaultObject(CDefaultObject const&) = delete;
	CDefaultObject(CDefaultObject&&) = delete;
	CDefaultObject& operator=(CDefaultObject const&) = delete;
	CDefaultObject& operator=(CDefaultObject&&) = delete;

#if defined(CRY_AUDIO_USE_DEBUG_CODE)

	CDefaultObject() = delete;

	explicit CDefaultObject(char const* const szName)
		: m_pIObject(nullptr)
		, m_name(szName)
	{}
#else
	CDefaultObject()
		: m_pIObject(nullptr)
	{}
#endif // CRY_AUDIO_USE_DEBUG_CODE

	void           Release();

	void           StopAllTriggers();

	void           SetImplData(Impl::IObject* const pIObject);
	Impl::IObject* GetImplData() const { return m_pIObject; }

	void           Update(float const deltaTime);

	void           ConstructTriggerInstance(
		ControlId const triggerId,
		uint16 const numPlayingConnectionInstances,
		uint16 const numPendingConnectionInstances,
		ERequestFlags const flags,
		void* const pOwner,
		void* const pUserData,
		void* const pUserDataOwner);

	void ReportStartedTriggerInstance(TriggerInstanceId const triggerInstanceId, ETriggerResult const result);
	void ReportFinishedTriggerInstance(TriggerInstanceId const triggerInstanceId, ETriggerResult const result);

private:

	TriggerInstances m_triggerInstances;
	Impl::IObject*   m_pIObject;

#if defined(CRY_AUDIO_USE_DEBUG_CODE)
public:

	void        ForceImplementationRefresh();

	char const* GetName() const  { return m_name.c_str(); }

	bool        IsActive() const { return !m_triggerInstances.empty(); }

	void        StoreParameterValue(ControlId const id, float const value);
	void        StoreSwitchValue(ControlId const switchId, SwitchStateId const switchStateId);

	void        DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY);

private:

	bool ExecuteDefaultTrigger(ControlId const id);

	ParameterValues                            m_parameters;
	SwitchStateIds                             m_switchStates;
	CryFixedStringT<MaxObjectNameLength> const m_name;
	Debug::StateDrawInfo                       m_stateDrawInfo;
#endif // CRY_AUDIO_USE_DEBUG_CODE
};
} // namespace CryAudio
