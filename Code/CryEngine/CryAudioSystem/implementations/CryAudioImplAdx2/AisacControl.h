// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IParameterConnection.h>
#include <PoolObject.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
class CAisacControl final : public IParameterConnection, public CPoolObject<CAisacControl, stl::PSyncNone>
{
public:

	CAisacControl() = delete;
	CAisacControl(CAisacControl const&) = delete;
	CAisacControl(CAisacControl&&) = delete;
	CAisacControl& operator=(CAisacControl const&) = delete;
	CAisacControl& operator=(CAisacControl&&) = delete;

	explicit CAisacControl(char const* const szName, float const multiplier, float const shift)
		: m_name(szName)
		, m_multiplier(multiplier)
		, m_shift(shift)
	{}

	virtual ~CAisacControl() override = default;

	// IParameterConnection
	virtual void Set(IObject* const pIObject, float const value) override;
	virtual void SetGlobally(float const value) override;
	// ~IParameterConnection

private:

	CryFixedStringT<MaxControlNameLength> const m_name;
	float const m_multiplier;
	float const m_shift;
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
