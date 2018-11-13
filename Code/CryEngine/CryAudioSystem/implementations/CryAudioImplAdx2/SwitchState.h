// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "GlobalData.h"
#include <ISwitchStateConnection.h>
#include <PoolObject.h>

#include <cri_atom_ex.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
enum class ESwitchType : EnumFlagsType
{
	None,
	Selector,
	AisacControl,
	Category,
	GameVariable,
};

class CSwitchState final : public ISwitchStateConnection, public CPoolObject<CSwitchState, stl::PSyncNone>
{
public:

	CSwitchState() = delete;
	CSwitchState(CSwitchState const&) = delete;
	CSwitchState(CSwitchState&&) = delete;
	CSwitchState& operator=(CSwitchState const&) = delete;
	CSwitchState& operator=(CSwitchState&&) = delete;

	explicit CSwitchState(
		ESwitchType const type,
		char const* const szName,
		char const* const szLabelName,
		CriFloat32 const value = static_cast<CriFloat32>(s_defaultStateValue))
		: m_type(type)
		, m_name(szName)
		, m_labelName(szLabelName)
		, m_value(value)
	{}

	virtual ~CSwitchState() override = default;

	ESwitchType     GetType() const      { return m_type; }
	CriChar8 const* GetName() const      { return static_cast<CriChar8 const*>(m_name); }
	CriChar8 const* GetLabelName() const { return static_cast<CriChar8 const*>(m_labelName); }
	CriFloat32      GetValue() const     { return m_value; }

private:

	ESwitchType const                           m_type;
	CryFixedStringT<MaxControlNameLength> const m_name;
	CryFixedStringT<MaxControlNameLength> const m_labelName;
	CriFloat32 const                            m_value;
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
