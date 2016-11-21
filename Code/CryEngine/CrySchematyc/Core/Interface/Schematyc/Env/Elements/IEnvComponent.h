// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Env/IEnvElement.h"
#include "Schematyc/Utils/EnumFlags.h"

namespace Schematyc
{
// Forward declare interfaces.
struct IComponentPreviewer;
struct INetworkSpawnParams;
struct IProperties;
// Forward declare structures.
struct SGUID;
// Forward declare classes.
class CComponent;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(CComponent)

enum class EEnvComponentFlags // #SchematycTODO : Add server/client only options?
{
	None      = 0,
	Singleton = BIT(0),    // Allow only of one instance of this component per class/object.
	Transform = BIT(1),    // Component has transform.
	Socket    = BIT(2),    // Other components can be attached to socket of this component.
	Attach    = BIT(3)     // This component can be attached to socket of other components.
};

typedef CEnumFlags<EEnvComponentFlags> EnvComponentFlags;

enum class EEnvComponentDependencyType
{
	None,
	Soft,   // Dependency must be initialized before component.
	Hard    // Dependency must exist and be initialized before component.
};

struct SEnvComponentDependency
{
	inline SEnvComponentDependency(EEnvComponentDependencyType _type, const SGUID& _guid)
		: type(_type)
		, guid(_guid)
	{}

	EEnvComponentDependencyType type;
	SGUID                       guid;

};

struct IEnvComponent : public IEnvElementBase<EEnvElementType::Component>
{
	virtual ~IEnvComponent() {}

	virtual const char*                    GetIcon() const = 0;
	virtual EnvComponentFlags              GetFlags() const = 0;

	virtual uint32                         GetDependencyCount() const = 0;
	virtual const SEnvComponentDependency* GetDependency(uint32 dependencyIdx) const = 0;
	virtual bool                           IsDependency(const SGUID& guid) const = 0;

	virtual CComponentPtr                  Create() const = 0;

	virtual const IProperties*             GetProperties() const = 0;
	virtual const INetworkSpawnParams*     GetNetworkSpawnParams() const = 0;
	virtual IComponentPreviewer*           GetPreviewer() const = 0;
};
} // Schematyc
