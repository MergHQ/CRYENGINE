// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "RequestData.h"
#include "Common.h"
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
class CObject;

class CRequest final
{
public:

	CRequest() = default;

	explicit CRequest(
		SRequestData const* const pRequestData,
		ERequestFlags const flags_ = ERequestFlags::None,
		void* const pOwner_ = nullptr,
		void* const pUserData_ = nullptr,
		void* const pUserDataOwner_ = nullptr)
		: flags(flags_)
		, pOwner(pOwner_)
		, pUserData(pUserData_)
		, pUserDataOwner(pUserDataOwner_)
		, status(ERequestStatus::None)
		, pData(AllocateRequestData(pRequestData))
	{}

	explicit CRequest(
		SRequestData const* const pRequestData,
		SRequestUserData const& userData)
		: flags(userData.flags)
		, pOwner(userData.pOwner)
		, pUserData(userData.pUserData)
		, pUserDataOwner(userData.pUserDataOwner)
		, status(ERequestStatus::None)
		, pData(AllocateRequestData(pRequestData))
	{}

	SRequestData* GetData() const { return pData.get(); }

	ERequestFlags  flags = ERequestFlags::None;
	void*          pOwner = nullptr;
	void*          pUserData = nullptr;
	void*          pUserDataOwner = nullptr;
	ERequestStatus status = ERequestStatus::None;

private:

	// Must be private as it needs "AllocateRequestData"!
	std::shared_ptr<SRequestData> pData = nullptr;
};
} // namespace CryAudio
