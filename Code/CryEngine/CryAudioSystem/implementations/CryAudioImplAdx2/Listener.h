// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>
#include <PoolObject.h>

#include "Common.h"

#include <cri_atom_ex.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
class CListener final : public IListener
{
public:

	CListener() = delete;
	CListener(CListener const&) = delete;
	CListener(CListener&&) = delete;
	CListener& operator=(CListener const&) = delete;
	CListener& operator=(CListener&&) = delete;

	explicit CListener(CObjectTransformation const& transformation, uint16 const id, CriAtomEx3dListenerHn const pHandle);
	virtual ~CListener() override = default;

	// CryAudio::Impl::IListener
	virtual void                         Update(float const deltaTime) override;
	virtual void                         SetName(char const* const szName) override;
	virtual void                         SetTransformation(CObjectTransformation const& transformation) override;
	virtual CObjectTransformation const& GetTransformation() const override { return m_transformation; }
	// ~CryAudio::Impl::IListener

	uint16                GetId() const       { return m_id; }
	CriAtomEx3dListenerHn GetHandle() const   { return m_pHandle; }
	Vec3 const&           GetPosition() const { return m_position; }
	Vec3 const&           GetVelocity() const { return m_velocity; }

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	char const* GetName() const { return m_name.c_str(); }
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

private:

	void SetVelocity();

	uint16 const                m_id;
	CriAtomEx3dListenerHn const m_pHandle;
	bool                        m_isMovingOrDecaying;
	Vec3                        m_velocity;
	Vec3                        m_position;
	Vec3                        m_previousPosition;
	CObjectTransformation       m_transformation;
	S3DAttributes               m_3dAttributes;

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxObjectNameLength> m_name;
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
