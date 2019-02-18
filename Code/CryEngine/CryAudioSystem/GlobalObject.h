// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "TriggerInstance.h"

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	#include "Debug.h"

struct IRenderAuxGeom;
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

namespace CryAudio
{
class CGlobalObject final
{
public:

	CGlobalObject(CGlobalObject const&) = delete;
	CGlobalObject(CGlobalObject&&) = delete;
	CGlobalObject& operator=(CGlobalObject const&) = delete;
	CGlobalObject& operator=(CGlobalObject&&) = delete;

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)

	CGlobalObject() = delete;

	explicit CGlobalObject(char const* const szName)
		: m_pIObject(nullptr)
		, m_transformation(CTransformation::GetEmptyObject())
		, m_name(szName)
	{}
#else
	CGlobalObject()
		: m_pIObject(nullptr)
	{}
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

	void           Release();

	void           StopAllTriggers();

	void           SetImplDataPtr(Impl::IObject* const pIObject);
	Impl::IObject* GetImplDataPtr() const { return m_pIObject; }

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

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
public:

	void ForceImplementationRefresh(bool const setTransformation);

	// Transformation is used to link the preview object to the listener.
	CTransformation const& GetTransformation() const { return m_transformation; }
	void                   HandleSetTransformation(CTransformation const& transformation);

	char const*            GetName() const { return m_name.c_str(); }
	ERequestStatus         HandleSetName(char const* const szName);

	bool                   IsActive() const { return !m_triggerInstances.empty(); }

	void                   StoreParameterValue(ControlId const id, float const value);
	void                   StoreSwitchValue(ControlId const switchId, SwitchStateId const switchStateId);

	void                   DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY);

private:

	bool ExecuteDefaultTrigger(ControlId const id);

	CTransformation                      m_transformation;
	ParameterValues                      m_parameters;
	SwitchStateIds                       m_switchStates;
	CryFixedStringT<MaxObjectNameLength> m_name;
	Debug::StateDrawInfo                 m_stateDrawInfo;
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
};
} // namespace CryAudio
