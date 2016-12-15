// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IGameObject.h>

#include "Schematyc/Entity/IEntityObject.h"

namespace Schematyc
{
// Forward declare interfaces.
struct IObjectProperties;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(IObjectProperties)

class CEntityObjectComponent final : public INetworkObject, public IEntityObjectComponent, public IEntityPropertyGroup
{
public:

	CRY_ENTITY_COMPONENT_CLASS(CEntityObjectComponent, IEntityObjectComponent, "SchematycEntityObjectComponent", 0x556382B49CE24ABC, 0xA2FB6711F095054A);

	virtual ~CEntityObjectComponent();

	// INetworkObject
	virtual bool   RegisterSerializeCallback(int32 aspects, const NetworkSerializeCallback& callback, CConnectionScope& connectionScope) override;
	virtual void   MarkAspectsDirty(int32 aspects) override;
	virtual uint16 GetChannelId() const override;
	virtual bool   ServerAuthority() const override;
	virtual bool   ClientAuthority() const override;
	virtual bool   LocalAuthority() const override;
	// ~INetworkObject

	// IEntityObject
	virtual EntityEventSignal::Slots& GetEventSignalSlots() override;
	virtual GeomUpdateSignal&         GetGeomUpdateSignal() override;
	// ~IEntityObject

	// IEntityComponent
	virtual uint64                GetEventMask() const override;
	virtual void                  ProcessEvent(SEntityEvent& event) override;
	virtual IEntityPropertyGroup* GetPropertyGroup() override;
	virtual void                  GetMemoryUsage(ICrySizer* pSizer) const override;
	// ~IEntityComponent

	// IEntityPropertyGroup
	virtual const char* GetLabel() const override;
	virtual void        SerializeProperties(Serialization::IArchive& archive) override;
	// ~IEntityPropertyGroup

	void SetRuntimeClass(const IRuntimeClass& runtimeClass, bool bIsPreview);

private:

	void CreateObject(ESimulationMode simulationMode);
	void DestroyObject();

	void DisplayDetails(Serialization::IArchive& archive);
	void ShowInSchematyc();

private:

	const IRuntimeClass* m_pRuntimeClass = nullptr;
	bool                 m_bIsPreview = false;
	IObjectPropertiesPtr m_pProperties;
	
	IObject*             m_pObject = nullptr;

	EntityEventSignal    m_eventSignal;
	GeomUpdateSignal     m_geomUpdateSignal;
};
} // Schematyc
