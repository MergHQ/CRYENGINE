// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Env/IEnvElement.h"
#include "Schematyc/Utils/EnumFlags.h"

namespace Schematyc
{
// Forward declare interfaces.
class CAction;
struct IProperties;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(CAction)

enum class EEnvActionFlags
{
	None                    = 0,
	Singleton               = BIT(0), // Allow only of one instance of this action to run at a time per object.
	ServerOnly              = BIT(1), // Action should only be active on server.
	ClientOnly              = BIT(2), // Action should only be active on clients.
	NetworkReplicateServer  = BIT(3), // Action should be replicated on server.
	NetworkReplicateClients = BIT(4)  // Action should be replicated on clients.
};

typedef CEnumFlags<EEnvActionFlags> EnvActionFlags;

struct IEnvAction : public IEnvElementBase<EEnvElementType::Action>
{
	virtual ~IEnvAction() {}

	virtual const char*        GetIcon() const = 0;
	virtual EnvActionFlags     GetFlags() const = 0;
	virtual CActionPtr         Create() const = 0;
	virtual const IProperties* GetProperties() const = 0;
};
} // Schematyc
