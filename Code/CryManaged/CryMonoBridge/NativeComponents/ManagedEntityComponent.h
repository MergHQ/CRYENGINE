#pragma once

#include <CryEntitySystem/IEntityComponent.h>

#include "Wrappers/MonoObject.h"

#include <array>

// Hacky workaround for no API access for getting MonoProperty from reflection data
// Post-5.3 we should expose this to Mono and send the change back.
struct MonoReflectionPropertyInternal
{
	MonoInternals::MonoObject object;
	MonoInternals::MonoClass *klass;
	MonoInternals::MonoProperty *property;
};

// Type of property, has to be kept in sync with managed code's EntityPropertyType
enum class EEntityPropertyType : uint32
{
	Primitive = 0,
	Geometry,
	Texture,
	Particle,
	AnyFile,
	Sound,
	Material,
	Animation
};

struct SManagedEntityComponentFactory : public ICryFactory
{	
	struct SProperty
	{
		SProperty(MonoReflectionPropertyInternal* pReflectionProperty, const char* szName, const char* szLabel, const char* szDesc, EEntityPropertyType serType);

		MonoReflectionPropertyInternal* pProperty;

		string name;
		string label;
		string description;

		// MonoTypeEnum
		uint32 type;
		EEntityPropertyType serializationType;
	};

	SManagedEntityComponentFactory(std::shared_ptr<CMonoClass> pClass, CryInterfaceID id);

	// ICryFactory
	virtual const char*            GetName() const override { return name; }
	virtual const CryClassID&      GetClassID() const override { return supportedInterfaceIds[0]; }
	virtual bool                   ClassSupports(const CryInterfaceID& iid) const override 
	{
		return stl::find(supportedInterfaceIds, iid);
	}
	virtual void                   ClassSupports(const CryInterfaceID*& pIIDs, size_t& numIIDs) const override
	{
		// TODO: Support inheriting from other custom managed IEntityComponent implementations
		// Then return those ids here. Probably not needed after Timur's refactor.
		pIIDs = supportedInterfaceIds.data();
		numIIDs = supportedInterfaceIds.size();
	}
	virtual ICryUnknownPtr         CreateClassInstance() const override;
	// ~ICryFactory

	template<typename... Args>
	void AddProperty(Args&&... args) { properties.emplace_back(args...); }

	std::array<CryInterfaceID, 2> supportedInterfaceIds;
	std::shared_ptr<CMonoClass> pClass;
	string name;

	IEntityClass* pEntityClass;
	uint64 eventMask;

	std::vector<SProperty> properties;

	std::shared_ptr<CMonoMethod> pInitializeMethod;
	std::shared_ptr<CMonoMethod> pConstructorMethod;
	
	std::shared_ptr<CMonoMethod> pTransformChangedMethod;
	std::shared_ptr<CMonoMethod> pUpdateGameplayMethod;
	std::shared_ptr<CMonoMethod> pUpdateEditingMethod;
	std::shared_ptr<CMonoMethod> pGameModeChangeMethod;
	std::shared_ptr<CMonoMethod> pHideMethod;
	std::shared_ptr<CMonoMethod> pUnHideMethod;
	std::shared_ptr<CMonoMethod> pCollisionMethod;
	std::shared_ptr<CMonoMethod> pPrePhysicsUpdateMethod;
	std::shared_ptr<CMonoMethod> pOnGameplayStartMethod;
	std::shared_ptr<CMonoMethod> pOnRemoveMethod;
};

// Wrapper of an entity component to allow exposing managed equivalents to native code
class CManagedEntityComponent final
	: public IEntityComponent
	, public IEntityPropertyGroup
{
public:
	CManagedEntityComponent(const SManagedEntityComponentFactory& factory);
	virtual ~CManagedEntityComponent() {}

	// IEntityComponent
	virtual void Initialize() override;

	virtual	void ProcessEvent(SEntityEvent &event) override;
	virtual uint64 GetEventMask() const override { return m_factory.eventMask; }

	virtual IEntityPropertyGroup* GetPropertyGroup() final { return this; }

	virtual void* QueryInterface(const CryInterfaceID& iid) const override { return m_factory.ClassSupports(iid) ? (void*)this : nullptr; };
	// ~IEntityComponent

	// IEntityPropertyGroup
	virtual const char* GetLabel() const override { return m_propertyLabel; }

	virtual void SerializeProperties(Serialization::IArchive& archive) override;
	// ~IEntityPropertyGroup

	CMonoObject* GetObject() const { return m_pMonoObject.get(); }

protected:
	string m_propertyLabel;

	const SManagedEntityComponentFactory& m_factory;
	std::shared_ptr<CMonoObject> m_pMonoObject;
};