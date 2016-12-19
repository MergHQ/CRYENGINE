// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IGameObject.h>

#include "Schematyc/Entity/IEntityComponent.h"

namespace Schematyc
{
	// Forward declare interfaces.
	struct IObjectProperties;
	// Forward declare shared pointers.
	DECLARE_SHARED_POINTERS(IObjectProperties)

	class CEntityObjectComponent final
		: public INetworkObject
		, public IEntityComponentWrapper
		, public IEntityPropertyGroup
	{
	public:
		CRY_ENTITY_COMPONENT_CLASS(CEntityObjectComponent, IEntityComponentWrapper, "SchematycComponent", 0x556382B49CE24ABC, 0xA2FB6711F095054A);

		CEntityObjectComponent();
		virtual ~CEntityObjectComponent();

		// INetworkObject
		virtual bool   RegisterSerializeCallback(int32 aspects, const NetworkSerializeCallback& callback, CConnectionScope& connectionScope) override { return false; }
		virtual void   MarkAspectsDirty(int32 aspects) override {}
		virtual uint16 GetChannelId() const override { return 0; }
		virtual bool   ServerAuthority() const override { return true; }
		virtual bool   ClientAuthority() const override { return false; }
		virtual bool   LocalAuthority() const override { return gEnv->bServer; }
		// ~INetworkObject

		// IEntityComponentWrapper
		virtual EntityEventSignal::Slots& GetEventSignalSlots() override;
		virtual GeomUpdateSignal&         GetGeomUpdateSignal() override;
		// ~IEntityComponentWrapper

		// IEntityComponent
		virtual uint64 GetEventMask() const override { return BIT64(ENTITY_EVENT_RESET) | BIT64(ENTITY_EVENT_START_LEVEL) | BIT64(ENTITY_EVENT_XFORM) | BIT64(ENTITY_EVENT_DONE) | BIT64(ENTITY_EVENT_HIDE) | BIT64(ENTITY_EVENT_UNHIDE) | BIT64(ENTITY_EVENT_SCRIPT_EVENT) | BIT64(ENTITY_EVENT_ENTERAREA) | BIT64(ENTITY_EVENT_LEAVEAREA); }
		virtual void ProcessEvent(SEntityEvent &event) override;
		virtual IEntityPropertyGroup* GetPropertyGroup() override { return this; }
		virtual void GetMemoryUsage(ICrySizer* pSizer) const override;
		// ~IEntityComponent

		// IEntityPropertyGroup
		virtual const char* GetLabel() const override { return m_propertiesLabel; }

		virtual void SerializeProperties(Serialization::IArchive& archive) override;
		// ~IEntityPropertyGroup

		void SetRuntimeClass(const Schematyc::IRuntimeClass* pRuntimeClass, bool bIsPreview);

		void SerializeToUI(Serialization::IArchive& archive);

	private:
		void DisplayDetails(Serialization::IArchive& archive);
		void ShowInSchematyc();

		void CreateObject(ESimulationMode simulationMode);
		void DestroyObject();

	private:
		IObject* m_pObject;
		const Schematyc::IRuntimeClass* m_pRuntimeClass;

		IObjectPropertiesPtr m_pProperties;

		EntityEventSignal m_eventSignal;
		GeomUpdateSignal  m_geomUpdateSignal;

		string m_propertiesLabel;
		bool m_bIsPreview;
	};
} // Schematyc
