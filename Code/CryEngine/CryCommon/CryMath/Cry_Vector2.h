// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef CRYTEK_CRYVECTOR2_H
#define CRYTEK_CRYVECTOR2_H

template<typename T> struct Vec2_tpl;
template<typename T> struct Vec2Constants
{
	static Vec2_tpl<T> fVec2_Zero;
	static Vec2_tpl<T> fVec2_OneX;
	static Vec2_tpl<T> fVec2_OneY;
	static Vec2_tpl<T> fVec2_OneZ;
	static Vec2_tpl<T> fVec2_One;
};

template<class F> struct Vec2_tpl
{
	typedef F value_type;
	enum { component_count = 2 };

	F x, y;

#ifdef _DEBUG
	ILINE Vec2_tpl()
	{
		if (sizeof(F) == 4)
		{
			uint32* p = alias_cast<uint32*>(&x);
			p[0] = F32NAN;
			p[1] = F32NAN;
		}
		if (sizeof(F) == 8)
		{
			uint64* p = alias_cast<uint64*>(&x);
			p[0] = F64NAN;
			p[1] = F64NAN;
		}
	}
#else
	ILINE Vec2_tpl() {}
#endif

	ILINE Vec2_tpl(type_zero) : x(0), y(0) {}
	ILINE Vec2_tpl(F vx, F vy) { x = vx; y = vy; }
	explicit ILINE Vec2_tpl(F m) { x = y = m; }

	ILINE Vec2_tpl& set(F nx, F ny) { x = F(nx); y = F(ny); return *this; }

	template<class F1> ILINE Vec2_tpl(const Vec2_tpl<F1>& src) { x = F(src.x); y = F(src.y); }
	template<class F1> ILINE explicit Vec2_tpl(const Vec3_tpl<F1>& src) { x = F(src.x); y = F(src.y); }
	template<class F1> ILINE explicit Vec2_tpl(const F1* psrc) { x = F(psrc[0]); y = F(psrc[1]); }

	explicit ILINE Vec2_tpl(const Vec3_tpl<F>& v) : x((F)v.x), y((F)v.y) { CRY_MATH_ASSERT(this->IsValid()); }

	ILINE Vec2_tpl& operator=(const Vec2_tpl& src) { x = src.x; y = src.y; return *this; }
	//template<class F1> Vec2_tpl& operator=(const Vec2_tpl<F1>& src) { x=F(src.x); y=F(src.y); return *this; }
	//template<class F1> Vec2_tpl& operator=(const Vec3_tpl<F1>& src) { x=F(src.x); y=F(src.y); return *this; }

	ILINE int operator!() const { return x == 0 && y == 0; }

	//! The default Normalize function is in fact "safe". 0 vectors remain unchanged.
	Vec2_tpl& Normalize()
	{
		F fInvLen = isqrt_safe_tpl(x * x + y * y);
		x *= fInvLen;
		y *= fInvLen;
		return *this;
	}

	//! Normalize if non-0, otherwise set to specified "safe" value.
	Vec2_tpl& NormalizeSafe(const struct Vec2_tpl<F>& safe = Vec2Constants<F>::fVec2_Zero)
	{
		F fLen2 = x * x + y * y;
		if (fLen2 > 0.0f)
		{
			F fInvLen = isqrt_tpl(fLen2);
			x *= fInvLen;
			y *= fInvLen;
		}
		else
		{
			*this = safe;
		}
		return *this;
	}

	Vec2_tpl GetNormalized() const
	{
		F fInvLen = isqrt_safe_tpl(x * x + y * y);
		return *this * fInvLen;
	}

	Vec2_tpl GetNormalizedSafe(const struct Vec2_tpl<F>& safe = Vec2Constants<F>::fVec2_OneX) const
	{
		F fLen2 = x * x + y * y;
		if (fLen2 > 0.0f)
		{
			F fInvLen = isqrt_tpl(fLen2);
			return *this * fInvLen;
		}
		else
		{
			return safe;
		}
	}

	ILINE bool IsEquivalent(const Vec2_tpl<F>& v1, F epsilon = VEC_EPSILON) const
	{
		CRY_MATH_ASSERT(v1.IsValid());
		CRY_MATH_ASSERT(this->IsValid());
		return  ((fabs_tpl(x - v1.x) <= epsilon) && (fabs_tpl(y - v1.y) <= epsilon));
	}

	ILINE static bool IsEquivalent(const Vec2_tpl<F>& v0, const Vec2_tpl<F>& v1, F epsilon = VEC_EPSILON)
	{
		CRY_MATH_ASSERT(v0.IsValid());
		CRY_MATH_ASSERT(v1.IsValid());
		return  ((fabs_tpl(v0.x - v1.x) <= epsilon) && (fabs_tpl(v0.y - v1.y) <= epsilon));
	}

	ILINE F GetLength() const
	{
		return sqrt_tpl(x * x + y * y);
	}

	ILINE F GetLengthSquared() const
	{
		return x * x + y * y;
	}

	ILINE F GetLength2() const
	{
		return x * x + y * y;
	}

	void SetLength(F fLen)
	{
		F fLenMe = GetLength2();
		if (fLenMe < 0.00001f * 0.00001f)
			return;
		fLenMe = fLen * isqrt_tpl(fLenMe);
		x *= fLenMe;
		y *= fLenMe;
	}

	ILINE F         area() const              { return x * y; }

	ILINE F&        operator[](int idx)       { return *((F*)&x + idx); }
	ILINE F         operator[](int idx) const { return *((F*)&x + idx); }
	ILINE           operator F*()             { return &x; }
	ILINE Vec2_tpl& flip()                    { x = -x; y = -y; return *this; }
	ILINE Vec2_tpl& zero()                    { x = y = 0; return *this; }
	ILINE Vec2_tpl  rot90ccw() const          { return Vec2_tpl(-y, x); }
	ILINE Vec2_tpl  rot90cw() const           { return Vec2_tpl(y, -x); }

#ifdef quotient_h
	quotient_tpl<F> fake_atan2() const
	{
		quotient_tpl<F> res;
		int quad = -(signnz(x * x - y * y) - 1 >> 1); // hope the optimizer will remove complement shifts and adds
		if (quad) { res.x = -y; res.y = x; }
		else { res.x = x; res.y = y; }
		int sgny = signnz(res.y);
		quad |= 1 - sgny;                           //(res.y<0)<<1;
		res.x *= sgny;
		res.y *= sgny;
		res += 1 + (quad << 1);
		return res;
	}
#endif
	ILINE F         atan2() const        { return atan2_tpl(y, x); }

	ILINE Vec2_tpl  operator-() const    { return Vec2_tpl(-x, -y); }

	ILINE Vec2_tpl  operator*(F k) const { return Vec2_tpl(x * k, y * k); }
	ILINE Vec2_tpl& operator*=(F k)      { x *= k; y *= k; return *this; }
	ILINE Vec2_tpl  operator/(F k) const { return *this * ((F)1.0 / k); }
	ILINE Vec2_tpl& operator/=(F k)      { return *this *= ((F)1.0 / k); }

	//	bool operator==(const Vec2_tpl<F> &vec) { return x == vec.x && y == vec.y; }
	ILINE bool        operator!=(const Vec2_tpl<F>& vec)                            { return !(*this == vec); }

	ILINE friend bool operator==(const Vec2_tpl<F>& left, const Vec2_tpl<F>& right) { return left.x == right.x && left.y == right.y; }
	ILINE friend bool operator!=(const Vec2_tpl<F>& left, const Vec2_tpl<F>& right) { return !(left == right);  }

	ILINE bool        IsZero(F e = (F)0.0) const
	{
		return (fabs_tpl(x) <= e) && (fabs_tpl(y) <= e);
	}

	//! IsZeroSlow would be a better name.
	ILINE bool IsZeroFast(F e = (F)0.0003) const
	{
		return (fabs_tpl(x) + fabs_tpl(y)) <= e;
	}

	ILINE F Dot(const Vec2_tpl& rhs) const { return x * rhs.x + y * rhs.y; }

	//! \return A vector perpendicular to this one (this->Cross(newVec) points "up").
	ILINE Vec2_tpl Perp() const { return Vec2_tpl(-y, x); }

	//! The size of the "parallel-trapets" area spanned by the two vectors.
	ILINE F Cross(const Vec2_tpl<F>& v) const
	{
		return float (x * v.y - y * v.x);
	}

	//! Linear-Interpolation between Vec3 (lerp).
	//! Example:
	//!   Vec3 r=Vec3::CreateLerp( p, q, 0.345f );
	ILINE void               SetLerp(const Vec2_tpl<F>& p, const Vec2_tpl<F>& q, F t)    { *this = p * (1.0f - t) + q * t; }
	ILINE static Vec2_tpl<F> CreateLerp(const Vec2_tpl<F>& p, const Vec2_tpl<F>& q, F t) { return p * (1.0f - t) + q * t; }

	//! Spherical-Interpolation between 3d-vectors (geometrical slerp).
	//! both vectors are assumed to be normalized.
	//! Example:
	//!   Vec3 r=Vec3::CreateSlerp( p, q, 0.5674f );
	void SetSlerp(const Vec2_tpl<F>& p, const Vec2_tpl<F>& q, F t)
	{
		CRY_MATH_ASSERT((fabs_tpl(1 - (p | p))) < 0.005); //check if unit-vector
		CRY_MATH_ASSERT((fabs_tpl(1 - (q | q))) < 0.005); //check if unit-vector
		// calculate cosine using the "inner product" between two vectors: p*q=cos(radiant)
		F cosine = (p | q);
		//we explore the special cases where the both vectors are very close together,
		//in which case we approximate using the more economical LERP and avoid "divisions by zero" since sin(Angle) = 0  as   Angle=0
		if (cosine >= (F)0.99)
		{
			SetLerp(p, q, t); //perform LERP:
			this->Normalize();
		}
		else
		{
			//perform SLERP: because of the LERP-check above, a "division by zero" is not possible
			F rad = acos_tpl(cosine);
			F scale_0 = sin_tpl((1 - t) * rad);
			F scale_1 = sin_tpl(t * rad);
			*this = (p * scale_0 + q * scale_1) / sin_tpl(rad);
			this->Normalize();
		}
	}
	ILINE static Vec2_tpl<F> CreateSlerp(const Vec2_tpl<F>& p, const Vec2_tpl<F>& q, F t)
	{
		Vec2_tpl<F> v;
		v.SetSlerp(p, q, t);
		return v;
	}

	bool IsValid() const
	{
		if (!NumberValid(x)) return false;
		if (!NumberValid(y)) return false;
		return true;
	}

	ILINE F GetDistance(const Vec2_tpl<F>& vec1) const
	{
		return sqrt_tpl((x - vec1.x) * (x - vec1.x) + (y - vec1.y) * (y - vec1.y));
	}

	AUTO_STRUCT_INFO;
};

// Typedefs

typedef Vec2_tpl<f32>  Vec2;  //!< Always 32 bit.
typedef Vec2_tpl<f64>  Vec2d; //!< Always 64 bit.
typedef Vec2_tpl<int>  Vec2i;
typedef Vec2_tpl<real> Vec2r; //!< Variable float precision. depending on the target system it can be 32, 64 or 80 bit.

template<class F>
Vec2_tpl<F> operator*(F op1, const Vec2_tpl<F>& op2) { return Vec2_tpl<F>(op1 * op2.x, op1 * op2.y); }

template<class F1, class F2>
F1 operator*(const Vec2_tpl<F1>& op1, const Vec2_tpl<F2>& op2) { return op1.x * op2.x + op1.y * op2.y; } //!< Dot product.
template<class F1, class F2>
F1 operator|(const Vec2_tpl<F1>& op1, const Vec2_tpl<F2>& op2) { return op1.x * op2.x + op1.y * op2.y; } //!< Dot product.

template<class F1, class F2>
F1 operator^(const Vec2_tpl<F1>& op1, const Vec2_tpl<F2>& op2) { return op1.x * op2.y - op1.y * op2.x; } //!< Cross product.

template<class F1, class F2>
Vec2_tpl<F1> operator+(const Vec2_tpl<F1>& op1, const Vec2_tpl<F2>& op2)
{
	return Vec2_tpl<F1>(op1.x + op2.x, op1.y + op2.y);
}
template<class F1, class F2>
Vec2_tpl<F1> operator-(const Vec2_tpl<F1>& op1, const Vec2_tpl<F2>& op2)
{
	return Vec2_tpl<F1>(op1.x - op2.x, op1.y - op2.y);
}

template<class F1, class F2>
Vec2_tpl<F1>& operator+=(Vec2_tpl<F1>& op1, const Vec2_tpl<F2>& op2)
{
	op1.x += op2.x;
	op1.y += op2.y;
	return op1;
}
template<class F1, class F2>
Vec2_tpl<F1>& operator-=(Vec2_tpl<F1>& op1, const Vec2_tpl<F2>& op2)
{
	op1.x -= op2.x;
	op1.y -= op2.y;
	return op1;
}

#if !defined(SWIG)
template<typename T> Vec2_tpl<T> Vec2Constants<T >::fVec2_Zero(0, 0);
template<typename T> Vec2_tpl<T> Vec2Constants<T >::fVec2_OneX(1, 0);
template<typename T> Vec2_tpl<T> Vec2Constants<T >::fVec2_OneY(0, 1);
template<typename T> Vec2_tpl<T> Vec2Constants<T >::fVec2_One(1, 1);
#endif

template<class F>
bool operator==(const Vec2_tpl<F>& left, const Vec2_tpl<F>& right)
{
	return left.x == right.x && left.y == right.y;
}

template<> ILINE Vec2 clamp_tpl<Vec2>(Vec2 X, Vec2 Min, Vec2 Max)
{
	return Vec2(clamp_tpl(X.x, Min.x, Max.x), clamp_tpl(X.y, Min.y, Max.y));
}

#endif // CRYTEK_CRYVECTOR2_H
