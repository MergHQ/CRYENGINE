// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/yasli/Archive.h>
#include "Serializer.h"

namespace Serialization{

typedef yasli::Archive       IArchive;
typedef yasli::Serializer    SStruct;
typedef yasli::Context       SContext;
typedef yasli::Context       SContextLink;
typedef yasli::TypeID        TypeID;
typedef std::vector<SStruct> SStructs;

/*
using yasli::IsDefaultSerializeable;
using yasli::HasSerializeOverride;
using yasli::IsSerializeable;
*/
}
