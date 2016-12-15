// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
class CAudioListener final : public IAudioListener
{
public:

	CAudioListener() = default;
	virtual ~CAudioListener() override = default;

	CAudioListener(CAudioListener const&) = delete;
	CAudioListener(CAudioListener&&) = delete;
	CAudioListener& operator=(CAudioListener const&) = delete;
	CAudioListener& operator=(CAudioListener&&) = delete;
};

class CAudioParameter final : public IAudioRtpc
{
public:

	CAudioParameter() = default;
	virtual ~CAudioParameter() override = default;

	CAudioParameter(CAudioParameter const&) = delete;
	CAudioParameter(CAudioParameter&&) = delete;
	CAudioParameter& operator=(CAudioParameter const&) = delete;
	CAudioParameter& operator=(CAudioParameter&&) = delete;
};

class CAudioSwitchState final : public IAudioSwitchState
{
public:

	CAudioSwitchState() = default;
	virtual ~CAudioSwitchState() override = default;

	CAudioSwitchState(CAudioSwitchState const&) = delete;
	CAudioSwitchState(CAudioSwitchState&&) = delete;
	CAudioSwitchState& operator=(CAudioSwitchState const&) = delete;
	CAudioSwitchState& operator=(CAudioSwitchState&&) = delete;
};

class CAudioEnvironment final : public IAudioEnvironment
{
public:

	CAudioEnvironment() = default;
	virtual ~CAudioEnvironment() override = default;

	CAudioEnvironment(CAudioEnvironment const&) = delete;
	CAudioEnvironment(CAudioEnvironment&&) = delete;
	CAudioEnvironment& operator=(CAudioEnvironment const&) = delete;
	CAudioEnvironment& operator=(CAudioEnvironment&&) = delete;
};

class CAudioFileEntry final : public IAudioFileEntry
{
public:

	CAudioFileEntry() = default;
	virtual ~CAudioFileEntry() override = default;

	CAudioFileEntry(CAudioFileEntry const&) = delete;
	CAudioFileEntry(CAudioFileEntry&&) = delete;
	CAudioFileEntry& operator=(CAudioFileEntry const&) = delete;
	CAudioFileEntry& operator=(CAudioFileEntry&&) = delete;
};

class CAudioStandaloneFile final : public IAudioStandaloneFile
{
public:

	CAudioStandaloneFile() = default;
	virtual ~CAudioStandaloneFile() override = default;

	CAudioStandaloneFile(CAudioStandaloneFile const&) = delete;
	CAudioStandaloneFile(CAudioStandaloneFile&&) = delete;
	CAudioStandaloneFile& operator=(CAudioStandaloneFile const&) = delete;
	CAudioStandaloneFile& operator=(CAudioStandaloneFile&&) = delete;
};
}
}
}
