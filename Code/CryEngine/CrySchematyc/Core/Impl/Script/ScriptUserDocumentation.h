// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/Forward.h>

namespace Schematyc
{
struct SScriptUserDocumentation
{
	void SetCurrentUserAsAuthor();
	void Serialize(Serialization::IArchive& archive);

	string author;
	string description;
};
}
