// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseConnection.h"

#include <PoolObject.h>
#include <CryAudioImplFmod/GlobalData.h>

namespace ACE
{
namespace Impl
{
namespace Fmod
{
class CKeyConnection final : public CBaseConnection, public CryAudio::CPoolObject<CKeyConnection, stl::PSyncNone>
{
public:

	CKeyConnection() = delete;
	CKeyConnection(CKeyConnection const&) = delete;
	CKeyConnection(CKeyConnection&&) = delete;
	CKeyConnection& operator=(CKeyConnection const&) = delete;
	CKeyConnection& operator=(CKeyConnection&&) = delete;

	explicit CKeyConnection(
		ControlId const id,
		string const event = "")
		: CBaseConnection(id)
		, m_event(event)
	{}

	virtual ~CKeyConnection() override = default;

	// CBaseConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CBaseConnection

	string GetEvent() const { return m_event; }

private:

	string m_event;
};
} // namespace Fmod
} // namespace Impl
} // namespace ACE
