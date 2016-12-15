// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <type_traits>

template<int left, int right> struct static_max
{
	enum
	{
		value = (left > right ? left : right),
	};
};

template<int left, int right> struct static_min
{
	enum
	{
		value = (left < right ? left : right),
	};
};

template<unsigned num> struct static_log2
{
	enum
	{
		value = static_log2<(num >> 1)>::value + 1,
	};
};

template<> struct static_log2<1>
{
	enum
	{
		value = 0,
	};
};

template<> struct static_log2<0>
{
};

// aligment_type - returns POD type with the desired alignment
template<size_t Alignment>
struct alignment_type
{
private:
	template<typename AlignmentType>
	struct SameAlignment
	{
		enum { value = Alignment == alignof(AlignmentType), };
	};

public:
	typedef                        int (alignment_type::* mptr);
	typedef int (alignment_type::* mfptr)();

	typedef typename std::conditional<SameAlignment<char>::value, char,
		typename std::conditional<SameAlignment<short>::value, short,
			typename std::conditional<SameAlignment<int>::value, int,
				typename std::conditional<SameAlignment<long>::value, long,
					typename std::conditional<SameAlignment<long long>::value, long long,
						typename std::conditional<SameAlignment<float>::value, float,
							typename std::conditional<SameAlignment<double>::value, double,
								typename std::conditional<SameAlignment<long double>::value, long double,
									typename std::conditional<SameAlignment<void*>::value, void*,
										typename std::conditional<SameAlignment<mptr>::value, mptr,
											typename std::conditional<SameAlignment<mfptr>::value, mfptr, char>::type>::type
										>::type
									>::type
								>::type
							>::type
						>::type
					>::type
				>::type
			>::type
		>::type type;
};

//! aligned_buffer - declares a buffer with desired alignment and size
template<size_t Alignment, size_t Size>
struct aligned_buffer
{
private:
	struct storage
	{
		union
		{
			typename alignment_type<Alignment>::type _alignment;
			char _buffer[Size];
		} buffer;
	};

	storage buffer;
};

//! aligned_storage - declares a buffer that can naturally hold the passed type
template<typename Ty>
struct aligned_storage
{
private:
	struct storage
	{
		union
		{
			typename alignment_type<alignof(Ty)>::type _alignment;
			char _buffer[sizeof(Ty)];
		} buffer;
	};

	storage buffer;
};

// test type equality
template<typename T1, typename T2> struct SSameType
{
	enum
	{
		value = false
	};
};
template<typename T1> struct SSameType<T1, T1>
{
	enum
	{
		value = true
	};
};
