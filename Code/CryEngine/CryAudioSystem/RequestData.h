// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

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
struct SRequestData : public _i_multithread_reference_target_t
{
	explicit SRequestData(ERequestType const requestType_)
		: requestType(requestType_)
	{}

	virtual ~SRequestData() override = default;

	SRequestData(SRequestData const&) = delete;
	SRequestData(SRequestData&&) = delete;
	SRequestData& operator=(SRequestData const&) = delete;
	SRequestData& operator=(SRequestData&&) = delete;

	ERequestType const requestType;
};

SRequestData* AllocateRequestData(SRequestData const* const pRequestData);
} // namespace CryAudio
