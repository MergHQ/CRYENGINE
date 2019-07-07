// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __MNM_FIXED_AABB_H
#define __MNM_FIXED_AABB_H

#pragma once

#include "MNMFixedVec3.h"

namespace MNM
{
template<typename BaseType, size_t IntegerBitCount>
struct FixedAABB
{
	typedef FixedVec3<BaseType, IntegerBitCount> value_type;

	value_type min;
	value_type max;

	struct init_empty {};

	inline FixedAABB()
	{
	}

	inline FixedAABB(const init_empty&)
		: min(value_type::value_type::max())
		, max(value_type::value_type::min())
	{
	}

	inline FixedAABB(const value_type& _min, const value_type& _max)
		: min(_min)
		, max(_max)
	{
	}

	template<typename OBaseType, size_t OIntegerBitCount>
	inline FixedAABB(const FixedAABB<OBaseType, OIntegerBitCount>& other)
		: min(value_type(other.min))
		, max(value_type(other.max))
	{
	}

	bool empty() const
	{
		return min.x > max.x;
	}

	value_type size() const
	{
		return (max - min).abs();
	}

	value_type center() const
	{
		return (max + min) * value_type::value_type::fraction(1, 2);
	}

	template<typename BaseTypeO, size_t IntegerBitCountO>
	bool overlaps(const FixedAABB<BaseTypeO, IntegerBitCountO>& other) const
	{
		if ((min.x > typename value_type::value_type(other.max.x)) || (min.y > typename value_type::value_type(other.max.y))
		    || (min.z > typename value_type::value_type(other.max.z)) || (max.x < typename value_type::value_type(other.min.x))
		    || (max.y < typename value_type::value_type(other.min.y)) || (max.z < typename value_type::value_type(other.min.z)))
			return false;
		return true;
	}

	template<typename BaseTypeO, size_t IntegerBitCountO>
	bool overlaps(const FixedVec3<BaseTypeO, IntegerBitCountO>& tv0, const FixedVec3<BaseTypeO, IntegerBitCountO>& tv1, const FixedVec3<BaseTypeO, IntegerBitCountO>& tv2) const
	{
		typedef FixedVec3<BaseTypeO, IntegerBitCountO> TFixedVec3;

		//------ convert AABB into half-length AABB -----------
		const TFixedVec3 h = (max - min) / typename value_type::value_type(2); //calculate the half-length-vectors
		const TFixedVec3 c = (max + min) / typename value_type::value_type(2); //the center is relative to the PIVOT

		//move everything so that the boxcenter is in (0,0,0)
		const TFixedVec3 v0 = tv0 - c;
		const TFixedVec3 v1 = tv1 - c;
		const TFixedVec3 v2 = tv2 - c;

		//compute triangle edges
		const TFixedVec3 e0 = v1 - v0;
		const TFixedVec3 e1 = v2 - v1;
		const TFixedVec3 e2 = v0 - v2;

		//--------------------------------------------------------------------------------------
		//    use SEPARATING AXIS THEOREM to test overlap  between AABB and triangle
		//    cross-product(edge from triangle, {x,y,z}-direction),  this are 3x3=9 tests
		//--------------------------------------------------------------------------------------
		typename value_type::value_type min, max, p0, p1, p2, rad, fex, fey, fez;
		fex = fabsf(e0.x);
		fey = fabsf(e0.y);
		fez = fabsf(e0.z);

		//AXISTEST_X01(e0.z, e0.y, fez, fey);
		p0 = e0.z * v0.y - e0.y * v0.z;
		p2 = e0.z * v2.y - e0.y * v2.z;
		if (p0 < p2) { min = p0; max = p2; }
		else { min = p2; max = p0; }
		rad = fez * h.y + fey * h.z;
		if (min > rad || max < -rad)
		{
			return false;
		}
		//AXISTEST_Y02(e0.z, e0.x, fez, fex);
		p0 = -e0.z * v0.x + e0.x * v0.z;
		p2 = -e0.z * v2.x + e0.x * v2.z;
		if (p0 < p2) { min = p0; max = p2; }
		else { min = p2; max = p0; }
		rad = fez * h.x + fex * h.z;
		if (min > rad || max < -rad)
		{
			return false;
		}

		//AXISTEST_Z12(e0.y, e0.x, fey, fex);
		p1 = e0.y * v1.x - e0.x * v1.y;
		p2 = e0.y * v2.x - e0.x * v2.y;
		if (p2 < p1) { min = p2; max = p1; }
		else { min = p1; max = p2; }
		rad = fey * h.x + fex * h.y;
		if (min > rad || max < -rad)
		{
			return false;
		}

		//-----------------------------------------------

		fex = fabsf(e1.x);
		fey = fabsf(e1.y);
		fez = fabsf(e1.z);
		//AXISTEST_X01(e1.z, e1.y, fez, fey);
		p0 = e1.z * v0.y - e1.y * v0.z;
		p2 = e1.z * v2.y - e1.y * v2.z;
		if (p0 < p2) { min = p0; max = p2; }
		else { min = p2; max = p0; }
		rad = fez * h.y + fey * h.z;
		if (min > rad || max < -rad)
		{
			return false;
		}

		//AXISTEST_Y02(e1.z, e1.x, fez, fex);
		p0 = -e1.z * v0.x + e1.x * v0.z;
		p2 = -e1.z * v2.x + e1.x * v2.z;
		if (p0 < p2) { min = p0; max = p2; }
		else { min = p2; max = p0; }
		rad = fez * h.x + fex * h.z;
		if (min > rad || max < -rad)
		{
			return false;
		}

		//AXISTEST_Z0(e1.y, e1.x, fey, fex);
		p0 = e1.y * v0.x - e1.x * v0.y;
		p1 = e1.y * v1.x - e1.x * v1.y;
		if (p0 < p1) { min = p0; max = p1; }
		else { min = p1; max = p0; }
		rad = fey * h.x + fex * h.y;
		if (min > rad || max < -rad)
		{
			return false;
		}

		//-----------------------------------------------

		fex = fabsf(e2.x);
		fey = fabsf(e2.y);
		fez = fabsf(e2.z);
		//AXISTEST_X2(e2.z, e2.y, fez, fey);
		p0 = e2.z * v0.y - e2.y * v0.z;
		p1 = e2.z * v1.y - e2.y * v1.z;
		if (p0 < p1) { min = p0; max = p1; }
		else { min = p1; max = p0; }
		rad = fez * h.y + fey * h.z;
		if (min > rad || max < -rad)
		{
			return false;
		}

		//AXISTEST_Y1(e2.z, e2.x, fez, fex);
		p0 = -e2.z * v0.x + e2.x * v0.z;
		p1 = -e2.z * v1.x + e2.x * v1.z;
		if (p0 < p1) { min = p0; max = p1; }
		else { min = p1; max = p0; }
		rad = fez * h.x + fex * h.z;
		if (min > rad || max < -rad)
		{
			return false;
		}

		//AXISTEST_Z12(e2.y, e2.x, fey, fex);
		p1 = e2.y * v1.x - e2.x * v1.y;
		p2 = e2.y * v2.x - e2.x * v2.y;
		if (p2 < p1) { min = p2; max = p1; }
		else { min = p1; max = p2; }
		rad = fey * h.x + fex * h.y;
		if (min > rad || max < -rad)
		{
			return false;
		}

		//the {x,y,z}-directions (actually, since we use the AABB of the triangle we don't even need to test these)
		//first test overlap in the {x,y,z}-directions
		//find min, max of the triangle each direction, and test for overlap in that direction --
		//this is equivalent to testing a minimal AABB around the triangle against the AABB

		const FixedAABB<BaseTypeO, IntegerBitCountO> taabb(TFixedVec3::minimize(v0, v1, v2), TFixedVec3::maximize(v0, v1, v2));

		//test in X-direction
		if (taabb.min.x > h.x || taabb.max.x < -h.x)
		{
			return false;
		}

		//test in Y-direction
		if (taabb.min.y > h.y || taabb.max.y < -h.y)
		{
			return false;
		}

		//test in Z-direction
		if (taabb.min.z > h.z || taabb.max.z < -h.z)
		{
			return false;
		}

		//test if the box intersects the plane of the triangle
		//compute plane equation of triangle: normal*x+d=0

		const TFixedVec3 normal(e0.cross(e1));
		const Plane_tpl<int> plane = Plane_tpl<int>::CreatePlane(normal.GetVec3(), v0.GetVec3());

		const int hx = h.x.as_int();
		const int hy = h.y.as_int();
		const int hz = h.z.as_int();

		Vec3_tpl<int> vmin, vmax;
		if (plane.n.x > 0) { vmin.x = -hx; vmax.x = hx; }
		else { vmin.x = hx; vmax.x = -hx; }
		if (plane.n.y > 0) { vmin.y = -hy; vmax.y = hy; }
		else { vmin.y = hy; vmax.y = -hy; }
		if (plane.n.z > 0) { vmin.z = -hz; vmax.z = hz; }
		else { vmin.z = hz;  vmax.z = -hz; }

		if ((plane | vmin) > 0)
		{
			return false;
		}

		if ((plane | vmax) < 0)
		{
			return false;
		}

		return true;
	}

	template<typename BaseTypeO, size_t IntegerBitCountO>
	bool contains(const FixedAABB<BaseTypeO, IntegerBitCountO>& other) const
	{
		if (min.x <= typename value_type::value_type(other.min.x) &&
			min.y <= typename value_type::value_type(other.min.y) &&
			min.z <= typename value_type::value_type(other.min.z) &&
			max.x >= typename value_type::value_type(other.max.x) &&
			max.y >= typename value_type::value_type(other.max.y) &&
			max.z >= typename value_type::value_type(other.max.z))
		{
			return true;
		}
		return false;
	}

	template<typename BaseTypeO, size_t IntegerBitCountO>
	bool contains(const FixedVec3<BaseTypeO, IntegerBitCountO>& other) const
	{
		if (min.x <= typename value_type::value_type(other.x) &&
			min.y <= typename value_type::value_type(other.y) &&
			min.z <= typename value_type::value_type(other.z) &&
			max.x >= typename value_type::value_type(other.x) &&
			max.y >= typename value_type::value_type(other.y) &&
			max.z >= typename value_type::value_type(other.z))
		{
			return true;
		}
		return false;
	}

	template<typename BaseTypeO, size_t IntegerBitCountO>
	inline FixedAABB& operator+=(const FixedVec3<BaseTypeO, IntegerBitCountO>& other)
	{
		min = value_type::minimize(min, other);
		max = value_type::maximize(max, other);
		return *this;
	}

	template<typename BaseTypeO, size_t IntegerBitCountO>
	inline FixedAABB operator+(const FixedVec3<BaseTypeO, IntegerBitCountO>& other) const
	{
		FixedAABB r(*this);
		r += other;
		return r;
	}

	template<typename BaseTypeO, size_t IntegerBitCountO>
	inline FixedAABB& operator+=(const FixedAABB<BaseTypeO, IntegerBitCountO>& other)
	{
		min = value_type::minimize(min, other.min);
		max = value_type::maximize(max, other.max);
		return *this;
	}

	template<typename BaseTypeO, size_t IntegerBitCountO>
	inline FixedAABB operator+(const FixedAABB<BaseTypeO, IntegerBitCountO>& other) const
	{
		FixedAABB r(*this);
		r += other;
		return r;
	}

	template<typename BaseTypeO, size_t IntegerBitCountO>
	inline FixedAABB& operator*=(const fixed_t<BaseTypeO, IntegerBitCountO>& x)
	{
		min *= x;
		max *= x;
		return *this;
	}

	template<typename BaseTypeO, size_t IntegerBitCountO>
	inline FixedAABB operator*(const fixed_t<BaseTypeO, IntegerBitCountO>& other) const
	{
		FixedAABB r(*this);
		r *= other;
		return r;
	}

	AUTO_STRUCT_INFO;
};
}

#endif  // #ifndef __MNM_FIXED_AABB_H
