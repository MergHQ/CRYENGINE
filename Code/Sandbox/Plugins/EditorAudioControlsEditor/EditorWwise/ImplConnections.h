// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ImplConnection.h>

namespace ACE
{
namespace Wwise
{
class CParameterConnection final : public CImplConnection
{
public:

	explicit CParameterConnection(CID const id)
		: CImplConnection(id)
		, m_mult(1.0f)
		, m_shift(0.0f)
	{}

	CParameterConnection() = delete;

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

using ParameterConnectionPtr = std::shared_ptr<CParameterConnection>;

class CStateToParameterConnection final : public CImplConnection
{
public:

	explicit CStateToParameterConnection(CID const id)
		: CImplConnection(id)
		, m_value(0.0f)
	{}

	CStateToParameterConnection() = delete;

	// CImplConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CImplConnection

	void  SetValue(float const value) { m_value = value; }
	float GetValue() const            { return m_value; }

private:

	float m_value;
};

using StateConnectionPtr = std::shared_ptr<CStateToParameterConnection>;
} // namespace Wwise
} // namespace ACE
