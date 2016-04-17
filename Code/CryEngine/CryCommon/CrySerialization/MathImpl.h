// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/IArchive.h>

#include <CryMath/Cry_Vector2.h>
#include <CryMath/Cry_Vector3.h>
#include <CryMath/Cry_Vector4.h>
#include <CryMath/Cry_Quat.h>
#include <CryMath/Cry_Matrix34.h>
#include <CryMath/Cry_Geo.h>

template<typename T>
bool Serialize(Serialization::IArchive& ar, Vec2_tpl<T>& value, const char* name, const char* label)
{
	typedef T (& Array)[2];
	return ar((Array)value, name, label);
}

template<typename T>
bool Serialize(Serialization::IArchive& ar, Vec3_tpl<T>& value, const char* name, const char* label)
{
	typedef T (& Array)[3];
	return ar((Array)value, name, label);
}

template<typename T>
inline bool Serialize(Serialization::IArchive& ar, struct Vec4_tpl<T>& v, const char* name, const char* label)
{
	typedef T (& Array)[4];
	return ar((Array)v, name, label);
}

template<typename T>
bool Serialize(Serialization::IArchive& ar, struct Quat_tpl<T>& value, const char* name, const char* label)
{
	if (ar.isEdit())
	{
		typedef T (& Array)[3];
		Vec3_tpl<T> v = Vec3_tpl<T>(RAD2DEG(Ang3_tpl<T>::GetAnglesXYZ(Matrix33_tpl<T>(value))));
		bool result = ar((Array)v, name, label);
		if (ar.isInput())
			value = Quat_tpl<T>(Ang3_tpl<T>(DEG2RAD(v)));
		return result;
	}
	else
	{
		typedef T (& Array)[4];
		return ar((Array)value, name, label);
	}
}

template<typename T>
struct SerializableQuatT : QuatT_tpl<T>
{
	void Serialize(Serialization::IArchive& ar)
	{
		ar(this->q, "q", "Quaternion");
		ar(this->t, "t", "Translation");
	}
};

template<typename T>
bool Serialize(Serialization::IArchive& ar, struct QuatT_tpl<T>& value, const char* name, const char* label)
{
	return Serialize(ar, static_cast<SerializableQuatT<T>&>(value), name, label);
}

struct SerializableAABB : AABB
{
	void Serialize(Serialization::IArchive& ar)
	{
		ar(this->min, "min", "Min");
		ar(this->max, "max", "Max");
	}
};

inline bool Serialize(Serialization::IArchive& ar, struct AABB& value, const char* name, const char* label)
{
	return Serialize(ar, static_cast<SerializableAABB&>(value), name, label);
}

template<typename T>
bool Serialize(Serialization::IArchive& ar, Matrix34_tpl<T>& value, const char* name, const char* label)
{
	typedef T (& Array)[3][4];
	return ar((Array)value, name, label);
}

//////////////////////////////////////////////////////////////////////////

namespace Serialization
{

template<class T>
bool Serialize(Serialization::IArchive& ar, Serialization::SRadiansAsDeg<T>& value, const char* name, const char* label)
{
	if (ar.isEdit())
	{
		float degrees = RAD2DEG(*value.radians);
		float oldDegrees = degrees;
		if (!ar(degrees, name, label))
			return false;
		if (oldDegrees != degrees)
			*value.radians = DEG2RAD(degrees);
		return true;
	}
	else
		return ar(*value.radians, name, label);
}

template<class T>
bool Serialize(Serialization::IArchive& ar, Serialization::SRadianAng3AsDeg<T>& value, const char* name, const char* label)
{
	if (ar.isEdit())
	{
		Ang3 degrees(RAD2DEG(value.ang3->x), RAD2DEG(value.ang3->y), RAD2DEG(value.ang3->z));
		Ang3 oldDegrees = degrees;
		if (!ar(degrees, name, label))
			return false;
		if (oldDegrees != degrees)
			*value.ang3 = Ang3(DEG2RAD(degrees.x), DEG2RAD(degrees.y), DEG2RAD(degrees.z));
		return true;
	}
	else
		return ar(*value.ang3, name, label);
}

}

//////////////////////////////////////////////////////////////////////////

namespace Serialization
{

template<class T>
bool SerializeRadiusAsDiameter(Serialization::IArchive& ar, T* radius, const char* name, const char* label)
{
	if (ar.isEdit())
	{
		T diameter = *radius * T(2);
		T oldDiameter = diameter;
		if (!ar(diameter, name, label))
			return false;
		if (oldDiameter != diameter)
			*radius = diameter / T(2);
		return true;
	}
	else
		return ar(*radius, name, label);
}

template<class T>
bool Serialize(Serialization::IArchive& ar, Serialization::SRadiusAsDiameter<T>& value, const char* name, const char* label)
{
	return SerializeRadiusAsDiameter(ar, value.radius, name, label);
}

template<class T>
bool Serialize(Serialization::IArchive& ar, Serialization::SRadiusWithRangeAsDiameter<T>& value, const char* name, const char* label)
{
	if (!SerializeRadiusAsDiameter(ar, value.radius, name, label))
		return false;

	if (ar.isInput())
	{
		if (*value.radius < value.hardMin)
			*value.radius = value.hardMin;
		if (*value.radius > value.hardMax)
			*value.radius = value.hardMax;
	}
	return true;
}
}

//////////////////////////////////////////////////////////////////////////

template<typename T>
bool Serialize(Serialization::IArchive& ar, Ang3_tpl<T>& value, const char* name, const char* label)
{
	typedef T (& Array)[3];
	return ar((Array)value, name, label);
}

//////////////////////////////////////////////////////////////////////////

namespace Serialization
{

template<class T>
bool Serialize(Serialization::IArchive& ar, Serialization::QuatAsAng3<T>& value, const char* name, const char* label)
{
	if (ar.isEdit())
	{
		Ang3 ang3(*value.quat);
		Ang3 oldAng3 = ang3;
		if (!ar(Serialization::RadiansAsDeg(ang3), name, label))
			return false;
		if (ang3 != oldAng3)
			*value.quat = Quat(ang3);
		return true;
	}
	else
		return ar(*value.quat, name, label);
}

template<class T>
bool Serialize(Serialization::IArchive& ar, Serialization::QuatTAsVec3Ang3<T>& value, const char* name, const char* label)
{
	if (ar.isEdit())
	{
		if (!ar.openBlock(name, label))
			return false;

		ar(QuatAsAng3<T>((value.trans)->q), "rot", "Rotation");
		ar.doc("Euler Angles in degrees");
		ar((value.trans)->t, "t", "Translation");
		ar.closeBlock();
		return true;
	}
	else
	{
		return ar(*(value.trans), name, label);
	}
}

}
