// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <cstddef>     // std::size_t
#include <type_traits> // std::enable_if

namespace stl
{
//! See std::integer_sequence. To be removed when upgraded to C++14.
template<class T, T ... I>
class integer_sequence
{
public:
	static constexpr std::size_t size() noexcept { return sizeof ... (I); }
};

//! See std::index_sequence. To be removed when upgraded to C++14.
template<std::size_t ... I>
using index_sequence = integer_sequence<std::size_t, I ...>;

namespace details
{
template<typename T, T N, typename = void, T ... I>
struct integer_sequence_applyer
{
	using next = integer_sequence_applyer<T, N - 1, void, I ..., sizeof ... (I)>;
	using type = typename next::type;
};

template<typename T, T N, T ... I>
struct integer_sequence_applyer<T, N, typename std::enable_if<N == 0>::type, I ...>
{
	using type = integer_sequence<T, I ...>;
};
}

//! See std::make_integer_sequence. To be removed when upgraded to C++14.
template<class T, T N>
using make_integer_sequence = typename details::integer_sequence_applyer<T, N>::type;

//! See std::make_index_sequence. To be removed when upgraded to C++14.
template<std::size_t N>
using make_index_sequence = make_integer_sequence<std::size_t, N>;
}
