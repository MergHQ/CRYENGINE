// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
class CListener final : public IListener
{
public:

	CListener() = default;

	CListener(CListener const&) = delete;
	CListener(CListener&&) = delete;
	CListener& operator=(CListener const&) = delete;
	CListener& operator=(CListener&&) = delete;

	// CryAudio::Impl::IListener
	virtual void                         Update(float const deltaTime) override                                  {}
	virtual void                         SetName(char const* const szName) override;
	virtual void                         SetTransformation(CObjectTransformation const& transformation) override { m_transformation = transformation; }
	virtual CObjectTransformation const& GetTransformation() const override                                      { return m_transformation; }
	// ~CryAudio::Impl::IListener

#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
	char const* GetName() const { return m_name.c_str(); }
#endif  // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE

private:

	CObjectTransformation m_transformation;

#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxObjectNameLength> m_name;
#endif  // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE
};

class CParameter final : public IParameter
{
public:

	CParameter() = default;
	CParameter(CParameter const&) = delete;
	CParameter(CParameter&&) = delete;
	CParameter& operator=(CParameter const&) = delete;
	CParameter& operator=(CParameter&&) = delete;
};

class CSwitchState final : public ISwitchState
{
public:

	CSwitchState() = default;
	CSwitchState(CSwitchState const&) = delete;
	CSwitchState(CSwitchState&&) = delete;
	CSwitchState& operator=(CSwitchState const&) = delete;
	CSwitchState& operator=(CSwitchState&&) = delete;
};

class CEnvironment final : public IEnvironment
{
public:

	CEnvironment() = default;
	CEnvironment(CEnvironment const&) = delete;
	CEnvironment(CEnvironment&&) = delete;
	CEnvironment& operator=(CEnvironment const&) = delete;
	CEnvironment& operator=(CEnvironment&&) = delete;
};

class CFile final : public IFile
{
public:

	CFile() = default;
	CFile(CFile const&) = delete;
	CFile(CFile&&) = delete;
	CFile& operator=(CFile const&) = delete;
	CFile& operator=(CFile&&) = delete;
};

class CStandaloneFile final : public IStandaloneFile
{
public:

	CStandaloneFile() = default;
	CStandaloneFile(CStandaloneFile const&) = delete;
	CStandaloneFile(CStandaloneFile&&) = delete;
	CStandaloneFile& operator=(CStandaloneFile const&) = delete;
	CStandaloneFile& operator=(CStandaloneFile&&) = delete;
};
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio
