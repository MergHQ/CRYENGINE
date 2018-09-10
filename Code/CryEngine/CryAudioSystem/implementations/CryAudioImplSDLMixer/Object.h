// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include <ATLEntityData.h>
#include <PoolObject.h>

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
using VolumeMultipliers = std::map<SampleId, float>;

class CObject final : public IObject, public CPoolObject<CObject, stl::PSyncNone>
{
public:

	CObject() = delete;
	CObject(CObject const&) = delete;
	CObject(CObject&&) = delete;
	CObject& operator=(CObject const&) = delete;
	CObject& operator=(CObject&&) = delete;

	explicit CObject(uint32 const id);
	virtual ~CObject() override = default;

	// CryAudio::Impl::IObject
	virtual void                         Update(float const deltaTime) override                                               {}
	virtual void                         SetTransformation(CObjectTransformation const& transformation) override;
	virtual CObjectTransformation const& GetTransformation() const override                                                   { return m_transformation; }
	virtual void                         SetEnvironment(IEnvironment const* const pIEnvironment, float const amount) override {}
	virtual void                         SetParameter(IParameter const* const pIParameter, float const value) override;
	virtual void                         SetSwitchState(ISwitchState const* const pISwitchState) override;
	virtual void                         SetObstructionOcclusion(float const obstruction, float const occlusion) override {}
	virtual void                         SetOcclusionType(EOcclusionType const occlusionType) override                    {}
	virtual ERequestStatus               ExecuteTrigger(ITrigger const* const pITrigger, IEvent* const pIEvent) override;
	virtual void                         StopAllTriggers() override                                                       {}
	virtual ERequestStatus               PlayFile(IStandaloneFile* const pIStandaloneFile) override;
	virtual ERequestStatus               StopFile(IStandaloneFile* const pIStandaloneFile) override;
	virtual ERequestStatus               SetName(char const* const szName) override;
	virtual void                         ToggleFunctionality(EObjectFunctionality const type, bool const enable) override {}

	// Below data is only used when INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE is defined!
	virtual void DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter) override {}
	// ~CryAudio::Impl::IObject

	const uint32               m_id;
	CObjectTransformation      m_transformation;
	EventInstanceList          m_events;
	StandAloneFileInstanceList m_standaloneFiles;
	VolumeMultipliers          m_volumeMultipliers;

private:

	void SetVolume(SampleId const sampleId);
};
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio