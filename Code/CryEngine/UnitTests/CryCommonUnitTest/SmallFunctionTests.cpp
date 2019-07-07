// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

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

void PlainFunctionVoidNoArg() {}

// Helper interface to test function behaviors for type conversions and virtual methods
struct IInterface
{
	virtual ~IInterface() = default;
	virtual int Method() const = 0;
};

class CImpl : public IInterface
{
public:
	explicit CImpl(int value) : m_value(value) {}
private:
	virtual int Method() const override
	{
		return m_value;
	}

	int m_value;
};

// Helper class to detect object copying and destruction behavior
// Can detect leaks through comparing number of instances before and after scope
struct SInstancesDetector
{
	static std::set<SInstancesDetector*> sInstances;
	SInstancesDetector() { sInstances.insert(this); }
	SInstancesDetector(const SInstancesDetector&) { sInstances.insert(this); }
	~SInstancesDetector() { sInstances.erase(this); }
};

std::set<SInstancesDetector*> SInstancesDetector::sInstances;

// Creates a function object capturing exactly one instance detector
SmallFunction<void()> CreateSmallFunctionCaptureInstanceDetector()
{
	SInstancesDetector::sInstances.clear();
	SInstancesDetector instance;
	return [instance] {};
}

// Meta test
static_assert(!std::has_virtual_destructor<SmallFunction<void()>>::value, "SmallFunction is designed to be without virtual table");

// Tests certain invariants when default constructed
TEST(SmallFunctionTest, DefaultConstruction)
{
	SmallFunction<void()> f;
	REQUIRE(!f);
	REQUIRE(f == nullptr);
}

// Tests that a function can be constructed from a function pointer of matching signature
TEST(SmallFunctionTest, ConstructionFromFunctionPointer)
{
	SmallFunction<int(int)> f = &PlainFunction;
	REQUIRE(f(4) == 16);
}

// Tests that a function can be constructed from a reference to function of matching signature
TEST(SmallFunctionTest, ConstructionFromFunctionReference)
{
	SmallFunction<int(int)> f = PlainFunction;
	REQUIRE(f(4) == 16);
}

// Tests that a function can be constructed from a lambda
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

// Tests that a function object can be constructed from a function that is contra-variance to the signature,
// i.e. SmallFunction<void(Derived*)> can be assigned a function void(Base*), but not vice versa
TEST(SmallFunctionTest, ConstructionContravariance)
{
	SmallFunction<int(CImpl*)> f = [](const IInterface* pObject) { return pObject->Method(); };
	CImpl* pObject = new CImpl(1024);
	REQUIRE(f(pObject) == 1024);
	delete pObject;
}

// Tests that a function can be assigned a function pointer of matching signature
TEST(SmallFunctionTest, AssignFunctionPointer)
{
	SmallFunction<int(int)> f;
	f = &PlainFunction;
	REQUIRE(f(4) == 16);
}

// Tests that a function can be assigned a reference to function of matching signature
TEST(SmallFunctionTest, AssignFunctionReference)
{
	SmallFunction<int(int)> f;
	f = PlainFunction;
	REQUIRE(f(4) == 16);
}

// Tests that a function can be assigned a lambda
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

// Tests that when a non-empty function is assigned to a new function pointer, 
// the previous object is properly released.
TEST(SmallFunctionTest, AssignFunctionPointerNoLeak)
{
	SInstancesDetector::sInstances.clear();
	SmallFunction<void()> f = CreateSmallFunctionCaptureInstanceDetector();
	REQUIRE(SInstancesDetector::sInstances.size() == 1);
	f = &PlainFunctionVoidNoArg;
	REQUIRE(SInstancesDetector::sInstances.size() == 0);
}

// Tests that when a non-empty function is assigned to a new reference of function, 
// the previous object is properly released.
TEST(SmallFunctionTest, AssignFunctionReferenceNoLeak)
{
	SInstancesDetector::sInstances.clear();
	SmallFunction<void()> f = CreateSmallFunctionCaptureInstanceDetector();
	REQUIRE(SInstancesDetector::sInstances.size() == 1);
	f = PlainFunctionVoidNoArg;
	REQUIRE(SInstancesDetector::sInstances.size() == 0);
}

// Tests that when a non-empty function is assigned to a new lambda,
// the previous object is properly released.
TEST(SmallFunctionTest, AssignLambdaNoLeak)
{
	SInstancesDetector::sInstances.clear();
	SmallFunction<void()> f = CreateSmallFunctionCaptureInstanceDetector();
	REQUIRE(SInstancesDetector::sInstances.size() == 1);
	f = [] {};
	REQUIRE(SInstancesDetector::sInstances.size() == 0);
}

// Tests that when a non-empty function is destructed, the captured object is properly released.
TEST(SmallFunctionTest, Destruction)
{
	SInstancesDetector::sInstances.clear();

	{
		SmallFunction<void()> f = CreateSmallFunctionCaptureInstanceDetector();
		REQUIRE(SInstancesDetector::sInstances.size() == 1);
	}
	REQUIRE(SInstancesDetector::sInstances.size() == 0);
}

// Tests that copy-constructing a function copies the captured object.
// Tests that destructing function copies work independently from each other and release the captured objects respectively.
TEST(SmallFunctionTest, CopyConstruction)
{
	SInstancesDetector::sInstances.clear();
	{
		SmallFunction<void()> f1 = CreateSmallFunctionCaptureInstanceDetector();
		{
			SmallFunction<void()> f2 = f1;
			REQUIRE(SInstancesDetector::sInstances.size() == 2);
		}
		REQUIRE(SInstancesDetector::sInstances.size() == 1);
	}
	REQUIRE(SInstancesDetector::sInstances.size() == 0);
}

// Tests that copy-assigning a function copies the captured object.
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

// Tests that copy-assigning a function copies the captured object.
// Tests that copy-assigning a non-empty function properly releases the previous captured object.
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

// Tests that move-constructing a function transfers the captured object and the function object 
// that is moved-from is empty.
TEST(SmallFunctionTest, MoveConstruction)
{
	SmallFunction<void()> f1 = CreateSmallFunctionCaptureInstanceDetector();
	REQUIRE(SInstancesDetector::sInstances.size() == 1);
	SmallFunction<void()> f2 = std::move(f1);
	REQUIRE(!f1);
	REQUIRE(f2);
	REQUIRE(SInstancesDetector::sInstances.size() == 1);
}

// Tests that move-assigning a function transfers the captured object and the function object
// that is moved-from is empty.
TEST(SmallFunctionTest, MoveAssignment)
{
	SmallFunction<void()> f1 = CreateSmallFunctionCaptureInstanceDetector();
	REQUIRE(SInstancesDetector::sInstances.size() == 1);
	SmallFunction<void()> f2;
	f2 = std::move(f1);
	REQUIRE(!f1);
	REQUIRE(f2);
	REQUIRE(SInstancesDetector::sInstances.size() == 1);
}

// Tests that a function object's non-emptiness can be tested as a bool
TEST(SmallFunctionTest, ExplicitBool)
{
	SmallFunction<void()> f;
	REQUIRE(!f);

	SmallFunction<void()> f2 = [] {};
	REQUIRE(f2);
}

// Tests that a function object's non-emptiness always matches the inequality to nullptr
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

// Tests that function object can return value
TEST(SmallFunctionTest, CallReturn)
{
	SmallFunction<float()> f = [] { return 3.14f; };
	REQUIRE(f() == 3.14f);
}

// Tests that a function object can return non-copyable object
TEST(SmallFunctionTest, CallReturnNonCopyable)
{
	SmallFunction<std::unique_ptr<int>()> f = [] { return stl::make_unique<int>(33); };
	auto result = f();
	REQUIRE(*result == 33);
}

// Tests that a function object can be called with arguments different but convertible to the signature
// Simple types
TEST(SmallFunctionTest, CallWithConvertibleArgumentsSimple)
{
	SmallFunction<double(double)> f = [](double x) { return x * x; };
	REQUIRE(f(10L) == 100.0);
}

// Tests that a function object can be called with arguments different but convertible to the signature
// Complex types and const-ness
TEST(SmallFunctionTest, CallWithConvertibleArgumentsComplex)
{
	SmallFunction<int(const IInterface*)> f = [](const IInterface* pObject) { return pObject->Method(); };
	CImpl pObject{ 1024 };
	REQUIRE(f(&pObject) == 1024); // conversion from CImpl* to const IInterface*
}

// Tests that a function object can be converted from and to std::function of same signature
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

	(void)result; //Mutes warning "unused variable 'result'". Here we are only interested in the side effects.
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
