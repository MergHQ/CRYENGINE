#pragma once

#include <CryEntitySystem/IEntityComponent.h>

#include <CryMono/IMonoClass.h>
#include <CryMono/IMonoObject.h>

#include <mono/metadata/class.h>
#include <mono/metadata/object.h>

// Hacky workaround for no API access for getting MonoProperty from reflection data
// Post-5.3 we should expose this to Mono and send the change back.
struct MonoReflectionPropertyInternal
{
	MonoObject object;
	MonoClass *klass;
	MonoProperty *property;
};

// Type of property, has to be kept in sync with managed code's EntityPropertyType
enum class EEntityPropertyType : uint32
{
	Primitive = 0,
	Object,
	Texture,
	Particle,
	AnyFile,
	Sound,
	Material,
	Animation
};

class CManagedEntityComponentFactory
{
public:
	struct SProperty
	{
		SProperty(MonoReflectionPropertyInternal* pReflectionProperty, const char* szName, const char* szLabel, const char* szDesc, EEntityPropertyType serType);

		MonoReflectionPropertyInternal* pProperty;

		string name;
		string label;
		string description;

		MonoTypeEnum type;
		EEntityPropertyType serializationType;
	};

	CManagedEntityComponentFactory(std::shared_ptr<IMonoClass> pClass, CryInterfaceID id);

	IMonoClass* GetClass() const { return m_pClass.get(); }
	CryInterfaceID GetId() const { return m_id; }

	IEntityClass* GetEntityClass() const { return m_pEntityClass; }
	void SetEntityClass(IEntityClass* pClass) { m_pEntityClass  = pClass; }

	uint64 GetEventMask() const { return m_eventMask; }

	template<typename... Args>
	void AddProperty(Args&&... args) { m_properties.emplace_back(args...); }

	const std::vector<SProperty>& GetProperties() const { return m_properties; }

protected:
	CryInterfaceID m_id;
	std::shared_ptr<IMonoClass> m_pClass;

	IEntityClass* m_pEntityClass;
	uint64 m_eventMask;

	std::vector<SProperty> m_properties;
};

// Wrapper of an entity component to allow exposing managed equivalents to native code
class CManagedEntityComponent final
	: public IEntityComponent
	, public IEntityPropertyGroup
{
public:
	CManagedEntityComponent(const CManagedEntityComponentFactory& factory);
	virtual ~CManagedEntityComponent() {}

	// IEntityComponent
	virtual void Initialize() override;

	virtual	void ProcessEvent(SEntityEvent &event) override;
	virtual uint64 GetEventMask() const override { return m_eventMask; }

	virtual IEntityPropertyGroup* GetPropertyGroup() final { return this; }
	// ~IEntityComponent

	// IEntityPropertyGroup
	virtual const char* GetLabel() const override { return m_propertyLabel; }

	virtual void SerializeProperties(Serialization::IArchive& archive) override;
	// ~IEntityPropertyGroup

	IMonoObject* GetObject() const { return m_pMonoObject.get(); }

protected:
	string m_propertyLabel;

	const CManagedEntityComponentFactory& m_factory;
	std::shared_ptr<IMonoObject> m_pMonoObject;

	uint64 m_eventMask;
};