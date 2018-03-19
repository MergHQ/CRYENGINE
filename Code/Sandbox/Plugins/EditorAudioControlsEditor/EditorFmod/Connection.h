// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IConnection.h>
#include <CryAudioImplFmod/GlobalData.h>

#include "Item.h"

namespace ACE
{
namespace Fmod
{
class CBaseConnection : public IConnection
{
public:

	explicit CBaseConnection(ControlId const id)
		: m_id(id)
	{}

	virtual ~CBaseConnection() override = default;

	CBaseConnection() = delete;

	// IConnection
	virtual ControlId GetID() const override final                                                             { return m_id; }
	virtual bool      HasProperties() const override                                                           { return false; }
	virtual void      Serialize(Serialization::IArchive& ar) override                                          {}
	virtual void      SetPlatformEnabled(PlatformIndexType const platformIndex, bool const isEnabled) override {}
	virtual bool      IsPlatformEnabled(PlatformIndexType const platformIndex) const override                  { return true; }
	virtual void      ClearPlatforms() override                                                                {}
	// ~IConnection

private:

	ControlId const m_id;
};

enum class EEventActionType
{
	Start,
	Stop,
	Pause,
	Resume,
};

class CEventConnection final : public CBaseConnection
{
public:

	explicit CEventConnection(ControlId const id, EEventActionType const actionType = EEventActionType::Start)
		: CBaseConnection(id)
		, m_actionType(actionType)
	{}

	CEventConnection() = delete;

	// CBaseConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CBaseConnection

	EEventActionType GetActionType() const { return m_actionType; }

private:

	EEventActionType m_actionType;
};

enum class ESnapshotActionType
{
	Start,
	Stop,
};

class CSnapshotConnection final : public CBaseConnection
{
public:

	explicit CSnapshotConnection(ControlId const id, ESnapshotActionType const actionType = ESnapshotActionType::Start)
		: CBaseConnection(id)
		, m_actionType(actionType)
	{}

	CSnapshotConnection() = delete;

	// CBaseConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CBaseConnection

	ESnapshotActionType GetActionType() const { return m_actionType; }

private:

	ESnapshotActionType m_actionType;
};

class CParameterConnection final : public CBaseConnection
{
public:

	explicit CParameterConnection(
	  ControlId const id,
	  float const mult = CryAudio::Impl::Fmod::s_defaultParamMultiplier,
	  float const shift = CryAudio::Impl::Fmod::s_defaultParamShift)
		: CBaseConnection(id)
		, m_mult(mult)
		, m_shift(shift)
	{}

	CParameterConnection() = delete;

	// CBaseConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CBaseConnection

	float GetMultiplier() const { return m_mult; }
	float GetShift() const      { return m_shift; }

private:

	float m_mult;
	float m_shift;
};

class CParameterToStateConnection final : public CBaseConnection
{
public:

	explicit CParameterToStateConnection(
	  ControlId const id,
	  EItemType const itemType,
	  float const value = CryAudio::Impl::Fmod::s_defaultStateValue)
		: CBaseConnection(id)
		, m_itemType(itemType)
		, m_value(value)
	{}

	CParameterToStateConnection() = delete;

	// CBaseConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CBaseConnection

	float GetValue() const { return m_value; }

private:

	EItemType const m_itemType;
	float           m_value;
};

class CBankConnection final : public CBaseConnection
{
public:

	explicit CBankConnection(ControlId const id)
		: CBaseConnection(id)
		, m_configurationsMask(std::numeric_limits<PlatformIndexType>::max())
	{}

	CBankConnection() = delete;

	// CBaseConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	virtual void SetPlatformEnabled(PlatformIndexType const platformIndex, bool const isEnabled) override;
	virtual bool IsPlatformEnabled(PlatformIndexType const platformIndex) const override;
	virtual void ClearPlatforms() override;
	// ~CBaseConnection

private:

	PlatformIndexType m_configurationsMask;
};
} // namespace Fmod
} // namespace ACE

