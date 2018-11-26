// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IStandaloneFileConnection.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
class CStandaloneFile final : public IStandaloneFileConnection
{
public:

	CStandaloneFile(CStandaloneFile const&) = delete;
	CStandaloneFile(CStandaloneFile&&) = delete;
	CStandaloneFile& operator=(CStandaloneFile const&) = delete;
	CStandaloneFile& operator=(CStandaloneFile&&) = delete;

	CStandaloneFile() = default;
	virtual ~CStandaloneFile() override = default;

	// IStandaloneFileConnection
	virtual ERequestStatus Play(IObject* const pIObject) override { return ERequestStatus::Failure; }
	virtual ERequestStatus Stop(IObject* const pIObject) override { return ERequestStatus::Failure; }
	// ~IStandaloneFileConnection
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
