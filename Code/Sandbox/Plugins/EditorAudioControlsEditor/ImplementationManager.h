// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <QObject>
#include <CrySandbox/CrySignal.h>

namespace ACE
{
class IAudioSystemEditor;
}

class CImplementationManager : public QObject
{
	Q_OBJECT

public:
	CImplementationManager();
	virtual ~CImplementationManager();

	bool                     LoadImplementation();
	void                     Release();
	ACE::IAudioSystemEditor* GetImplementation();

	CCrySignal<void()> signalImplementationAboutToChange;
	CCrySignal<void()> signalImplementationChanged;

private:
	ACE::IAudioSystemEditor* ms_pAudioSystemImpl;
	HMODULE                  ms_hMiddlewarePlugin;
};
