// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseConnection.h"

#include <PoolObject.h>

namespace ACE
{
namespace Impl
{
namespace Fmod
{
class CSnapshotConnection final : public CBaseConnection, public CryAudio::CPoolObject<CSnapshotConnection, stl::PSyncNone>
{
public:

	CSnapshotConnection() = delete;
	CSnapshotConnection(CSnapshotConnection const&) = delete;
	CSnapshotConnection(CSnapshotConnection&&) = delete;
	CSnapshotConnection& operator=(CSnapshotConnection const&) = delete;
	CSnapshotConnection& operator=(CSnapshotConnection&&) = delete;

	enum class EActionType : CryAudio::EnumFlagsType
	{
		Start,
		Stop, };

	explicit CSnapshotConnection(ControlId const id, EActionType const actionType = EActionType::Start)
		: CBaseConnection(id)
		, m_actionType(actionType)
	{}

	virtual ~CSnapshotConnection() override = default;

	// CBaseConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CBaseConnection

	EActionType GetActionType() const { return m_actionType; }

private:

	EActionType m_actionType;
};
} // namespace Fmod
} // namespace Impl
} // namespace ACE
