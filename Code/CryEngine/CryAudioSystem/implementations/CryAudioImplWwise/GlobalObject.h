// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseObject.h"

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
class CGlobalObject final : public CBaseObject
{
public:

	CGlobalObject() = delete;
	CGlobalObject(CGlobalObject const&) = delete;
	CGlobalObject(CGlobalObject&&) = delete;
	CGlobalObject& operator=(CGlobalObject const&) = delete;
	CGlobalObject& operator=(CGlobalObject&&) = delete;

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	CGlobalObject(AkGameObjectID const id, char const* const szName)
		: CBaseObject(id, szName)
	{}
#else
	CGlobalObject(AkGameObjectID const id)
		: CBaseObject(id)
	{}
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

	virtual ~CGlobalObject() override = default;

	// CryAudio::Impl::IObject
	virtual void SetTransformation(CTransformation const& transformation) override;
	virtual void SetOcclusion(float const occlusion) override;
	virtual void SetOcclusionType(EOcclusionType const occlusionType) override;
	// ~CryAudio::Impl::IObject
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
