// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
enum class ERequestType : EnumFlagsType
{
	None,
	SystemRequest,
	CallbackRequest,
	ObjectRequest,
	ListenerRequest,
};

//////////////////////////////////////////////////////////////////////////
struct SRequestData
{
	explicit SRequestData(ERequestType const requestType_)
		: requestType(requestType_)
	{}

	virtual ~SRequestData() = default;

	SRequestData(SRequestData const&) = delete;
	SRequestData(SRequestData&&) = delete;
	SRequestData& operator=(SRequestData const&) = delete;
	SRequestData& operator=(SRequestData&&) = delete;

	ERequestType const requestType;
};

std::shared_ptr<SRequestData> AllocateRequestData(SRequestData const* const pRequestData);
} // namespace CryAudio
