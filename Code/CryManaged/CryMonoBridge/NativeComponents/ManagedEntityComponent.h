#pragma once

#include <CryEntitySystem/IEntityComponent.h>

#include <CryMono/IMonoClass.h>
#include <CryMono/IMonoObject.h>
#include <CryMono/IMonoMethod.h>

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

struct SManagedEntityComponentFactory
{
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

	SManagedEntityComponentFactory(std::shared_ptr<IMonoClass> pClass, CryInterfaceID id);

	template<typename... Args>
	void AddProperty(Args&&... args) { properties.emplace_back(args...); }

	CryInterfaceID id;
	std::shared_ptr<IMonoClass> pClass;

	IEntityClass* pEntityClass;
	uint64 eventMask;

	std::vector<SProperty> properties;

	std::shared_ptr<IMonoMethod> pInitializeMethod;
	std::shared_ptr<IMonoMethod> pConstructorMethod;

	std::shared_ptr<IMonoMethod> pTransformChangedMethod;
	std::shared_ptr<IMonoMethod> pUpdateMethod;
	std::shared_ptr<IMonoMethod> pGameModeChangeMethod;
	std::shared_ptr<IMonoMethod> pHideMethod;
	std::shared_ptr<IMonoMethod> pUnHideMethod;
	std::shared_ptr<IMonoMethod> pCollisionMethod;
	std::shared_ptr<IMonoMethod> pPrePhysicsUpdateMethod;
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
	// ~IEntityComponent

	// IEntityPropertyGroup
	virtual const char* GetLabel() const override { return m_propertyLabel; }

	virtual void SerializeProperties(Serialization::IArchive& archive) override;
	// ~IEntityPropertyGroup

	IMonoObject* GetObject() const { return m_pMonoObject.get(); }

protected:
	string m_propertyLabel;

	const SManagedEntityComponentFactory& m_factory;
	std::shared_ptr<IMonoObject> m_pMonoObject;
};