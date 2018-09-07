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

	explicit CObject(CObjectTransformation const& transformation);
	virtual ~CObject() override;

	// CryAudio::Impl::IObject
	virtual void Update(float const deltaTime) override;
	virtual void SetTransformation(CObjectTransformation const& transformation) override;
	virtual void SetEnvironment(IEnvironment const* const pIEnvironment, float const amount) override;
	virtual void SetParameter(IParameter const* const pIParameter, float const value) override;
	virtual void SetSwitchState(ISwitchState const* const pISwitchState) override;
	virtual void SetObstructionOcclusion(float const obstruction, float const occlusion) override;
	virtual void SetOcclusionType(EOcclusionType const occlusionType) override;
	virtual void ToggleFunctionality(EObjectFunctionality const type, bool const enable) override;

	// Below data is only used when INCLUDE_FMOD_IMPL_PRODUCTION_CODE is defined!
	virtual void DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter) override;
	// ~CryAudio::Impl::IObject

private:

	void Set3DAttributes();
	void UpdateVelocities(float const deltaTime);
	void SetParameterById(uint32 const parameterId, float const value);

	EObjectFlags m_flags;
	float        m_previousAbsoluteVelocity;
	Vec3         m_position;
	Vec3         m_previousPosition;
	Vec3         m_velocity;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	std::map<char const* const, float> m_parameterInfo;
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
