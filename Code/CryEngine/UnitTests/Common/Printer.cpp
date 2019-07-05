// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include <CryCore/Platform/platform.h>
#include "UnitTest.h"

// GTest printing overloads for floating point with enough decimal and hex precision
// to improve error message qualities
// See also gtest-printers.h from Google Test
namespace testing
{
	//std::ceil is not constexpr, therefore we supplement ours
	template<typename T>
	constexpr int ceil(T f)
	{
		return (static_cast<T>(static_cast<int>(f)) == f) ?
			static_cast<int>(f) :
			static_cast<int>(f) + ((f > 0) ? 1 : 0);
	}

	// "9.20000076(0x1.2666680p+3)" for single-precision float
	template<typename T>
	void CryUnitTestPrintFloatingPoint(T value, ::std::ostream* os)
	{
		std::streamsize defaultPrecision = os->precision();
		constexpr int max_digits16 = ceil(std::numeric_limits<T>::digits / T(4)/*log2(16)*/ + 1);
		*os << std::setprecision(std::numeric_limits<T>::max_digits10) << value
			<< '(' << std::hexfloat << std::setprecision(max_digits16) << value << ')'
			<< std::setprecision(defaultPrecision);
	}

	template<> ::std::string PrintToString(const float& value)
	{
		::std::stringstream ss;
		CryUnitTestPrintFloatingPoint(value, &ss);
		return ss.str();
	}

	template<> ::std::string PrintToString(const double& value)
	{
		::std::stringstream ss;
		CryUnitTestPrintFloatingPoint(value, &ss);
		return ss.str();
	}

	template<> ::std::string PrintToString(const long double& value)
	{
		::std::stringstream ss;
		CryUnitTestPrintFloatingPoint(value, &ss);
		return ss.str();
	}
}
