// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/Framework/Common.h>

namespace Cry {
namespace Reflection {

class NatVisWriter
{
public:
	bool WriteFile();

private:
	bool IsFundamental(Type::CTypeId typeId);
};

} // ~Reflection namespace
} // ~Cry namespace

#include "NatVisWriter_impl.inl"
