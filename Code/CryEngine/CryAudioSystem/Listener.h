// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IListener.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
struct IListener;
} // namespace Impl

class CListener final : public CryAudio::IListener
{
public:

	CListener() = delete;
	CListener(CListener const&) = delete;
	CListener(CListener&&) = delete;
	CListener& operator=(CListener const&) = delete;
	CListener& operator=(CListener&&) = delete;

	explicit CListener(Impl::IListener* const pImplData)
		: m_pImplData(pImplData)
	{}

	// CryAudio::IListener
	virtual void SetTransformation(CTransformation const& transformation, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	virtual void SetName(char const* const szName, SRequestUserData const& userData = SRequestUserData::GetEmptyObject()) override;
	// ~CryAudio::IListener

	void                   Update(float const deltaTime);
	void                   HandleSetTransformation(CTransformation const& transformation);
	CTransformation const& GetTransformation() const;

	Impl::IListener* m_pImplData;

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	void                   HandleSetName(char const* const szName);
	char const*            GetName() const                { return m_name.c_str(); }
	CTransformation const& GetDebugTransformation() const { return m_transformation; }

private:

	CTransformation                      m_transformation;
	CryFixedStringT<MaxObjectNameLength> m_name;
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
};
} // namespace CryAudio
