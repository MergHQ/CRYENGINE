// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
using ListenerId = uint;

class CListener final : public IListener
{
public:

	CListener() = delete;
	CListener(CListener const&) = delete;
	CListener(CListener&&) = delete;
	CListener& operator=(CListener const&) = delete;
	CListener& operator=(CListener&&) = delete;

	explicit CListener(CObjectTransformation const& transformation, ListenerId const id);
	virtual ~CListener() override = default;

	// CryAudio::Impl::IListener
	virtual void                         Update(float const deltaTime) override {}
	virtual void                         SetName(char const* const szName) override;
	virtual void                         SetTransformation(CObjectTransformation const& transformation) override;
	virtual CObjectTransformation const& GetTransformation() const override { return m_transformation; }
	// ~CryAudio::Impl::IListener

#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
	char const* GetName() const { return m_name.c_str(); }
#endif  // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE

private:

	ListenerId const      m_id;
	CObjectTransformation m_transformation;

#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxObjectNameLength> m_name;
#endif  // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE
};
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio