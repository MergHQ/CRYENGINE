// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include <UnitTest.h>
#include <CryCore/SmallFunction.h>
#include <CryCore/StlUtils.h>
#include <memory>
#include <type_traits>
#include "Utils.h"

int PlainFunction(int x)
{
	return x * x;
}

struct SInstancesDetector
{
	static std::set<SInstancesDetector*> sInstances;
	SInstancesDetector() { sInstances.insert(this); }
	SInstancesDetector(const SInstancesDetector&) { sInstances.insert(this); }
	~SInstancesDetector() { sInstances.erase(this); }
};

std::set<SInstancesDetector*> SInstancesDetector::sInstances;

//Meta test
static_assert(!std::has_virtual_destructor<SmallFunction<void()>>::value, "SmallFunction is designed to be without virtual table");

TEST(SmallFunctionTest, DefaultConstruction)
{
	SmallFunction<void()> f;
	REQUIRE(!f);
	REQUIRE(f == nullptr);
}

TEST(SmallFunctionTest, ConstructionFromFunctionPointer)
{
	SmallFunction<int(int)> f = &PlainFunction;
	REQUIRE(f(4) == 16);
}

TEST(SmallFunctionTest, ConstructionFromFunctionReference)
{
	SmallFunction<int(int)> f = PlainFunction;
	REQUIRE(f(4) == 16);
}

TEST(SmallFunctionTest, ConstructionFromLambda)
{
	int v = 0;

	SmallFunction<void(int)> f = [&v](int x)
	{
		v = x;
	};

	f(42);
	REQUIRE(v == 42);
}

TEST(SmallFunctionTest, AssignFunctionPointer)
{
	SmallFunction<int(int)> f;
	f = &PlainFunction;
	REQUIRE(f(4) == 16);
}

TEST(SmallFunctionTest, AssignFunctionReference)
{
	SmallFunction<int(int)> f;
	f = PlainFunction;
	REQUIRE(f(4) == 16);
}

TEST(SmallFunctionTest, AssignLambda)
{
	int v = 0;

	SmallFunction<void(int)> f;
	f = [&v](int x)
	{
		v = x;
	};

	f(42);
	REQUIRE(v == 42);
}

TEST(SmallFunctionTest, Destruction)
{
	SInstancesDetector::sInstances.clear();

	{
		SInstancesDetector instance;
		SmallFunction<void()> f = [instance] {};
		REQUIRE(SInstancesDetector::sInstances.size() == 2);
	}
	REQUIRE(SInstancesDetector::sInstances.size() == 0);
}

TEST(SmallFunctionTest, CopyConstruction)
{
	SInstancesDetector::sInstances.clear();
	SInstancesDetector instance;
	{
		SmallFunction<void()> f1 = [instance] {};
		{
			SmallFunction<void()> f2 = f1;
			REQUIRE(SInstancesDetector::sInstances.size() == 3);
		}
		REQUIRE(SInstancesDetector::sInstances.size() == 2);
	}
	REQUIRE(SInstancesDetector::sInstances.size() == 1);
}

TEST(SmallFunctionTest, CopyAssignNonEmptyToEmpty)
{
	int v = 0;
	SInstancesDetector::sInstances.clear();

	SInstancesDetector instance;

	SmallFunction<void(int)> f1;
	SmallFunction<void(int)> f2 = [&v, instance](int x)
	{
		v = x;
	};
	f1 = f2;

	REQUIRE(SInstancesDetector::sInstances.size() == 3);

	f1(42);
	REQUIRE(v == 42);
}

TEST(SmallFunctionTest, CopyAssignNonEmptyToNonEmpty)
{
	SInstancesDetector::sInstances.clear();
	SInstancesDetector instance;
	SmallFunction<void()> f1 = [instance] {};
	REQUIRE(SInstancesDetector::sInstances.size() == 2);
	{
		SmallFunction<void()> f2 = [instance] {};
		f1 = f2;
	}

	REQUIRE(SInstancesDetector::sInstances.size() == 2);
	{
		SmallFunction<void()> f2 = [] {};
		f1 = f2;
	}
	REQUIRE(SInstancesDetector::sInstances.size() == 1);
}

SmallFunction<void()> GetMovedFromFunction()
{
	SInstancesDetector::sInstances.clear();
	SInstancesDetector instance;
	return [instance] {};
}

TEST(SmallFunctionTest, MoveConstruction)
{
	SmallFunction<void()> f1 = GetMovedFromFunction();
	REQUIRE(SInstancesDetector::sInstances.size() == 1);
	SmallFunction<void()> f2 = std::move(f1);
	REQUIRE(!f1);
	REQUIRE(f2);
	REQUIRE(SInstancesDetector::sInstances.size() == 1);
}

TEST(SmallFunctionTest, MoveAssignment)
{
	SmallFunction<void()> f1 = GetMovedFromFunction();
	REQUIRE(SInstancesDetector::sInstances.size() == 1);
	SmallFunction<void()> f2;
	f2 = std::move(f1);
	REQUIRE(!f1);
	REQUIRE(f2);
	REQUIRE(SInstancesDetector::sInstances.size() == 1);
}

TEST(SmallFunctionTest, ExplicitBool)
{
	SmallFunction<void()> f;
	REQUIRE(!f);

	SmallFunction<void()> f2 = [] {};
	REQUIRE(f2);
}

TEST(SmallFunctionTest, Null)
{
	//Construct from nullptr
	SmallFunction<void()> f = nullptr;

	//Must be the same as default construction
	REQUIRE(!f);

	//Must satisfy == nullptr
	REQUIRE(f == nullptr);

	SmallFunction<void()> f2 = [] {};
	REQUIRE(f2 != nullptr);

	//Reassign to nullptr must work
	f2 = nullptr;
	REQUIRE(!f2);
}

TEST(SmallFunctionTest, CallReturn)
{
	SmallFunction<float()> f = [] { return 3.14f; };
	REQUIRE(f() == 3.14f);
}

TEST(SmallFunctionTest, CallReturnNonCopyable)
{
	SmallFunction<std::unique_ptr<int>()> f = [] { return stl::make_unique<int>(33); };
	auto result = f();
	REQUIRE(*result == 33);
}

TEST(SmallFunctionTest, ConversionFromToSTDFunction)
{
	int v = 0;
	std::function<void()> f1 = [&v] { v++; };

	//With large enough buffer, SmallFunction can hold callable type std::function
	SmallFunction<void(), 64> f2 = f1;
	f2();
	REQUIRE(v == 1);

	//Prove the functionalities don't break when reassigning either function
	f1 = f2;
	f1();
	REQUIRE(v == 2);

	f2 = nullptr;
	f1();
	REQUIRE(v == 3);

	//Caveat: Don't do this in production, it can't elide the indirections and would be slower every time
	f2 = f1;
	f2();
	REQUIRE(v == 4);
}

// Copying / moving behaviors are highly dependent on optimization level, therefore the tests only make sense in non-debug
#ifndef _DEBUG

TEST(SmallFunctionTest, AssignLambdaCopyMoveBehavior)
{
	CCopyMoveInspector inspector;
	CCopyMoveInspector::Reset();
	SmallFunction<void()> f;
	f = [inspector] {};

	// Except the capturing copy, the assignment shouldn't create extra copies.
	// It must not construct a function and move the function, but move the temporary lambda directly.
	REQUIRE(CCopyMoveInspector::copyCount == 1);
	REQUIRE(CCopyMoveInspector::moveCount <= 1); // The move may be elided by the compiler
}

TEST(SmallFunctionTest, CallReturnNonTrivial)
{
	struct Inspector : public CCopyMoveInspector
	{
		int m_Value = 0;
	};
	Inspector::Reset();
	SmallFunction<Inspector()> f = []
	{
		Inspector inspector;
		inspector.m_Value++;
		inspector.m_Value -= 3;
		return inspector;
	};
	auto result = f();
	REQUIRE(Inspector::copyCount == 0);

	//before C++17, the local variable is moved out. The move sometimes gets elided by the compiler.
	REQUIRE(Inspector::moveCount <= 1);
}

TEST(SmallFunctionTest, CallParameterCopy)
{
	CCopyMoveInspector inspector;
	CCopyMoveInspector::Reset();
	SmallFunction<void(CCopyMoveInspector)> f = [](CCopyMoveInspector) {};
	f(inspector);
	REQUIRE(CCopyMoveInspector::copyCount == 1);
	REQUIRE(CCopyMoveInspector::moveCount == 0);
}

TEST(SmallFunctionTest, CallParameterLValueReference)
{
	CCopyMoveInspector inspector;
	CCopyMoveInspector::Reset();
	SmallFunction<void(const CCopyMoveInspector&)> f = [](const CCopyMoveInspector&) {};
	f(inspector);
	REQUIRE(CCopyMoveInspector::copyCount == 0);
	REQUIRE(CCopyMoveInspector::moveCount == 0);
}

TEST(SmallFunctionTest, CallParameterRValueRefToValue)
{
	CCopyMoveInspector::Reset();
	SmallFunction<void(CCopyMoveInspector)> f = [](CCopyMoveInspector) {};
	f(CCopyMoveInspector());
	// Because of type erasure, there is no way to overload lvalue / rvalue reference
	// for all parameters. The temporary is expected to be copied only once.
	REQUIRE(CCopyMoveInspector::copyCount == 1);
	REQUIRE(CCopyMoveInspector::moveCount == 0);
}

TEST(SmallFunctionTest, CallParameterRValueRefToLValueRef)
{
	CCopyMoveInspector::Reset();
	SmallFunction<void(const CCopyMoveInspector&)> f = [](const CCopyMoveInspector&) {};
	f(CCopyMoveInspector());
	REQUIRE(CCopyMoveInspector::copyCount == 0);
	REQUIRE(CCopyMoveInspector::moveCount == 0);
}

#endif // _DEBUG
