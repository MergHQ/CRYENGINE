// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseObject.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CGlobalObject final : public CBaseObject
{
public:

	CGlobalObject() = delete;
	CGlobalObject(CGlobalObject const&) = delete;
	CGlobalObject(CGlobalObject&&) = delete;
	CGlobalObject& operator=(CGlobalObject const&) = delete;
	CGlobalObject& operator=(CGlobalObject&&) = delete;

	explicit CGlobalObject(Objects const& objects);
	virtual ~CGlobalObject() override = default;

	// CryAudio::Impl::IObject
	virtual void SetEnvironment(IEnvironment const* const pIEnvironment, float const amount) override;
	virtual void SetParameter(IParameter const* const pIParameter, float const value) override;
	virtual void SetSwitchState(ISwitchState const* const pISwitchState) override;
	virtual void SetObstructionOcclusion(float const obstruction, float const occlusion) override;
	virtual void SetOcclusionType(EOcclusionType const occlusionType) override;
	virtual void ToggleFunctionality(EObjectFunctionality const type, bool const enable) override;
	// ~CryAudio::Impl::IObject

private:

	Objects const& m_objects;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
