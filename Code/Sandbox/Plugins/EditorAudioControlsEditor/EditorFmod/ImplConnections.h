// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ImplConnection.h>

#include "ImplTypes.h"

namespace ACE
{
namespace Fmod
{
enum class EEventType
{
	Start,
	Stop,
	Pause,
	Resume,
};

class CEventConnection final : public CImplConnection
{
public:

	explicit CEventConnection(CID const nID)
		: CImplConnection(nID)
		, m_type(EEventType::Start)
	{}

	CEventConnection() = delete;

	// CImplConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CImplConnection

	void       SetType(EEventType const type) { m_type = type; }
	EEventType GetType() const                { return m_type; }

private:

	EEventType m_type;
};

enum class ESnapshotType
{
	Start,
	Stop,
};

class CSnapshotConnection final : public CImplConnection
{
public:

	explicit CSnapshotConnection(CID const nID)
		: CImplConnection(nID)
		, m_type(ESnapshotType::Start)
	{}

	CSnapshotConnection() = delete;

	// CImplConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CImplConnection

	void          SetType(ESnapshotType const type) { m_type = type; }
	ESnapshotType GetType() const                   { return m_type; }

private:

	ESnapshotType m_type;
};

class CParamConnection final : public CImplConnection
{
public:

	explicit CParamConnection(CID const nID)
		: CImplConnection(nID)
		, m_mult(1.0f)
		, m_shift(0.0f)
	{}

	CParamConnection() = delete;

	// CImplConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CImplConnection

	void  SetMultiplier(float const mult) { m_mult = mult; }
	float GetMultiplier() const           { return m_mult; }
	void  SetShift(float const shift)     { m_shift = shift; }
	float GetShift() const                { return m_shift; }

private:

	float m_mult;
	float m_shift;
};

class CParamToStateConnection final : public CImplConnection
{
public:

	explicit CParamToStateConnection(CID const id, EImplItemType const type)
		: CImplConnection(id)
		, m_type(type)
		, m_value(0.0f)
	{}

	CParamToStateConnection() = delete;

	// CImplConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CImplConnection

	void          SetValue(float const value) { m_value = value; }
	float         GetValue() const            { return m_value; }

	EImplItemType GetType() const             { return m_type; }

private:

	EImplItemType const m_type;
	float               m_value;
};
} // namespace Fmod
} // namespace ACE
