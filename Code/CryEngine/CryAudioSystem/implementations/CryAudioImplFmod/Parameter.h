// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseParameter.h"
#include <PoolObject.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CParameter final : public CBaseParameter, public CPoolObject<CParameter, stl::PSyncNone>
{
public:

	CParameter() = delete;
	CParameter(CParameter const&) = delete;
	CParameter(CParameter&&) = delete;
	CParameter& operator=(CParameter const&) = delete;
	CParameter& operator=(CParameter&&) = delete;

	explicit CParameter(
		uint32 const id,
		float const multiplier,
		float const shift)
		: CBaseParameter(id, multiplier, shift, EParameterType::Parameter)
	{}

	virtual ~CParameter() override = default;

	// IParameterConnection
	virtual void Set(IObject* const pIObject, float const value) override;
	virtual void SetGlobally(float const value) override;
	// ~IParameterConnection
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
