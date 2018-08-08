// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseObject.h"
#include <PoolObject.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
class CObject final : public CBaseObject, public CPoolObject<CObject, stl::PSyncNone>
{
public:

	CObject()
		: m_obstruction(0.0f)
		, m_occlusion(0.0f)
	{}

	CObject(CObject const&) = delete;
	CObject(CObject&&) = delete;
	CObject& operator=(CObject const&) = delete;
	CObject& operator=(CObject&&) = delete;

	// CryAudio::Impl::IObject
	virtual void SetEnvironment(IEnvironment const* const pIEnvironment, float const amount) override;
	virtual void SetParameter(IParameter const* const pIParameter, float const value) override;
	virtual void SetSwitchState(ISwitchState const* const pISwitchState) override;
	virtual void SetObstructionOcclusion(float const obstruction, float const occlusion) override;
	// ~CryAudio::Impl::IObject

private:

	float m_obstruction;
	float m_occlusion;
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
