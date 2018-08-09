// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseConnection.h"

namespace ACE
{
namespace Impl
{
namespace Adx2
{
class CCueConnection final : public CBaseConnection
{
public:

	enum class EActionType
	{
		Start,
		Stop,
		Pause,
		Resume,
	};

	explicit CCueConnection(ControlId const id, string const& cueSheetName = "", EActionType const actionType = EActionType::Start)
		: CBaseConnection(id)
		, m_cueSheetName(cueSheetName)
		, m_actionType(actionType)
	{}

	CCueConnection() = delete;

	// CBaseConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CBaseConnection

	string const& GetCueSheetName() const { return m_cueSheetName; }
	EActionType   GetActionType() const   { return m_actionType; }

private:

	string const m_cueSheetName;
	EActionType  m_actionType;
};
} // namespace Adx2
} // namespace Impl
} // namespace ACE
