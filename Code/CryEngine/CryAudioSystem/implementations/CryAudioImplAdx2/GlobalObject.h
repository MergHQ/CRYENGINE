// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseObject.h"

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
class CGlobalObject final : public CBaseObject
{
public:

	CGlobalObject(CGlobalObject const&) = delete;
	CGlobalObject(CGlobalObject&&) = delete;
	CGlobalObject& operator=(CGlobalObject const&) = delete;
	CGlobalObject& operator=(CGlobalObject&&) = delete;

	CGlobalObject();
	virtual ~CGlobalObject() override;

	// CryAudio::Impl::IObject
	virtual void SetTransformation(CTransformation const& transformation) override;
	virtual void SetOcclusion(float const occlusion) override;
	virtual void SetOcclusionType(EOcclusionType const occlusionType) override;
	// ~CryAudio::Impl::IObject
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
