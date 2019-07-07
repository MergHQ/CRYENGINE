// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

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
class CGameVariableState final : public ISwitchStateConnection, public CPoolObject<CGameVariableState, stl::PSyncNone>
{
public:

	CGameVariableState() = delete;
	CGameVariableState(CGameVariableState const&) = delete;
	CGameVariableState(CGameVariableState&&) = delete;
	CGameVariableState& operator=(CGameVariableState const&) = delete;
	CGameVariableState& operator=(CGameVariableState&&) = delete;

	explicit CGameVariableState(char const* const szName, CriFloat32 const value)
		: m_name(szName)
		, m_value(value)
	{}

	virtual ~CGameVariableState() override = default;

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
