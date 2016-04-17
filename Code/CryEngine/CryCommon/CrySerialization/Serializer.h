// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/Forward.h>
#include <yasli/Serializer.h>
#include <yasli/KeyValue.h>

#if defined(__GNUC__)
// Reinforce GCC's linker to properly export missing symbols (yasli::Serialize etc.)
// into CrySystem library:
	#include <yasli/ClassFactory.h>
#endif

namespace Serialization
{
typedef yasli::Serializer         SStruct;
typedef yasli::ContainerInterface IContainer;
typedef yasli::PointerInterface   IPointer;
typedef yasli::KeyValueInterface  IKeyValue;
typedef yasli::StringInterface    IString;
typedef yasli::WStringInterface   IWString;
}
