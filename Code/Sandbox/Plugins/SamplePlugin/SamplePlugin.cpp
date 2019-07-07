// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "SamplePlugin.h"

#include <CryCore/Platform/platform_impl.inl>
#include <Commands/ICommandManager.h>

namespace Private_SamplePlugin
{
	void SampleCommand()
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Sample Command executed");
	}
}

REGISTER_EDITOR_COMMAND(Private_SamplePlugin::SampleCommand, sample, sample_command, CCommandDescription(""));

REGISTER_PLUGIN(CSamplePlugin);
