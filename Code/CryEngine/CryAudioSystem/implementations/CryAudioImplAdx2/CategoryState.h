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
class CCategoryState final : public ISwitchStateConnection, public CPoolObject<CCategoryState, stl::PSyncNone>
{
public:

	CCategoryState() = delete;
	CCategoryState(CCategoryState const&) = delete;
	CCategoryState(CCategoryState&&) = delete;
	CCategoryState& operator=(CCategoryState const&) = delete;
	CCategoryState& operator=(CCategoryState&&) = delete;

	explicit CCategoryState(char const* const szName, CriFloat32 const value)
		: m_name(szName)
		, m_value(value)
	{}

	virtual ~CCategoryState() override = default;

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
