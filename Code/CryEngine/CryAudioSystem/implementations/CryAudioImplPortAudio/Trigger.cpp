// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Trigger.h"

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
//////////////////////////////////////////////////////////////////////////
CTrigger::CTrigger(
	uint32 const pathId_,
	int const numLoops_,
	double const sampleRate_,
	EEventType const eventType_,
	CryFixedStringT<MaxFilePathLength> const& filePath_,
	PaStreamParameters const& streamParameters_,
	CryFixedStringT<MaxFilePathLength> const& folder,
	CryFixedStringT<MaxFilePathLength> const& name,
	bool const isLocalized)
	: pathId(pathId_)
	, numLoops(numLoops_)
	, sampleRate(sampleRate_)
	, eventType(eventType_)
	, filePath(filePath_)
	, streamParameters(streamParameters_)
	, m_folder(folder)
	, m_name(name)
	, m_isLocalized(isLocalized)
{
}
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio
