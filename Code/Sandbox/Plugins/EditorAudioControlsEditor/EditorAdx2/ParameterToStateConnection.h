// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseConnection.h"
#include "Item.h"

#include <PoolObject.h>
#include <CryAudioImplAdx2/GlobalData.h>

namespace ACE
{
namespace Impl
{
namespace Adx2
{
class CParameterToStateConnection final : public CBaseConnection, public CryAudio::CPoolObject<CParameterToStateConnection, stl::PSyncNone>
{
public:

	CParameterToStateConnection() = delete;
	CParameterToStateConnection(CParameterToStateConnection const&) = delete;
	CParameterToStateConnection(CParameterToStateConnection&&) = delete;
	CParameterToStateConnection& operator=(CParameterToStateConnection const&) = delete;
	CParameterToStateConnection& operator=(CParameterToStateConnection&&) = delete;

	explicit CParameterToStateConnection(
		ControlId const id,
		EItemType const itemType,
		float const value = CryAudio::Impl::Adx2::s_defaultStateValue)
		: CBaseConnection(id)
		, m_itemType(itemType)
		, m_value(value)
	{}

	virtual ~CParameterToStateConnection() override = default;

	// CBaseConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CBaseConnection

	float GetValue() const { return m_value; }

private:

	EItemType const m_itemType;
	float           m_value;
};
} // namespace Adx2
} // namespace Impl
} // namespace ACE
