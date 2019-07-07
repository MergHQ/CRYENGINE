// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ITriggerConnection.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
class CBaseTriggerConnection : public ITriggerConnection
{
public:

	enum class EType : EnumFlagsType
	{
		Cue,
		Snapshot,
	};

	CBaseTriggerConnection() = delete;
	CBaseTriggerConnection(CBaseTriggerConnection const&) = delete;
	CBaseTriggerConnection(CBaseTriggerConnection&&) = delete;
	CBaseTriggerConnection& operator=(CBaseTriggerConnection const&) = delete;
	CBaseTriggerConnection& operator=(CBaseTriggerConnection&&) = delete;

	virtual ~CBaseTriggerConnection() override = default;

	EType       GetType() const { return m_type; }
	char const* GetName() const { return m_name.c_str(); }

protected:

	explicit CBaseTriggerConnection(EType const type, char const* const szName)
		: m_type(type)
		, m_name(szName)
	{}

	EType const m_type;
	CryFixedStringT<MaxControlNameLength> const m_name;
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
