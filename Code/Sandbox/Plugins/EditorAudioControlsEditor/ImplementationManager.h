// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <QObject>
#include <CrySandbox/CrySignal.h>

namespace ACE
{
struct IEditorImpl;
extern IEditorImpl* g_pEditorImpl;

class CImplementationManager final : public QObject
{
	Q_OBJECT

public:

	CImplementationManager();
	virtual ~CImplementationManager() override;

	bool LoadImplementation();
	void Release();

	CCrySignal<void()> SignalImplementationAboutToChange;
	CCrySignal<void()> SignalImplementationChanged;

private:

	HMODULE m_hMiddlewarePlugin;
};
} // namespace ACE
