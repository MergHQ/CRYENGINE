// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/IImpl.h"
#include <QObject>
#include <CrySandbox/CrySignal.h>

namespace ACE
{
extern Impl::IImpl* g_pIImpl;

class CImplementationManager final : public QObject
{
	Q_OBJECT

public:

	CImplementationManager(CImplementationManager const&) = delete;
	CImplementationManager(CImplementationManager&&) = delete;
	CImplementationManager& operator=(CImplementationManager const&) = delete;
	CImplementationManager& operator=(CImplementationManager&&) = delete;

	CImplementationManager() = default;
	virtual ~CImplementationManager() override;

	bool LoadImplementation();
	void Release();

	CCrySignal<void()> SignalOnBeforeImplementationChange;
	CCrySignal<void()> SignalOnAfterImplementationChange;

private:

	HMODULE m_hMiddlewarePlugin = nullptr;
};
} // namespace ACE
