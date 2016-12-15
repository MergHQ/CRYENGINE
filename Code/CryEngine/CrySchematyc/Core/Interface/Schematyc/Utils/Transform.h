// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/Forward.h>
#include <CrySerialization/Math.h>

#include "Schematyc/Reflection/Reflection.h"
#include "Schematyc/Utils/Rotation.h"

namespace Schematyc
{
class CTransform
{
public:

	inline CTransform()
		: m_translation(ZERO)
	{}

	explicit inline CTransform(const Vec3& translation, const CRotation& rotation)
		: m_translation(translation)
		, m_rotation(rotation)
	{}

	explicit inline CTransform(const QuatT& transform)
		: m_translation(transform.t)
		, m_rotation(transform.q)
	{}

	explicit inline CTransform(const Matrix34& transform)
		: m_translation(transform.GetTranslation())
		, m_rotation(Matrix33(transform))
	{}

	inline CTransform(const CTransform& rhs)
		: m_translation(rhs.m_translation)
		, m_rotation(rhs.m_rotation)
	{}

	inline void SetTranslation(const Vec3& translation)
	{
		m_translation = translation;
	}

	inline Vec3 GetTranslation() const
	{
		return m_translation;
	}

	inline void SetRotation(const CRotation& rotation)
	{
		m_rotation = rotation;
	}

	inline CRotation GetRotation() const
	{
		return m_rotation;
	}

	inline QuatT ToQuatT() const
	{
		return QuatT(m_rotation.ToQuat(), m_translation);
	}

	inline Matrix34 ToMatrix34() const
	{
		Matrix34 output = Matrix34::CreateRotationXYZ(m_rotation.ToAng3());
		output.SetTranslation(m_translation);
		return output;
	}

	inline void Serialize(Serialization::IArchive& archive)
	{
		archive(m_translation, "translation", "Translation");
		archive(m_rotation, "rotation", "Rotation");
	}

	inline void operator=(const QuatT& transform)
	{
		m_translation = transform.t;
		m_rotation = transform.q;
	}

	inline void operator=(const Matrix34& transform)
	{
		m_translation = transform.GetTranslation();
		m_rotation = Matrix33(transform);
	}

	static inline SGUID ReflectSchematycType(CTypeInfo<CTransform>& typeInfo)
	{
		return "a19ff444-7d11-49ea-80ea-5b629c44f588"_schematyc_guid;
	}

private:

	Vec3      m_translation;
	CRotation m_rotation;
};

namespace Transform
{
inline CTransform Multiply(const CTransform& a, const CTransform& b)
{
	return CTransform(a.ToQuatT() * b.ToQuatT());
}
}
} // Schematyc
