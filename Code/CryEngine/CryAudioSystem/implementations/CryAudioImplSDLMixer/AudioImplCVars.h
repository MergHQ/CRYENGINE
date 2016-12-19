// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
class CAudioImplCVars final
{
public:

	CAudioImplCVars() = default;
	CAudioImplCVars(CAudioImplCVars const&) = delete;
	CAudioImplCVars(CAudioImplCVars&&) = delete;
	CAudioImplCVars& operator=(CAudioImplCVars const&) = delete;
	CAudioImplCVars& operator=(CAudioImplCVars&&) = delete;

	void             RegisterVariables();
	void             UnregisterVariables();
};

extern CAudioImplCVars g_audioImplCVars;
}
}
}
