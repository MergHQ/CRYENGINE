// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseObject.h"
#include <PoolObject.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
class CObject final : public CBaseObject, public CPoolObject<CObject, stl::PSyncNone>
{
public:

	CObject() = delete;
	CObject(CObject const&) = delete;
	CObject(CObject&&) = delete;
	CObject& operator=(CObject const&) = delete;
	CObject& operator=(CObject&&) = delete;

	explicit CObject(AkGameObjectID const id, CTransformation const& transformation, char const* const szName);
	virtual ~CObject() override;

	// CryAudio::Impl::IObject
	virtual void                   Update(float const deltaTime) override;
	virtual void                   SetTransformation(CTransformation const& transformation) override;
	virtual CTransformation const& GetTransformation() const override { return m_transformation; }
	virtual void                   SetOcclusion(float const occlusion) override;
	virtual void                   SetOcclusionType(EOcclusionType const occlusionType) override;
	virtual void                   ToggleFunctionality(EObjectFunctionality const type, bool const enable) override;

	// Below data is only used when INCLUDE_WWISE_IMPL_PRODUCTION_CODE is defined!
	virtual void DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter) override;
	// ~CryAudio::Impl::IObject

	// CBaseObject
	virtual void SetAuxSendValues() override;
	// ~CBaseObject

	void SetAuxBusSend(AkAuxBusID const busId, float const amount);

private:

	void UpdateVelocities(float const deltaTime);
	void TryToSetRelativeVelocity(float const relativeVelocity);

	bool            m_needsToUpdateAuxSends;
	float           m_previousRelativeVelocity;
	float           m_previousAbsoluteVelocity;
	CTransformation m_transformation;
	Vec3            m_previousPosition;
	Vec3            m_velocity;

	using AuxSendValues = std::vector<AkAuxSendValue>;
	AuxSendValues m_auxSendValues;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	std::map<char const* const, float> m_parameterInfo;
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
