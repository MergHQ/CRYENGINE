// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySandbox/CrySignal.h>

namespace ACE
{
class CImplementationManager final
{
public:

	CImplementationManager(CImplementationManager const&) = delete;
	CImplementationManager(CImplementationManager&&) = delete;
	CImplementationManager& operator=(CImplementationManager const&) = delete;
	CImplementationManager& operator=(CImplementationManager&&) = delete;

	CImplementationManager() = default;
	~CImplementationManager();

	bool LoadImplementation();
	void Release();

	CCrySignal<void()> SignalOnBeforeImplementationChange;
	CCrySignal<void()> SignalOnAfterImplementationChange;

private:

	HMODULE m_hMiddlewarePlugin = nullptr;
};
} // namespace ACE
