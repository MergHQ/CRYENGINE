// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "AutoRegister.h"
#include "STDModules.h"

namespace Schematyc
{
namespace Bool
{
bool Flip(bool x)
{
	return !x;
}

bool Or(bool a, bool b)
{
	return a || b;
}

bool And(bool a, bool b)
{
	return a && b;
}

bool XOR(bool a, bool b)
{
	return a != b;
}

bool Compare(bool a, bool b, EComparisonMode mode)
{
	switch (mode)
	{
	case EComparisonMode::Equal:
		{
			return a == b;
		}
	case EComparisonMode::NotEqual:
		{
			return a != b;
		}
	case EComparisonMode::LessThan:
		{
			return a < b;
		}
	case EComparisonMode::LessThanOrEqual:
		{
			return a <= b;
		}
	case EComparisonMode::GreaterThan:
		{
			return a > b;
		}
	case EComparisonMode::GreaterThanOrEqual:
		{
			return a >= b;
		}
	default:
		{
			SCHEMATYC_ENV_ERROR("Invalid comparison mode!");
			return false;
		}
	}
}

int32 ToInt32(bool bValue)
{
	return bValue ? 1 : 0;
}

uint32 ToUInt32(bool bValue)
{
	return bValue ? 1 : 0;
}

void ToString(bool bValue, CSharedString& result)
{
	ToString(result, bValue);
}

static void RegisterFunctions(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(GetTypeDesc<bool>().GetGUID());
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Flip, "6a1d0e15-c4ad-4c87-ab38-3739020dd708"_schematyc_guid, "Flip");
		pFunction->SetDescription("Flip boolean value");
		pFunction->BindInput(1, 'x', "Value");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Or, "ff6f2ed1-942b-4726-9f13-b9e711cd28df"_schematyc_guid, "Or");
		pFunction->SetDescription("Logical OR operation");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&And, "87d62e0c-d32f-4ea7-aa5c-22aeeb1c9cd5"_schematyc_guid, "And");
		pFunction->SetDescription("Logical AND operation");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&XOR, "14a312fc-0885-4ab7-8bef-f124588e4c45"_schematyc_guid, "XOR");
		pFunction->SetDescription("Logical XOR operation");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Compare, "f4c59052-39c1-4a56-b614-39ca32b400af"_schematyc_guid, "Compare");
		pFunction->SetDescription("Compare A and B");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindInput(3, 'mode', "Mode", "Comparison mode", EComparisonMode::Equal);
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&ToInt32, "F93DB38C-E4A1-4553-84D2-255A1E47AC8A"_schematyc_guid, "ToInt32");
		pFunction->SetDescription("Convert boolean to int32");
		pFunction->BindInput(1, 'val', "Value");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&ToUInt32, "FE42F543-AA16-47E8-A1EA-D4374C01B4A3"_schematyc_guid, "ToUInt32");
		pFunction->SetDescription("Convert boolean to uint32");
		pFunction->BindInput(1, 'val', "Value");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&ToString, "9a6f082e-ba3c-4510-9248-8bf8235d5e03"_schematyc_guid, "ToString");
		pFunction->SetDescription("Convert boolean to string");
		pFunction->BindInput(1, 'val', "Value");
		pFunction->BindOutput(2, 'res', "String"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
}
} // Bool

namespace Int32
{
int32 Add(int32 a, int32 b)
{
	return a + b;
}

int32 Subtract(int32 a, int32 b)
{
	return a - b;
}

int32 Multiply(int32 a, int32 b)
{
	return a * b;
}

int32 Divide(int32 a, int32 b)
{
	return b != 0 ? a / b : 0;
}

int32 Modulus(int32 a, int32 b)
{
	return b != 0 ? a % b : 0;
}

int32 Random(int32 min, int32 max)
{
	return min < max ? cry_random(min, max) : min;
}

int32 Abs(int32 x)
{
	return abs(x);
}

bool Compare(int32 a, int32 b, EComparisonMode mode)
{
	switch (mode)
	{
	case EComparisonMode::Equal:
		{
			return a == b;
		}
	case EComparisonMode::NotEqual:
		{
			return a != b;
		}
	case EComparisonMode::LessThan:
		{
			return a < b;
		}
	case EComparisonMode::LessThanOrEqual:
		{
			return a <= b;
		}
	case EComparisonMode::GreaterThan:
		{
			return a > b;
		}
	case EComparisonMode::GreaterThanOrEqual:
		{
			return a >= b;
		}
	default:
		{
			SCHEMATYC_ENV_ERROR("Invalid comparison mode!");
			return false;
		}
	}
}

float ToFloat(int32 value)
{
	return static_cast<float>(value);
}

uint32 ToUInt32(int32 value)
{
	return static_cast<uint32>(value);
}

void ToString(int32 value, CSharedString& result)
{
	ToString(result, value);
}

static void RegisterFunctions(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(GetTypeDesc<int32>().GetGUID());
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Add, "8aaf394a-5288-40df-b31e-47e0c9757a93"_schematyc_guid, "Add");
		pFunction->SetDescription("Add A to B");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Subtract, "cf5e826b-f22d-4fde-8a0c-259ceea75416"_schematyc_guid, "Subtract");
		pFunction->SetDescription("Subtract B from A");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Multiply, "d03e2bfe-79fc-4321-bf30-13be640443bc"_schematyc_guid, "Multiply");
		pFunction->SetDescription("Multiply A by B");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Divide, "450af09f-e7ea-4c6b-8cda-a913e53f4460"_schematyc_guid, "Divide");
		pFunction->SetDescription("Divide A by B");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Modulus, "37a5e2ba-caba-431f-882a-3048a6b18904"_schematyc_guid, "Modulus");
		pFunction->SetDescription("Calculate remainder when A is divided by B");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Random, "67bc3c63-e6f2-42c2-85af-a30fc30d0b11"_schematyc_guid, "Random");
		pFunction->SetDescription("Generate random number");
		pFunction->BindInput(1, 'min', "Min", "Minimum value");
		pFunction->BindInput(2, 'max', "Max", "Maximum value", int32(100));
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Abs, "f2f63e0a-3adc-4659-9444-32546d73c550"_schematyc_guid, "Abs");
		pFunction->SetDescription("Returns the absolute value");
		pFunction->BindInput(1, 'x', "Value"); // #SchematycTODO : Rename 'X'!
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Compare, "938d2cb0-62b3-4612-af76-915bdda8ef80"_schematyc_guid, "Compare");
		pFunction->SetDescription("Compare A and B");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindInput(3, 'mode', "Mode", "Comparison mode", EComparisonMode::Equal);
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&ToFloat, "dc514c68-89ea-4c14-8ce3-9b45f950409a"_schematyc_guid, "ToFloat");
		pFunction->SetDescription("Convert int32 to float");
		pFunction->BindInput(1, 'val', "Value");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&ToUInt32, "FE95372E-42BA-48C7-A051-7B91BDC85DE4"_schematyc_guid, "ToUInt32");
		pFunction->SetDescription("Convert int32 to uint32");
		pFunction->BindInput(1, 'val', "Value");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&ToString, "71b9bfa9-8578-42e3-8ad3-18ff4bb74cca"_schematyc_guid, "ToString");
		pFunction->SetDescription("Convert int32 to string");
		pFunction->BindInput(1, 'val', "Value");
		pFunction->BindOutput(2, 'res', "String"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
}
} // Int32

namespace UInt32
{
uint32 Add(uint32 a, uint32 b)
{
	return a + b;
}

uint32 Subtract(uint32 a, uint32 b)
{
	return a - b;
}

uint32 Multiply(uint32 a, uint32 b)
{
	return a * b;
}

uint32 Divide(uint32 a, uint32 b)
{
	return b != 0 ? a / b : 0;
}

uint32 Modulus(uint32 a, uint32 b)
{
	return b != 0 ? a % b : 0;
}

uint32 Random(uint32 min, uint32 max)
{
	return min < max ? cry_random(min, max) : min;
}

bool Compare(uint32 a, uint32 b, EComparisonMode mode)
{
	switch (mode)
	{
	case EComparisonMode::Equal:
		{
			return a == b;
		}
	case EComparisonMode::NotEqual:
		{
			return a != b;
		}
	case EComparisonMode::LessThan:
		{
			return a < b;
		}
	case EComparisonMode::LessThanOrEqual:
		{
			return a <= b;
		}
	case EComparisonMode::GreaterThan:
		{
			return a > b;
		}
	case EComparisonMode::GreaterThanOrEqual:
		{
			return a >= b;
		}
	default:
		{
			SCHEMATYC_ENV_ERROR("Invalid comparison mode!");
			return false;
		}
	}
}

float ToFloat(uint32 value)
{
	return static_cast<float>(value);
}

int32 ToInt32(uint32 value)
{
	return static_cast<int32>(value);
}

void ToString(uint32 value, CSharedString& result)
{
	ToString(result, value);
}

static void RegisterFunctions(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(GetTypeDesc<uint32>().GetGUID());
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Add, "5dcc5f2e-573e-41e8-8b38-4aaf5a9c0854"_schematyc_guid, "Add");
		pFunction->SetDescription("Add A to B");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Subtract, "bd78264c-f111-4760-b9bc-7a65fe2b722e"_schematyc_guid, "Subtract");
		pFunction->SetDescription("Subtract B from A");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Multiply, "53a75fa7-e113-4f24-99c0-a88a54ea1bf7"_schematyc_guid, "Multiply");
		pFunction->SetDescription("Multiply A by B");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Divide, "2258b2d2-387d-41f3-bf98-63f2458fc1d9"_schematyc_guid, "Divide");
		pFunction->SetDescription("Divide A by B");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Modulus, "770748ab-83c5-4da5-8bf8-c9006b75d8d7"_schematyc_guid, "Modulus");
		pFunction->SetDescription("Calculate remainder when A is divided by B");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Random, "d06c1c65-a8be-4473-aae0-23e7f8e1ff98"_schematyc_guid, "Random");
		pFunction->SetDescription("Generate random number");
		pFunction->BindInput(1, 'min', "Min", "Minimum value");
		pFunction->BindInput(2, 'max', "Max", "Maximum value", int32(100));
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Compare, "e7c171a1-5608-42cc-a357-407140e27cbd"_schematyc_guid, "Compare");
		pFunction->SetDescription("Compare A and B");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindInput(3, 'mode', "Mode", "Comparison mode", EComparisonMode::Equal);
		pFunction->BindOutput(0, 'res', "Output"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&ToFloat, "89fc18a2-0fde-4d07-ada6-7d95ef613132"_schematyc_guid, "ToFloat");
		pFunction->SetDescription("Convert uint32 to float");
		pFunction->BindInput(1, 'val', "Value");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&ToInt32, "ee7dbbc9-d950-44a4-9fd4-0a3891414147"_schematyc_guid, "ToInt32");
		pFunction->SetDescription("Convert uint32 to int32");
		pFunction->BindInput(1, 'val', "Value");
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&ToString, "38a094d6-a8b2-4a87-baee-8d456faf3739"_schematyc_guid, "ToString");
		pFunction->SetDescription("Convert uint32 to string");
		pFunction->BindInput(1, 'val', "Value");
		pFunction->BindOutput(2, 'res', "String"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
}
} // UInt32

namespace String
{
void Append(const CSharedString& a, const CSharedString& b, CSharedString& result)
{
	result = a;
	result.append(b.c_str());
}

bool Compare(const CSharedString& a, const CSharedString& b, EComparisonMode mode)
{
	const int delta = strcmp(a.c_str(), b.c_str());
	switch (mode)
	{
	case EComparisonMode::Equal:
		{
			return delta == 0;
		}
	case EComparisonMode::NotEqual:
		{
			return delta != 0;
		}
	case EComparisonMode::LessThan:
		{
			return delta < 0;
		}
	case EComparisonMode::LessThanOrEqual:
		{
			return delta <= 0;
		}
	case EComparisonMode::GreaterThan:
		{
			return delta > 0;
		}
	case EComparisonMode::GreaterThanOrEqual:
		{
			return delta >= 0;
		}
	default:
		{
			SCHEMATYC_ENV_ERROR("Invalid comparison mode!");
			return false;
		}
	}
}

static void RegisterFunctions(IEnvRegistrar& registrar)
{
	CEnvRegistrationScope scope = registrar.Scope(GetTypeDesc<CSharedString>().GetGUID());
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Append, "f7984665-576c-44cb-8bbb-7401365faa7a"_schematyc_guid, "Append");
		pFunction->SetDescription("Combine A and B");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindOutput(3, 'res', "String"); // #SchematycTODO : Rename 'Result'!
		scope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&Compare, "fd768ad6-4cf0-48f3-bf77-dbbbfdb670ef"_schematyc_guid, "Compare");
		pFunction->SetDescription("Compare A and B");
		pFunction->BindInput(1, 'a', "A");
		pFunction->BindInput(2, 'b', "B");
		pFunction->BindInput(3, 'mode', "Mode", "Comparison mode", EComparisonMode::Equal);
		pFunction->BindOutput(0, 'res', "Result");
		scope.Register(pFunction);
	}
}
} // String

static void RegisterBasicFunctions(IEnvRegistrar& registrar)
{
	Bool::RegisterFunctions(registrar);
	Int32::RegisterFunctions(registrar);
	UInt32::RegisterFunctions(registrar);
	String::RegisterFunctions(registrar);
}
} // Schematyc

SCHEMATYC_AUTO_REGISTER(&Schematyc::RegisterBasicFunctions)
