// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ImplConnection.h>

namespace ACE
{
namespace PortAudio
{
enum class EConnectionType
{
	Start,
	Stop,
	NumTypes,
};

class CConnection final : public CImplConnection
{
public:

	explicit CConnection(CID const id)
		: CImplConnection(id)
		, m_type(EConnectionType::Start)
		, m_loopCount(1)
		, m_isInfiniteLoop(false)
	{}

	// CImplConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CImplConnection

	EConnectionType m_type;
	uint32          m_loopCount;
	bool            m_isInfiniteLoop;
};
} // namespace PortAudio
} // namespace ACE
