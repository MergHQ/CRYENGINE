// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "RequestData.h"
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
class CObject;

class CRequest final
{
public:

	CRequest() = default;

	explicit CRequest(SRequestData const* const pRequestData)
		: flags(ERequestFlags::None)
		, pObject(nullptr)
		, pOwner(nullptr)
		, pUserData(nullptr)
		, pUserDataOwner(nullptr)
		, status(ERequestStatus::None)
		, pData(AllocateRequestData(pRequestData))
	{}

	explicit CRequest(
		ERequestFlags const flags_,
		CObject* const pObject_,
		void* const pOwner_,
		void* const pUserData_,
		void* const pUserDataOwner_,
		SRequestData const* const pRequestData_)
		: flags(flags_)
		, pObject(pObject_)
		, pOwner(pOwner_)
		, pUserData(pUserData_)
		, pUserDataOwner(pUserDataOwner_)
		, status(ERequestStatus::None)
		, pData(AllocateRequestData(pRequestData_))
	{}

	SRequestData* GetData() const { return pData.get(); }

	ERequestFlags  flags = ERequestFlags::None;
	CObject*       pObject = nullptr;
	void*          pOwner = nullptr;
	void*          pUserData = nullptr;
	void*          pUserDataOwner = nullptr;
	ERequestStatus status = ERequestStatus::None;

private:

	// Must be private as it needs "AllocateRequestData"!
	_smart_ptr<SRequestData> pData = nullptr;
};
} // namespace CryAudio
