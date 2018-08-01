// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "InternalEntities.h"
#include <CryInput/IInput.h>

namespace CryAudio
{
namespace Impl
{
struct IImpl;
}   // namespace Impl

class CAudioRequest;
class CATLAudioObject;
class CATLTriggerImpl;
struct SAudioRequestData;

enum class EInternalStates : EnumFlagsType
{
	None                        = 0,
	AudioMiddlewareShuttingDown = BIT(0),
};
CRY_CREATE_ENUM_FLAG_OPERATORS(EInternalStates);

class CAudioTranslationLayer final : public IInputEventListener
{
public:

	CAudioTranslationLayer() = default;
	~CAudioTranslationLayer();

	CAudioTranslationLayer(CAudioTranslationLayer const&) = delete;
	CAudioTranslationLayer(CAudioTranslationLayer&&) = delete;
	CAudioTranslationLayer& operator=(CAudioTranslationLayer const&) = delete;
	CAudioTranslationLayer& operator=(CAudioTranslationLayer&&) = delete;

	// IInputEventListener
	virtual bool OnInputEvent(SInputEvent const& event) override;
	// ~IInputEventListener

	void        Initialize();
	void        Terminate();
	void        ProcessRequest(CAudioRequest& request);
	void        Update(float const deltaTime);
	bool        CanProcessRequests() const { return (m_flags& EInternalStates::AudioMiddlewareShuttingDown) == 0; }
	void        NotifyListener(CAudioRequest const& request);
	char const* GetConfigPath() const;

private:

	ERequestStatus ProcessAudioManagerRequest(CAudioRequest const& request);
	ERequestStatus ProcessAudioCallbackManagerRequest(CAudioRequest& request);
	ERequestStatus ProcessAudioObjectRequest(CAudioRequest const& request);
	ERequestStatus ProcessAudioListenerRequest(SAudioRequestData const* const pPassedRequestData);
	ERequestStatus SetImpl(Impl::IImpl* const pIImpl);
	void           ReleaseImpl();

	ERequestStatus RefreshAudioSystem(char const* const szLevelName);
	void           SetImplLanguage();
	void           CreateInternalControls();
	void           ClearInternalControls();
	void           SetCurrentEnvironmentsOnObject(CATLAudioObject* const pObject, EntityId const entityToIgnore);

	void           CreateInternalTrigger(char const* const szTriggerName, ControlId const triggerId, CATLTriggerImpl const* const pTriggerConnection);
	void           CreateInternalSwitch(char const* const szSwitchName, ControlId const switchId, std::vector<char const*> const& stateNames);

	SInternalControls m_internalControls;

	uint32            m_objectPoolSize = 0;
	uint32            m_eventPoolSize = 0;

	// Utility members
	EInternalStates                    m_flags = EInternalStates::None;
	SImplInfo                          m_implInfo;
	CryFixedStringT<MaxFilePathLength> m_configPath;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
public:

	void DrawAudioSystemDebugInfo();
	void GetAudioTriggerData(ControlId const audioTriggerId, STriggerData& audioTriggerData) const;

private:

	void DrawAudioObjectDebugInfo(IRenderAuxGeom& auxGeom);
	void DrawATLComponentDebugInfo(IRenderAuxGeom& auxGeom, float posX, float const posY);
	void RetriggerAudioControls();
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};
} // namespace CryAudio
