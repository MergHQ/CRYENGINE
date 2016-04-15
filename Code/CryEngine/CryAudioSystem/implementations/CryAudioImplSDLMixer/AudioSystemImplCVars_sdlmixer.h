// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
namespace Impl
{

class CAudioSystemImplCVars
{
public:

	CAudioSystemImplCVars();
	~CAudioSystemImplCVars();

	void RegisterVariables();
	void UnregisterVariables();

	int m_nPrimaryPoolSize;

	PREVENT_OBJECT_COPY(CAudioSystemImplCVars);
};

extern CAudioSystemImplCVars g_audioImplCVars_sdlmixer;

}
}
