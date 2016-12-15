// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <Schematyc/Reflection/Reflection.h>

#include "UnitTests/UnitTestRegistrar.h"

namespace Schematyc
{
namespace ReflectionUnitTests
{
enum class EType
{
	BaseA,
	BaseB,
	BaseC
};

struct SBaseA
{
	inline SBaseA()
		: type(EType::BaseA)
	{}

	static SGUID ReflectSchematycType(CTypeInfo<SBaseA>& typeInfo)
	{
		return "f4c516fa-05be-4fdd-86f6-9bef1deeec8d"_schematyc_guid;
	}

	EType type;
};

struct SBaseB
{
	inline SBaseB()
		: type(EType::BaseB)
	{}

	static SGUID ReflectSchematycType(CTypeInfo<SBaseB>& typeInfo)
	{
		return "c8b8de47-1fff-4ec9-8a45-462086611dcb"_schematyc_guid;
	}

	EType type;
};

struct SBaseC : public SBaseA, public SBaseB
{
	inline SBaseC()
		: type(EType::BaseC)
	{}

	static SGUID ReflectSchematycType(CTypeInfo<SBaseC>& typeInfo)
	{
		typeInfo.AddBase<SBaseA>();
		typeInfo.AddBase<SBaseB>();
		return "618307d6-a7c5-4fd4-8859-db67ee998778"_schematyc_guid;
	}

	EType type;
};

struct SDerived : public SBaseC
{
	static SGUID ReflectSchematycType(CTypeInfo<SDerived>& typeInfo)
	{
		typeInfo.AddBase<SBaseC>();
		return "660a3811-7c0c-450e-bd41-5f375cd11771"_schematyc_guid;
	}
};

UnitTestResultFlags Run()
{
	SDerived derived;

	SBaseA* pBaseA = DynamicCast<SBaseA>(&derived);
	if(!pBaseA || (pBaseA->type != EType::BaseA))
	{
		return EUnitTestResultFlags::FatalError;
	}
	
	SBaseB* pBaseB = DynamicCast<SBaseB>(&derived);
	if (!pBaseB || (pBaseB->type != EType::BaseB))
	{
		return EUnitTestResultFlags::FatalError;
	}

	SBaseC* pBaseC = DynamicCast<SBaseC>(&derived);
	if (!pBaseC || (pBaseC->type != EType::BaseC))
	{
		return EUnitTestResultFlags::FatalError;
	}

	return EUnitTestResultFlags::Success;
}
} // ReflectionUnitTests

SCHEMATYC_REGISTER_UNIT_TEST(&ReflectionUnitTests::Run, "Reflection")
} // Schematyc
