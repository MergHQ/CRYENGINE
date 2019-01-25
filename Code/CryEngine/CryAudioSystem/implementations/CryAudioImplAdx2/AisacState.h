// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ISwitchStateConnection.h>
#include <PoolObject.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

#include <cri_atom_ex.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
class CAisacState final : public ISwitchStateConnection, public CPoolObject<CAisacState, stl::PSyncNone>
{
public:

	CAisacState() = delete;
	CAisacState(CAisacState const&) = delete;
	CAisacState(CAisacState&&) = delete;
	CAisacState& operator=(CAisacState const&) = delete;
	CAisacState& operator=(CAisacState&&) = delete;

	explicit CAisacState(char const* const szName, CriFloat32 const value)
		: m_name(szName)
		, m_value(value)
	{}

	virtual ~CAisacState() override = default;

	// ISwitchStateConnection
	virtual void Set(IObject* const pIObject) override;
	virtual void SetGlobally() override;
	// ~ISwitchStateConnection

private:

	CryFixedStringT<MaxControlNameLength> const m_name;
	CriFloat32 const                            m_value;
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
