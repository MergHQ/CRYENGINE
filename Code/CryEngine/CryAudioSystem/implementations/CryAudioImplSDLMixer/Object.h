// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include <IObject.h>
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

	explicit CObject(CTransformation const& transformation, uint32 const id)
		: m_id(id)
		, m_transformation(transformation)
	{}

	virtual ~CObject() override = default;

	// CryAudio::Impl::IObject
	virtual void                   Update(float const deltaTime) override;
	virtual void                   SetTransformation(CTransformation const& transformation) override                { m_transformation = transformation; }
	virtual CTransformation const& GetTransformation() const override                                               { return m_transformation; }
	virtual void                   SetOcclusion(float const occlusion) override                                     {}
	virtual void                   SetOcclusionType(EOcclusionType const occlusionType) override                    {}
	virtual void                   StopAllTriggers() override                                                       {}
	virtual ERequestStatus         SetName(char const* const szName) override;
	virtual void                   ToggleFunctionality(EObjectFunctionality const type, bool const enable) override {}

	// Below data is only used when INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE is defined!
	virtual void DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter) override {}
	// ~CryAudio::Impl::IObject

	void StopEvent(uint32 const id);
	void SetVolume(SampleId const sampleId, float const value);

#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
	char const* GetName() const { return m_name.c_str(); }
#endif  // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE

	uint32 const               m_id;
	CTransformation            m_transformation;
	EventInstances             m_events;
	StandAloneFileInstanceList m_standaloneFiles;
	VolumeMultipliers          m_volumeMultipliers;

#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
private:

	CryFixedStringT<MaxObjectNameLength> m_name;
#endif  // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE
};
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio