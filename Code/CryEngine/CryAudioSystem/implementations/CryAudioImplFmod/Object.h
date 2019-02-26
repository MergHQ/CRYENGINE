// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseObject.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CObject final : public CBaseObject, public CPoolObject<CObject, stl::PSyncNone>
{
public:

	CObject() = delete;
	CObject(CObject const&) = delete;
	CObject(CObject&&) = delete;
	CObject& operator=(CObject const&) = delete;
	CObject& operator=(CObject&&) = delete;

	explicit CObject(CTransformation const& transformation);
	virtual ~CObject() override;

	// CryAudio::Impl::IObject
	virtual void                   Update(float const deltaTime) override;
	virtual void                   SetTransformation(CTransformation const& transformation) override;
	virtual CTransformation const& GetTransformation() const override { return m_transformation; }
	virtual void                   SetOcclusion(float const occlusion) override;
	virtual void                   SetOcclusionType(EOcclusionType const occlusionType) override;
	virtual void                   ToggleFunctionality(EObjectFunctionality const type, bool const enable) override;

	// Below data is only used when CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE is defined!
	virtual void DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter) override;
	// ~CryAudio::Impl::IObject

private:

	void Set3DAttributes();
	void UpdateVelocities(float const deltaTime);
	void SetAbsoluteVelocity(float const velocity);

	CTransformation m_transformation;
	float           m_previousAbsoluteVelocity;
	Vec3            m_position;
	Vec3            m_previousPosition;
	Vec3            m_velocity;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
