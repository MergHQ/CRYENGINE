#include "StdAfx.h"
#include "EntityComponentFactory.h"
#include "MonoRuntime.h"

#include "Wrappers/MonoLibrary.h"
#include "Wrappers/MonoDomain.h"

#include <CryPhysics/physinterface.h>

#include <CrySerialization/Decorators/Resources.h>
#include <array>

CManagedEntityComponentFactory::CManagedEntityComponentFactory(std::shared_ptr<CMonoClass> pMonoClass, const CryGUID& guid, const Schematyc::SSourceFileInfo& managedSourceFileInfo, const char* szName, const char* szUiCategory, const char* szUiDescription, const char* szIcon)
	: Schematyc::CEnvElementBase<Schematyc::IEnvComponent>(guid, pMonoClass->GetName(), managedSourceFileInfo)
	, m_pClass(pMonoClass)
	, m_eventMask(0)
{
	m_classDescription.SetGUID(guid);

	m_classDescription.SetName(Schematyc::CTypeName(pMonoClass->GetName()));
	m_classDescription.SetLabel(szName);
	m_classDescription.SetEditorCategory(szUiCategory);
	m_classDescription.SetDescription(szUiDescription);
	m_classDescription.SetIcon(szIcon);

	m_classDescription.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });

	// Check which managed functions are implemented
	CMonoClass* pEntityComponentClass = gEnv->pMonoRuntime->GetCryCoreLibrary()->GetClass("CryEngine", "EntityComponent");

	m_pConstructorMethod = m_pClass->FindMethod(".ctor");
	m_pInternalSetEntityMethod = pEntityComponentClass->FindMethod("SetEntity", 2);
	m_pInitializeMethod = m_pClass->FindMethodWithDescInInheritedClasses("OnInitialize()", pEntityComponentClass);

	if (m_pGameStartMethod = m_pClass->FindMethodWithDescInInheritedClasses("OnGameplayStart()", pEntityComponentClass))
	{
		m_eventMask |= BIT64(ENTITY_EVENT_START_GAME);
	}
	if (m_pTransformChangedMethod = m_pClass->FindMethodWithDescInInheritedClasses("OnTransformChanged()", pEntityComponentClass))
	{
		m_eventMask |= BIT64(ENTITY_EVENT_XFORM);
	}
	if (m_pUpdateMethod = m_pClass->FindMethodWithDescInInheritedClasses("OnUpdate(single)", pEntityComponentClass))
	{
		m_eventMask |= BIT64(ENTITY_EVENT_UPDATE);
	}
	if (m_pUpdateMethodEditing = m_pClass->FindMethodWithDescInInheritedClasses("OnEditorUpdate(single)", pEntityComponentClass))
	{
		m_eventMask |= BIT64(ENTITY_EVENT_UPDATE);
	}
	if (m_pGameModeChangeMethod = m_pClass->FindMethodWithDescInInheritedClasses("OnEditorGameModeChange(bool)", pEntityComponentClass))
	{
		m_eventMask |= BIT64(ENTITY_EVENT_RESET);
	}
	if (m_pHideMethod = m_pClass->FindMethodWithDescInInheritedClasses("OnHide()", pEntityComponentClass))
	{
		m_eventMask |= BIT64(ENTITY_EVENT_HIDE);
	}
	if (m_pUnHideMethod = m_pClass->FindMethodWithDescInInheritedClasses("OnUnhide()", pEntityComponentClass))
	{
		m_eventMask |= BIT64(ENTITY_EVENT_UNHIDE);
	}
	if (m_pClass->FindMethodWithDescInInheritedClasses("OnCollision(CollisionEvent)", pEntityComponentClass))
	{
		m_pCollisionMethod = pEntityComponentClass->FindMethod("OnCollisionInternal", 2);

		m_eventMask |= BIT64(ENTITY_EVENT_COLLISION);
	}
	if (m_pPrePhysicsUpdateMethod = m_pClass->FindMethodWithDescInInheritedClasses("OnPrePhysicsUpdate(single)", pEntityComponentClass))
	{
		m_eventMask |= BIT64(ENTITY_EVENT_PREPHYSICSUPDATE);
	}
	if (m_pRemoveMethod = m_pClass->FindMethodWithDescInInheritedClasses("OnRemove()", pEntityComponentClass))
	{
		m_eventMask |= BIT64(ENTITY_EVENT_DONE);
	}
}

std::shared_ptr<IEntityComponent> CManagedEntityComponentFactory::CreateFromPool() const
{
	// We allocate extra memory for entity components in order to support getting properties from the Schematyc callbacks at the end of this file.
	// This allows us to get both the property being processed and the component itself from one pointer currently being handled by Schematyc
	// Otherwise we would need to refactor Schematyc to utilize std::function, introducing extra overhead in terms of both memory and performance.
	uint32 entityComponentSize = sizeof(CManagedEntityComponent) + sizeof(SProperty) * m_properties.size();
	CManagedEntityComponent* pRawComponent = (CManagedEntityComponent*)CryModuleMalloc(entityComponentSize);
	
	struct CustomDeleter
	{
		void operator()(CManagedEntityComponent* p)
		{
			// Explicit call to the destructor
			p->~CManagedEntityComponent();
			// Memory aligned free
			CryModuleFree(p);
		}
	};

	auto pComponent = std::shared_ptr<CManagedEntityComponent>(new(pRawComponent) CManagedEntityComponent(*this), CustomDeleter());

	for(auto it = m_properties.begin(); it != m_properties.end(); ++it)
	{
		SProperty* pProperty = (SProperty*)((uintptr_t)pComponent.get() + sizeof(CManagedEntityComponent) + std::distance(m_properties.begin(), it) * sizeof(SProperty));
		new(pProperty) SProperty(*it->get());
	}

	return pComponent;
}

void CManagedEntityComponentFactory::AddProperty(MonoInternals::MonoReflectionProperty* pReflectionProperty, const char* szPropertyName, const char* szPropertyLabel, const char* szPropertyDesc, EEntityPropertyType type)
{
	std::shared_ptr<CMonoProperty> pProperty = CMonoClass::MakeProperty(pReflectionProperty);
	MonoInternals::MonoTypeEnum monoType = pProperty->GetType(pReflectionProperty);

	// Calculate offset of member from CManagedEntityComponent pointer, 0 being start of the component
	uint32 memberOffset = sizeof(CManagedEntityComponent) + sizeof(SProperty) * m_properties.size();

	m_properties.emplace_back(stl::make_unique<SProperty>(memberOffset, pProperty, type, monoType));
	
	m_classDescription.AddMember(*m_properties.back().get(), memberOffset, m_properties.size(), szPropertyName, szPropertyLabel, szPropertyDesc, nullptr);
}

Schematyc::ETypeCategory GetSchematycPropertyType(MonoInternals::MonoTypeEnum type)
{
	switch (type)
	{
	case MonoInternals::MONO_TYPE_BOOLEAN:
	case MonoInternals::MONO_TYPE_U1:
	case MonoInternals::MONO_TYPE_CHAR:
	case MonoInternals::MONO_TYPE_I1:
	case MonoInternals::MONO_TYPE_I2:
	case MonoInternals::MONO_TYPE_U2:
	case MonoInternals::MONO_TYPE_I4:
	case MonoInternals::MONO_TYPE_U4:
	case MonoInternals::MONO_TYPE_I8:
	case MonoInternals::MONO_TYPE_U8:
		return Schematyc::ETypeCategory::Integral;
	case MonoInternals::MONO_TYPE_R4:
	case MonoInternals::MONO_TYPE_R8:
		return Schematyc::ETypeCategory::FloatingPoint;
	case MonoInternals::MONO_TYPE_STRING:
		return Schematyc::ETypeCategory::String;
	}

	// Tried to use unsupported type
	CRY_ASSERT(false);
	return Schematyc::ETypeCategory::Unknown;
}

template <typename T>
static bool SerializePrimitive(Serialization::IArchive& archive, const CMonoProperty* pProperty, const char* szName, const char* szLabel, const CMonoObject* pObject)
{
	T value;
	if (archive.isOutput())
	{
		bool bEncounteredException;
		std::shared_ptr<CMonoObject> pPropertyValue = std::static_pointer_cast<CMonoObject>(pProperty->Get(pObject, bEncounteredException));
		if (bEncounteredException || pPropertyValue == nullptr)
		{
			return false;
		}

		value = *pPropertyValue->Unbox<T>();
	}

	archive(value, szName, szLabel);

	if (archive.isInput())
	{
		void* pParams[1];
		pParams[0] = &value;

		bool bEncounteredException;
		pProperty->Set(pObject, pParams, bEncounteredException);

		return !bEncounteredException;
	}

	return true;
}

CManagedEntityComponentFactory::SProperty::SProperty(uint32 membOffset, std::shared_ptr<CMonoProperty> pMonoProperty, EEntityPropertyType serType, MonoInternals::MonoTypeEnum type)
	: Schematyc::CCommonTypeDesc(GetSchematycPropertyType(type))
	, memberOffset(membOffset)
	, pProperty(pMonoProperty)
	, serializationType(serType)
	, monoType(type)
{
	m_operators.defaultConstruct = &SProperty::DefaultConstruct;
	m_operators.destruct = &SProperty::Destroy;
	m_operators.copyConstruct = &SProperty::CopyConstruct;
	m_operators.copyAssign = &SProperty::CopyAssign;
	m_operators.equals = &SProperty::Equals;
	m_operators.serialize = &SProperty::Serialize;
	m_operators.toString = &SProperty::ToString;
}

void CManagedEntityComponentFactory::SProperty::DefaultConstruct(void* pComponentAddress)
{
	// Not implemented, catch if this is actually used
	CRY_ASSERT(false);
}

void CManagedEntityComponentFactory::SProperty::Destroy(void* pComponentAddress)
{
	// Not implemented, catch if this is actually used
	CRY_ASSERT(false);
}

void CManagedEntityComponentFactory::SProperty::CopyConstruct(void* pTargetComponentAddress, const void* pSourceComponentAddress)
{
	// Not implemented, catch if this is actually used
	CRY_ASSERT(false);
}

void CManagedEntityComponentFactory::SProperty::CopyAssign(void* pTargetComponentAddress, const void* pSourceComponentAddress)
{
	CManagedEntityComponent* pTargetComponent = static_cast<CManagedEntityComponent*>(pTargetComponentAddress);
	const CManagedEntityComponent* pSourceComponent = static_cast<const CManagedEntityComponent*>(pSourceComponentAddress);

	pTargetComponent->GetObject()->CopyFrom(*pSourceComponent->GetObject());
}

bool CManagedEntityComponentFactory::SProperty::Equals(const void* pLeftComponentAddress, const void* pRightComponentAddress)
{
	const CManagedEntityComponent* pLeftComponent = static_cast<const CManagedEntityComponent*>(pLeftComponentAddress);
	const CManagedEntityComponent* pRightComponent = static_cast<const CManagedEntityComponent*>(pRightComponentAddress);

	// Horrible string comparison check, we should find a way to compare through Mono somehow if even possible
	std::shared_ptr<CMonoString> pLeftString = pLeftComponent->GetObject()->ToString();
	std::shared_ptr<CMonoString> pRightString = pRightComponent->GetObject()->ToString();

	return !strcmp(pLeftString->GetString(), pRightString->GetString());
}

bool CManagedEntityComponentFactory::SProperty::Serialize(Serialization::IArchive& archive, void* pComponentAddress, const char* szName, const char* szLabel)
{
	const CManagedEntityComponentFactory::SProperty* pProperty = static_cast<const CManagedEntityComponentFactory::SProperty*>(pComponentAddress);
	const CManagedEntityComponent* pComponent = (const CManagedEntityComponent*)((uintptr_t)pProperty - pProperty->memberOffset);

	switch (pProperty->monoType)
	{
	case MonoInternals::MONO_TYPE_BOOLEAN:
	{
		return SerializePrimitive<bool>(archive, pProperty->pProperty.get(), szName, szLabel, static_cast<CMonoObject*>(pComponent->GetObject()));
	}
	break;
	case MonoInternals::MONO_TYPE_STRING:
	{
		string value;
		if (archive.isOutput())
		{
			bool bEncounteredException;
			std::shared_ptr<CMonoObject> pPropertyValue = std::static_pointer_cast<CMonoObject>(pProperty->pProperty->Get(pComponent->GetObject(), bEncounteredException));
			if (bEncounteredException || pPropertyValue == nullptr)
			{
				return false;
			}

			std::shared_ptr<CMonoString> pValue = pPropertyValue->ToString();
			value = pValue->GetString();
		}

		switch (pProperty->serializationType)
		{
		case EEntityPropertyType::Primitive:
			archive(value, szName, szLabel);
			break;
		case EEntityPropertyType::Object:
			archive(Serialization::ModelFilename(value), szName, szLabel);
			break;
		case EEntityPropertyType::Texture:
			archive(Serialization::TextureFilename(value), szName, szLabel);
			break;
		case EEntityPropertyType::Particle:
			archive(Serialization::ParticlePicker(value), szName, szLabel);
			break;
		case EEntityPropertyType::AnyFile:
			archive(Serialization::GeneralFilename(value), szName, szLabel);
			break;
		case EEntityPropertyType::Sound:
			archive(Serialization::SoundFilename(value), szName, szLabel);
			break;
		case EEntityPropertyType::Material:
			archive(Serialization::MaterialPicker(value), szName, szLabel);
			break;
		case EEntityPropertyType::Animation:
			archive(Serialization::CharacterAnimationPicker(value), szName, szLabel);
			break;
		}

		if (archive.isInput())
		{
			CMonoDomain* pDomain = pComponent->GetObject()->GetClass()->GetAssembly()->GetDomain();

			std::shared_ptr<CMonoString> pValue = pDomain->CreateString(value);

			void* pParams[1];
			pParams[0] = pValue->GetManagedObject();

			bool bEncounteredException;
			pProperty->pProperty->Set(static_cast<CMonoObject*>(pComponent->GetObject()), pParams, bEncounteredException);

			return !bEncounteredException;
		}

		return true;
	}
	break;
	case MonoInternals::MONO_TYPE_U1:
	case MonoInternals::MONO_TYPE_CHAR: // Char is unsigned by default for .NET
	{
		return SerializePrimitive<uchar>(archive, pProperty->pProperty.get(), szName, szLabel, static_cast<CMonoObject*>(pComponent->GetObject()));
	}
	break;
	case MonoInternals::MONO_TYPE_I1:
	{
		return SerializePrimitive<char>(archive, pProperty->pProperty.get(), szName, szLabel, static_cast<CMonoObject*>(pComponent->GetObject()));
	}
	break;
	case MonoInternals::MONO_TYPE_I2:
	{
		return SerializePrimitive<int16>(archive, pProperty->pProperty.get(), szName, szLabel, static_cast<CMonoObject*>(pComponent->GetObject()));
	}
	break;
	case MonoInternals::MONO_TYPE_U2:
	{
		return SerializePrimitive<uint16>(archive, pProperty->pProperty.get(), szName, szLabel, static_cast<CMonoObject*>(pComponent->GetObject()));
	}
	break;
	case MonoInternals::MONO_TYPE_I4:
	{
		return SerializePrimitive<int32>(archive, pProperty->pProperty.get(), szName, szLabel, static_cast<CMonoObject*>(pComponent->GetObject()));
	}
	break;
	case MonoInternals::MONO_TYPE_U4:
	{
		return SerializePrimitive<uint32>(archive, pProperty->pProperty.get(), szName, szLabel, static_cast<CMonoObject*>(pComponent->GetObject()));
	}
	break;
	case MonoInternals::MONO_TYPE_I8:
	{
		return SerializePrimitive<int64>(archive, pProperty->pProperty.get(), szName, szLabel, static_cast<CMonoObject*>(pComponent->GetObject()));
	}
	break;
	case MonoInternals::MONO_TYPE_U8:
	{
		return SerializePrimitive<uint64>(archive, pProperty->pProperty.get(), szName, szLabel, static_cast<CMonoObject*>(pComponent->GetObject()));
	}
	break;
	case MonoInternals::MONO_TYPE_R4:
	{
		return SerializePrimitive<float>(archive, pProperty->pProperty.get(), szName, szLabel, static_cast<CMonoObject*>(pComponent->GetObject()));
	}
	break;
	case MonoInternals::MONO_TYPE_R8:
	{
		return SerializePrimitive<double>(archive, pProperty->pProperty.get(), szName, szLabel, static_cast<CMonoObject*>(pComponent->GetObject()));
	}
	break;
	}

	return false;
}

void CManagedEntityComponentFactory::SProperty::ToString(Schematyc::IString& output, const void* pComponentAddress)
{
	const CManagedEntityComponentFactory::SProperty* pProperty = static_cast<const CManagedEntityComponentFactory::SProperty*>(pComponentAddress);
	const CManagedEntityComponent* pComponent = (const CManagedEntityComponent*)((uintptr_t)pProperty - pProperty->memberOffset);

	bool bEncounteredException;
	std::shared_ptr<CMonoObject> pPropertyValue = std::static_pointer_cast<CMonoObject>(pProperty->pProperty->Get(pComponent->GetObject(), bEncounteredException));
	if (!bEncounteredException)
	{
		std::shared_ptr<CMonoString> pValue = pPropertyValue->ToString();
		output.assign(pValue->GetString());
	}
}