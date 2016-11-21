// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// #SchematycTODO : Move serialization utils to separate header?

#pragma once

#include <CrySerialization/Forward.h>
#include <CrySerialization/Math.h>
#include <CrySerialization/Decorators/Range.h>

#include "Schematyc/Reflection/Reflection.h"

namespace Schematyc
{
namespace SerializationUtils
{
template<int32 MIN_X, int32 MAX_X, int32 MIN_Y = MIN_X, int32 MAX_Y = MAX_X, int32 MIN_Z = MIN_Y, int32 MAX_Z = MAX_Y>
class CRadianAnglesDecorator
{
public:

	CRadianAnglesDecorator(Ang3& _angles)
		: m_angles(_angles)
	{}

	void Serialize(Serialization::IArchive& archive)
	{
		m_angles.x = RAD2DEG(m_angles.x);
		m_angles.y = RAD2DEG(m_angles.y);
		m_angles.z = RAD2DEG(m_angles.z);

		archive(Serialization::Range(m_angles.x, float(MIN_X), float(MAX_X)), "deg.x", "^");
		archive.doc("Rotation around the X axis in degrees.");
		archive(Serialization::Range(m_angles.y, float(MIN_Y), float(MAX_Y)), "deg.y", "^");
		archive.doc("Rotation around the Y axis in degrees.");
		archive(Serialization::Range(m_angles.z, float(MIN_Z), float(MAX_Z)), "deg.z", "^");
		archive.doc("Rotation around the Z axis in degrees.");

		m_angles.x = DEG2RAD(m_angles.x);
		m_angles.y = DEG2RAD(m_angles.y);
		m_angles.z = DEG2RAD(m_angles.z);
	}

private:

	Ang3& m_angles;
};

template<int32 MIN_X, int32 MAX_X, int32 MIN_Y, int32 MAX_Y, int32 MIN_Z, int32 MAX_Z>
inline CRadianAnglesDecorator<MIN_X, MAX_X, MIN_Y, MAX_Y, MIN_Z, MAX_Z> RadianAngleDecorator(Ang3& value)
{
	return CRadianAnglesDecorator<MIN_X, MAX_X, MIN_Y, MAX_Y, MIN_Z, MAX_Z>(value);
}

template<int32 MIN_X, int32 MAX_X, int32 MIN_Y, int32 MAX_Y>
inline CRadianAnglesDecorator<MIN_X, MAX_X, MIN_Y, MAX_Y> RadianAnglesDecorator(Ang3& value)
{
	return CRadianAnglesDecorator<MIN_X, MAX_X, MIN_Y, MAX_Y>(value);
}

template<int32 MIN_X, int32 MAX_X>
inline CRadianAnglesDecorator<MIN_X, MAX_X> RadianAnglesDecorator(Ang3& value)
{
	return CRadianAnglesDecorator<MIN_X, MAX_X>(value);
}
} // SerializationUtils

class CRotation
{
private:

	enum class EFormat : uint32
	{
		Ang3,
		Quat
	};

public:

	inline CRotation()
		: m_format(EFormat::Ang3)
		, m_value(0.0f, 0.0f, 0.0f, 0.0f)
	{}

	explicit inline CRotation(const Ang3& angles)
		: m_format(EFormat::Ang3)
		, m_value(0.0f, angles.x, angles.y, angles.z)
	{}

	explicit inline CRotation(const Quat& rotation)
		: m_format(EFormat::Quat)
		, m_value(rotation)
	{}

	explicit inline CRotation(const Matrix33& rotation)
		: m_format(EFormat::Quat)
		, m_value(rotation)
	{}

	inline CRotation(const CRotation& rhs)
		: m_format(rhs.m_format)
		, m_value(rhs.m_value)
	{}

	inline Ang3 ToAng3() const
	{
		if (m_format == EFormat::Quat)
		{
			return Ang3(m_value);
		}
		else
		{
			return Ang3(m_value.v.x, m_value.v.y, m_value.v.z);
		}
	}

	inline Quat ToQuat() const
	{
		if (m_format == EFormat::Ang3)
		{
			return Quat(Ang3(m_value.v.x, m_value.v.y, m_value.v.z));
		}
		else
		{
			return m_value;
		}
	}

	inline Matrix33 ToMatrix33() const
	{
		return Matrix33::CreateRotationXYZ(ToAng3());
	}

	inline void operator=(const Ang3& angles)
	{
		m_format = EFormat::Ang3;
		m_value = Quat(0.0f, angles.x, angles.y, angles.z);
	}

	inline void operator=(const Quat& rotation)
	{
		m_format = EFormat::Quat;
		m_value = rotation;
	}

	inline void operator=(const Matrix33& rotation)
	{
		m_format = EFormat::Quat;
		m_value = Quat(rotation);
	}

	static inline SGUID ReflectSchematycType(CTypeInfo<CRotation>& typeInfo)
	{
		typeInfo.DeclareSerializeable();
		return "991960c1-3b34-45cc-ac3c-f3f0e55ed7ef"_schematyc_guid;
	}

private:

	EFormat m_format;
	Quat    m_value;
};

inline bool Serialize(Serialization::IArchive& archive, CRotation& value, const char* szName, const char* szLabel)
{
	// #SchematycTODO : Need to make sure rotations in Hunt data are converted to angles.
	Ang3 temp = archive.isOutput() ? value.ToAng3() : Ang3(ZERO);
	if (archive(SerializationUtils::RadianAnglesDecorator<0, 360>(temp), szName, szLabel))
	{
		if (archive.isInput())
		{
			value = temp;
		}
		return true;
	}
	return false;
}

namespace Rotation
{
inline CRotation Multiply(const CRotation& a, const CRotation& b)
{
	return CRotation(a.ToQuat() * b.ToQuat());
}
}
} // Schematyc
