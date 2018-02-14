// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once
#include <CryCore/Platform/platform.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <type_traits>
#include <string.h>

namespace CryGTestDetails
{
	constexpr int messageBufferLength = 256;

#define CRY_GTEST_MESSAGE(MSG) \
  CRY_GTEST_MESSAGE2(MSG, this->m_file, this->m_line, ::testing::TestPartResult::kNonFatalFailure)

#define CRY_GTEST_MESSAGE2(MSG, FILE_PATH, LINE, RESULT_TYPE) \
  GTEST_MESSAGE_AT_(FILE_PATH, LINE, MSG, RESULT_TYPE)

	enum class EBinaryOp
	{
		Equal,
		Unequal,
		LessThan,
		GreaterThan,
		LessEqual,
		GreaterEqual
	};

	class CDisableOperators
	{
	protected:
		struct None
		{
			// Stub function for REQUIRES() macro
			void Evaluate() const { FAIL(); }
		};

		template<typename T>
		struct AlwaysFalse
		{
			static constexpr bool value = false;
		};

	public:
#define DISABLE_BINARY_OPERATOR_WITH_MSG(OP, MSG)       \
		template<typename T>							\
		constexpr None operator OP (T&&) const			\
		{												\
			static_assert(AlwaysFalse<T>::value, MSG);	\
			return {};									\
		}

		DISABLE_BINARY_OPERATOR_WITH_MSG(&&, "\"&&\" (logical-and) is not supported. Please simplify the test expression or wrap it with parentheses.");
		DISABLE_BINARY_OPERATOR_WITH_MSG(||, "\"||\" (logical-or) is not supported. Please simplify the test expression or wrap it with parentheses.");
		DISABLE_BINARY_OPERATOR_WITH_MSG(&, "\"&\" (bitwise-and) is not supported. Please simplify the test expression or wrap it with parentheses.");
		DISABLE_BINARY_OPERATOR_WITH_MSG(|, "\"|\" (bitwise-or) is not supported. Please simplify the test expression or wrap it with parentheses.");
		DISABLE_BINARY_OPERATOR_WITH_MSG(^, "\"^\" (bitwise-xor) is not supported. Please simplify the test expression or wrap it with parentheses.");
		DISABLE_BINARY_OPERATOR_WITH_MSG(+=, "\"+=\" is not supported. Please simplify the test expression or wrap it with parentheses.");
		DISABLE_BINARY_OPERATOR_WITH_MSG(-=, "\"-=\" is not supported. Please simplify the test expression or wrap it with parentheses.");
		DISABLE_BINARY_OPERATOR_WITH_MSG(*=, "\"*=\" is not supported. Please simplify the test expression or wrap it with parentheses.");
		DISABLE_BINARY_OPERATOR_WITH_MSG(/=, "\"/=\" is not supported. Please simplify the test expression or wrap it with parentheses.");
		DISABLE_BINARY_OPERATOR_WITH_MSG(%=, "\"%=\" is not supported. Please simplify the test expression or wrap it with parentheses.");
		DISABLE_BINARY_OPERATOR_WITH_MSG(<<=, "\"<<=\" is not supported. Please simplify the test expression or wrap it with parentheses.");
		DISABLE_BINARY_OPERATOR_WITH_MSG(>>=, "\">>=\" is not supported. Please simplify the test expression or wrap it with parentheses.");
		DISABLE_BINARY_OPERATOR_WITH_MSG(&=, "\"&=\" is not supported. Please simplify the test expression or wrap it with parentheses.");
		DISABLE_BINARY_OPERATOR_WITH_MSG(|=, "\"|=\" is not supported. Please simplify the test expression or wrap it with parentheses.");
		DISABLE_BINARY_OPERATOR_WITH_MSG(^=, "\"^=\" is not supported. Please simplify the test expression or wrap it with parentheses.");
#undef DISABLE_BINARY_OPERATOR_WITH_MSG
	};

	template<typename T1, typename T2>
	class CBinaryExpressionBase : public CDisableOperators
	{
	public:

		CBinaryExpressionBase(
			const char* msg,
			const char* file,
			int line,
			T1 val1,
			T2 val2)
			: m_msg(msg)
			, m_file(file)
			, m_line(line)
			, m_val1(val1)
			, m_val2(val2)
		{}

	protected:
		template<size_t NDelim, size_t N1, size_t N2>
		static void parse(const char* msg, const char(&delim)[NDelim], char(&out1)[N1], char(&out2)[N2])
		{
			const char *pos = strstr(msg, delim);
			if (pos)
			{
				strncpy(out1, msg, std::min((size_t)(pos - msg), N1 - 1));
				strncpy(out2, pos + NDelim, N2 - 1);
			}
			else
			{
				strncpy(out1, msg, N1 - 1);
			}
		}

		const char*        m_msg;
		const char*        m_file;
		int                m_line;
		T1                 m_val1;
		T2                 m_val2;
	};

	template<EBinaryOp Op, typename T1, typename T2>
	class CBinaryExpression;

	template<typename T1, typename T2>
	class CBinaryExpression<EBinaryOp::Equal, T1, T2> : public CBinaryExpressionBase<T1, T2>
	{
	public:
		using CBinaryExpressionBase<T1, T2>::CBinaryExpressionBase;

		void Evaluate() const
		{
			char lhs[messageBufferLength] = {}, rhs[messageBufferLength] = {};
			this->parse(this->m_msg, "==", lhs, rhs);
			GTEST_ASSERT_(::testing::internal::EqHelper<GTEST_IS_NULL_LITERAL_(this->m_val1)>::Compare(lhs, rhs, this->m_val1, this->m_val2),
				CRY_GTEST_MESSAGE);
		}
	};

	template<typename T1, typename T2>
	class CBinaryExpression<EBinaryOp::Unequal, T1, T2> : public CBinaryExpressionBase<T1, T2>
	{
	public:
		using CBinaryExpressionBase<T1, T2>::CBinaryExpressionBase;

		void Evaluate() const
		{
			char lhs[messageBufferLength] = {}, rhs[messageBufferLength] = {};
			this->parse(this->m_msg, "!=", lhs, rhs);
			GTEST_ASSERT_(::testing::internal::CmpHelperNE(lhs, rhs, this->m_val1, this->m_val2), CRY_GTEST_MESSAGE);
		}
	};

	template<typename T1, typename T2>
	class CBinaryExpression<EBinaryOp::LessThan, T1, T2> : public CBinaryExpressionBase<T1, T2>
	{
	public:
		using CBinaryExpressionBase<T1, T2>::CBinaryExpressionBase;

		void Evaluate() const
		{
			char lhs[messageBufferLength] = {}, rhs[messageBufferLength] = {};
			this->parse(this->m_msg, "<", lhs, rhs);
			GTEST_ASSERT_(::testing::internal::CmpHelperLT(lhs, rhs, this->m_val1, this->m_val2), CRY_GTEST_MESSAGE);
		}
	};

	template<typename T1, typename T2>
	class CBinaryExpression<EBinaryOp::GreaterThan, T1, T2> : public CBinaryExpressionBase<T1, T2>
	{
	public:
		using CBinaryExpressionBase<T1, T2>::CBinaryExpressionBase;

		void Evaluate() const
		{
			char lhs[messageBufferLength] = {}, rhs[messageBufferLength] = {};
			this->parse(this->m_msg, ">", lhs, rhs);
			GTEST_ASSERT_(::testing::internal::CmpHelperGT(lhs, rhs, this->m_val1, this->m_val2), CRY_GTEST_MESSAGE);
		}
	};

	template<typename T1, typename T2>
	class CBinaryExpression<EBinaryOp::LessEqual, T1, T2> : public CBinaryExpressionBase<T1, T2>
	{
	public:
		using CBinaryExpressionBase<T1, T2>::CBinaryExpressionBase;

		void Evaluate() const
		{
			char lhs[messageBufferLength] = {}, rhs[messageBufferLength] = {};
			this->parse(this->m_msg, "<=", lhs, rhs);
			GTEST_ASSERT_(::testing::internal::CmpHelperLE(lhs, rhs, this->m_val1, this->m_val2), CRY_GTEST_MESSAGE);
		}
	};

	template<typename T1, typename T2>
	class CBinaryExpression<EBinaryOp::GreaterEqual, T1, T2> : public CBinaryExpressionBase<T1, T2>
	{
	public:
		using CBinaryExpressionBase<T1, T2>::CBinaryExpressionBase;

		void Evaluate() const
		{
			char lhs[messageBufferLength] = {}, rhs[messageBufferLength] = {};
			this->parse(this->m_msg, ">=", lhs, rhs);
			GTEST_ASSERT_(::testing::internal::CmpHelperGE(lhs, rhs, this->m_val1, this->m_val2), CRY_GTEST_MESSAGE);
		}
	};

	// Utilities for determining whether the type is suitable for pass-by-value
	template<typename T>
	struct FavorCopy
	{
		static constexpr bool value =
			std::is_integral<T>::value ||
			std::is_pointer<typename std::decay<T>::type>::value ||
			(std::is_trivially_copyable<T>::value && sizeof(T) <= sizeof(void*));
	};

	template<typename T>
	using EnableIfFavorCopy = typename std::enable_if<FavorCopy<T>::value, int>::type;

	template<typename T>
	using EnableIfUnfavorCopy = typename std::enable_if<!FavorCopy<T>::value, int>::type;

	template<typename T>
	class CLeftHandSideExpression
		: public CDisableOperators
	{
	public:
		constexpr CLeftHandSideExpression(const char* msg, const char* file, int line, T val)
			: m_msg(msg)
			, m_file(file)
			, m_line(line)
			, m_val(val)
		{}

		template<typename U>
		None operator= (U&&)
		{
			static_assert(AlwaysFalse<U>::value, "\"=\" is not supported. Did you mean \"==\"? Otherwise please simplify the test expression or wrap it with parentheses.");
			return {};
		}

		template<typename U, EnableIfUnfavorCopy<U> = 0>
		constexpr CBinaryExpression<EBinaryOp::Equal, T, const U&> operator==(const U& otherValue) const
		{
			return { m_msg, m_file, m_line, m_val, otherValue };
		}

		template<typename U, EnableIfFavorCopy<U> = 0>
		constexpr CBinaryExpression<EBinaryOp::Equal, T, U> operator==(U otherValue) const
		{
			return{ m_msg, m_file, m_line, m_val, otherValue };
		}

		template<typename U, EnableIfUnfavorCopy<U> = 0>
		constexpr CBinaryExpression<EBinaryOp::Unequal, T, const U&> operator!=(const U& otherValue) const
		{
			return { m_msg, m_file, m_line, m_val, otherValue };
		}

		template<typename U, EnableIfFavorCopy<U> = 0>
		constexpr CBinaryExpression<EBinaryOp::Unequal, T, U> operator!=(U otherValue) const
		{
			return{ m_msg, m_file, m_line, m_val, otherValue };
		}

		template<typename U, EnableIfUnfavorCopy<U> = 0>
		constexpr CBinaryExpression<EBinaryOp::LessEqual, T, const U&> operator<=(const U& otherValue) const
		{
			return { m_msg, m_file, m_line, m_val, otherValue };
		}

		template<typename U, EnableIfFavorCopy<U> = 0>
		constexpr CBinaryExpression<EBinaryOp::LessEqual, T, U> operator<=(U otherValue) const
		{
			return{ m_msg, m_file, m_line, m_val, otherValue };
		}

		template<typename U, EnableIfUnfavorCopy<U> = 0>
		constexpr CBinaryExpression<EBinaryOp::GreaterEqual, T, const U&> operator>=(const U& otherValue) const
		{
			return { m_msg, m_file, m_line, m_val, otherValue };
		}

		template<typename U, EnableIfFavorCopy<U> = 0>
		constexpr CBinaryExpression<EBinaryOp::GreaterEqual, T, U> operator>=(U otherValue) const
		{
			return{ m_msg, m_file, m_line, m_val, otherValue };
		}

		template<typename U, EnableIfUnfavorCopy<U> = 0>
		constexpr CBinaryExpression<EBinaryOp::LessThan, T, const U&> operator<(const U& otherValue) const
		{
			return { m_msg, m_file, m_line, m_val, otherValue };
		}

		template<typename U, EnableIfFavorCopy<U> = 0>
		constexpr CBinaryExpression<EBinaryOp::LessThan, T, U> operator<(U otherValue) const
		{
			return{ m_msg, m_file, m_line, m_val, otherValue };
		}

		template<typename U, EnableIfUnfavorCopy<U> = 0>
		constexpr CBinaryExpression<EBinaryOp::GreaterThan, T, const U&> operator>(const U& otherValue) const
		{
			return { m_msg, m_file, m_line, m_val, otherValue };
		}

		template<typename U, EnableIfFavorCopy<U> = 0>
		constexpr CBinaryExpression<EBinaryOp::GreaterThan, T, U> operator>(U otherValue) const
		{
			return{ m_msg, m_file, m_line, m_val, otherValue };
		}

		void Evaluate() const
		{
			GTEST_TEST_BOOLEAN_((m_val), m_msg, false, true, CRY_GTEST_MESSAGE);
		}

	private:

		const char* m_msg;
		const char* m_file;
		int         m_line;
		T           m_val;
	};

#undef CRY_GTEST_MESSAGE
#undef CRY_GTEST_MESSAGE2

	class CExpressionDecomposer
	{
	public:
		constexpr CExpressionDecomposer(const char* msg, const char* file, int line)
			: m_msg(msg)
			, m_file(file)
			, m_line(line)
		{}

		template<typename T, EnableIfUnfavorCopy<T> = 0>
		constexpr CLeftHandSideExpression<const T&> operator<<(const T& val) const
		{
			return{ m_msg, m_file, m_line, val };
		}

		// Pass by value for certain types of arguments such as integral types
		// This solves the problem sometimes the address of the value cannot be taken,
		// e.g. inline static const member
		template<typename T, EnableIfFavorCopy<T> = 0>
		constexpr CLeftHandSideExpression<T> operator<<(T val) const
		{
			return{ m_msg, m_file, m_line, val };
		}

	private:

		const char* m_msg;
		const char* m_file;
		int         m_line;
	};
}

//Redirection is necessary to reject comma separated arguments. Only single expressions are accepted.
#define REQUIRE_IMPL(expr) (CryGTestDetails::CExpressionDecomposer(STRINGIFY(expr), __FILE__, __LINE__) << expr).Evaluate()

//! Macro for testing a condition expression similar to EXPECT_TRUE(cond), but with improved error report.
//! When evaluating a comparison, it holds both sides of the comparison with an expression template.
//!
//! Example: 
//! 	int i = 3, j = 4;
//! 	REQUIRE(i == j);
//! 
//! Prints out this message: "Expected: i == j, actual: 3 vs 4"
//! The message contains the actual value of both sides in the same way as {EXPECT_??} macros.
//! This allows us to directly evaluate the equation rather than using one of the predefined
//! relationship-macros such as EXPECT_EQ, EXPECT_NE, EXPECT_LT, EXPECT_LE, ...
#define REQUIRE(expr) REQUIRE_IMPL(expr)


// GTest printing support for CRYENGINE types
#include <CryString/CryString.h>

inline void PrintTo(std::nullptr_t, ::std::ostream* os) { *os << "NULL"; }

inline void PrintTo(const string& str, ::std::ostream* os) { *os << str.c_str(); }
inline void PrintTo(const wstring& str, ::std::ostream* os) { *os << str.c_str(); }

template<class T, size_t S>
inline void PrintTo(const CryStackStringT<T, S>& str, ::std::ostream* os) { *os << str.c_str(); }

template<size_t S>
inline void PrintTo(const CryFixedStringT<S>& str, ::std::ostream* os) { *os << str.c_str(); }

template<size_t S>
inline void PrintTo(const CryFixedWStringT<S>& str, ::std::ostream* os) { *os << str.c_str(); }

