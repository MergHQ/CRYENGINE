// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>
#include <PoolObject.h>

#include "Common.h"

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
class CListener final : public IListener
{
public:

	explicit CListener(uint16 const id, CriAtomEx3dListenerHn const pHandle);
	virtual ~CListener() override = default;

	CListener() = delete;
	CListener(CListener const&) = delete;
	CListener(CListener&&) = delete;
	CListener& operator=(CListener const&) = delete;
	CListener& operator=(CListener&&) = delete;

	// CryAudio::Impl::IListener
	virtual void SetTransformation(CObjectTransformation const& transformation) override;
	// ~CryAudio::Impl::IListener

	uint16                GetId() const     { return m_id; }
	CriAtomEx3dListenerHn GetHandle() const { return m_pHandle; }

private:

	uint16 const                m_id;
	CriAtomEx3dListenerHn const m_pHandle;
	STransformation             m_transformation;
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
