// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/Forward.h>

template<class T, class I, class STORE>
struct DynArray;

template<class T, class I, class S>
bool Serialize(Serialization::IArchive& ar, DynArray<T, I, S>& container, const char* name, const char* label);

#include "DynArrayImpl.h"
