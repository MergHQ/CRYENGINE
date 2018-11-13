// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IObject.h>
#include <PoolObject.h>

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
class CEvent;

class CObject final : public IObject, public CPoolObject<CObject, stl::PSyncNone>
{
public:

	CObject(CObject const&) = delete;
	CObject(CObject&&) = delete;
	CObject& operator=(CObject const&) = delete;
	CObject& operator=(CObject&&) = delete;

	CObject() = default;
	virtual ~CObject() override = default;

	// CryAudio::Impl::IObject
	virtual void                   Update(float const deltaTime) override;
	virtual void                   SetTransformation(CTransformation const& transformation) override;
	virtual CTransformation const& GetTransformation() const override { return CTransformation::GetEmptyObject(); }
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

	// Below data is only used when INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE is defined!
	virtual void DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter) override;
	// ~CryAudio::Impl::IObject

	void StopEvent(uint32 const pathId);
	void RegisterEvent(CEvent* const pEvent);
	void UnregisterEvent(CEvent* const pEvent);

private:

	std::vector<CEvent*> m_activeEvents;
};
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio
