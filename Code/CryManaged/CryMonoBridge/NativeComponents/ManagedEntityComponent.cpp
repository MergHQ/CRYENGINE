#include "StdAfx.h"
#include "ManagedEntityComponent.h"

#include "MonoRuntime.h"

#include "Wrappers/MonoString.h"
#include "Wrappers/MonoDomain.h"
#include "Wrappers/MonoClass.h"
#include "Wrappers/MonoMethod.h"

#include <CryPhysics/physinterface.h>

#include <CrySerialization/Decorators/Resources.h>

SManagedEntityComponentFactory::SManagedEntityComponentFactory(std::shared_ptr<CMonoClass> pMonoClass, CryInterfaceID identifier)
	: pClass(pMonoClass)
	, eventMask(0)
{
	supportedInterfaceIds[0] = identifier;
	supportedInterfaceIds[1] = cryiidof<IEntityComponent>();

	name.Format("%s.%s", pClass->GetNamespace(), pClass->GetName());

	CMonoClass* pEntityComponentClass = GetMonoRuntime()->GetCryCoreLibrary()->GetClass("CryEngine", "EntityComponent");

	pConstructorMethod = pMonoClass->FindMethod(".ctor");
	pInitializeMethod = pClass->FindMethodInInheritedClasses("Initialize", 2);

	if (pTransformChangedMethod = pClass->FindMethodWithDescInInheritedClasses("OnTransformChanged()", pEntityComponentClass))
	{
		eventMask |= BIT64(ENTITY_EVENT_XFORM);
	}
	if (pUpdateGameplayMethod = pClass->FindMethodWithDescInInheritedClasses("OnUpdate(single)", pEntityComponentClass))
	{
		eventMask |= BIT64(ENTITY_EVENT_UPDATE);
	}
	if (pUpdateEditingMethod = pClass->FindMethodWithDescInInheritedClasses("OnEditorUpdate(single)", pEntityComponentClass))
	{
		eventMask |= BIT64(ENTITY_EVENT_UPDATE);
	}
	if (pGameModeChangeMethod = pClass->FindMethodWithDescInInheritedClasses("OnEditorGameModeChange(bool)", pEntityComponentClass))
	{
		eventMask |= BIT64(ENTITY_EVENT_RESET);
	}
	if (pHideMethod = pClass->FindMethodWithDescInInheritedClasses("OnHide()", pEntityComponentClass))
	{
		eventMask |= BIT64(ENTITY_EVENT_HIDE);
	}
	if (pUnHideMethod = pClass->FindMethodWithDescInInheritedClasses("OnUnhide()", pEntityComponentClass))
	{
		eventMask |= BIT64(ENTITY_EVENT_UNHIDE);
	}
	if (pClass->FindMethodWithDescInInheritedClasses("OnCollision(CollisionEvent)", pEntityComponentClass))
	{
		pCollisionMethod = pClass->FindMethodInInheritedClasses("OnCollisionInternal", 2);
		eventMask |= BIT64(ENTITY_EVENT_COLLISION);
	}
	if (pPrePhysicsUpdateMethod = pClass->FindMethodWithDescInInheritedClasses("OnPrePhysicsUpdate(single)", pEntityComponentClass))
	{
		eventMask |= BIT64(ENTITY_EVENT_PREPHYSICSUPDATE);
	}
	if (pOnGameplayStartMethod = pClass->FindMethodWithDescInInheritedClasses("OnGameplayStart()", pEntityComponentClass))
	{
		eventMask |= BIT64(ENTITY_EVENT_START_GAME);
	}
	if (pOnRemoveMethod = pClass->FindMethodWithDescInInheritedClasses("OnRemove()", pEntityComponentClass))
	{
		eventMask |= BIT64(ENTITY_EVENT_DONE);
	}
}

SManagedEntityComponentFactory::SProperty::SProperty(MonoReflectionPropertyInternal* pReflectionProperty, const char* szName, const char* szLabel, const char* szDesc, EEntityPropertyType serType)
	: pProperty(pReflectionProperty)
	, name(szName)
	, label(szLabel)
	, description(szDesc)
	, serializationType(serType)
{
	MonoInternals::MonoMethod* pGetMethod = MonoInternals::mono_property_get_get_method(pReflectionProperty->property);
	MonoInternals::MonoMethodSignature* pGetMethodSignature = MonoInternals::mono_method_get_signature(pGetMethod, MonoInternals::mono_class_get_image(pReflectionProperty->klass), MonoInternals::mono_method_get_token(pGetMethod));

	MonoInternals::MonoType* pPropertyType = MonoInternals::mono_signature_get_return_type(pGetMethodSignature);
	type = MonoInternals::mono_type_get_type(pPropertyType);
}

ICryUnknownPtr SManagedEntityComponentFactory::CreateClassInstance() const
{
	return crycomponent_cast<ICryUnknownPtr>(std::make_shared<CManagedEntityComponent>(*this));
}

CManagedEntityComponent::CManagedEntityComponent(const SManagedEntityComponentFactory& factory)
	: m_factory(factory)
{
	m_propertyLabel = factory.pClass->GetName();
	m_propertyLabel.append(" Properties");
}

void CManagedEntityComponent::Initialize()
{
	m_pMonoObject = m_factory.pClass->CreateUninitializedInstance();

	EntityId id = GetEntity()->GetId();

	void* pParams[2];
	pParams[0] = &m_pEntity;
	pParams[1] = &id;

	m_factory.pInitializeMethod->Invoke(m_pMonoObject.get(), pParams);
	m_factory.pConstructorMethod->Invoke(m_pMonoObject.get());

	if (m_factory.pOnGameplayStartMethod != nullptr && !gEnv->IsEditing() && gEnv->pGameFramework->IsGameStarted())
	{
		m_factory.pOnGameplayStartMethod->Invoke(m_pMonoObject.get());
	}
}

void CManagedEntityComponent::ProcessEvent(SEntityEvent &event)
{
	switch (event.event)
	{
		case ENTITY_EVENT_START_GAME:
			{
				m_factory.pOnGameplayStartMethod->Invoke(m_pMonoObject.get());
			}
			break;
		case ENTITY_EVENT_XFORM:
			{
				m_factory.pTransformChangedMethod->Invoke(m_pMonoObject.get());
			}
			break;
		case ENTITY_EVENT_UPDATE:
			{
				void* pParams[1];
				pParams[0] = &((SEntityUpdateContext*)event.nParam[0])->fFrameTime;

				if (gEnv->IsEditing())
				{
					if (m_factory.pUpdateEditingMethod != nullptr)
					{
						m_factory.pUpdateEditingMethod->Invoke(m_pMonoObject.get(), pParams);
					}
				}
				else
				{
					if (m_factory.pUpdateGameplayMethod != nullptr)
					{
						m_factory.pUpdateGameplayMethod->Invoke(m_pMonoObject.get(), pParams);
					}
				}
			}
			break;
		case ENTITY_EVENT_RESET:
			{
				void* pParams[1];
				pParams[0] = &event.nParam[0];

				m_factory.pGameModeChangeMethod->Invoke(m_pMonoObject.get(), pParams);
			}
			break;
		case ENTITY_EVENT_HIDE:
			{
				m_factory.pHideMethod->Invoke(m_pMonoObject.get());
			}
			break;
		case ENTITY_EVENT_UNHIDE:
			{
				m_factory.pUnHideMethod->Invoke(m_pMonoObject.get());
			}
			break;
		case ENTITY_EVENT_COLLISION:
			{
				EventPhysCollision* pCollision = (EventPhysCollision*)event.nParam[0];

				void* pParams[2];
				pParams[0] = &pCollision->pEntity[0];
				pParams[1] = &pCollision->pEntity[1];

				m_factory.pCollisionMethod->Invoke(m_pMonoObject.get(), pParams);
			}
			break;
		case ENTITY_EVENT_PREPHYSICSUPDATE:
			{
				void* pParams[1];
				pParams[0] = &event.fParam[0];

				m_factory.pPrePhysicsUpdateMethod->Invoke(m_pMonoObject.get(), pParams);
			}
			break;
		case ENTITY_EVENT_DONE:
			{
				m_factory.pOnRemoveMethod->Invoke(m_pMonoObject.get());
			}
			break;
	}
}

template <typename T>
void SerializePrimitive(Serialization::IArchive& archive, const SManagedEntityComponentFactory::SProperty& entityProperty, void* pObject)
{
	T value;
	if (archive.isOutput())
	{
		// TODO: Wrap into IMonoProperty and handle exceptions
		MonoInternals::MonoObject *pValue = MonoInternals::mono_property_get_value(entityProperty.pProperty->property, pObject, nullptr, nullptr);

		value = *(T*)MonoInternals::mono_object_unbox(pValue);
	}

	archive(value, entityProperty.name, entityProperty.label);

	if (archive.isInput())
	{
		void* pParams[1];
		pParams[0] = &value;

		MonoInternals::mono_property_set_value(entityProperty.pProperty->property, pObject, pParams, nullptr);
	}
}

void CManagedEntityComponent::SerializeProperties(Serialization::IArchive& archive)
{
	for(const SManagedEntityComponentFactory::SProperty& entityProperty : m_factory.properties)
	{
		switch (entityProperty.type)
		{
			case MonoInternals::MONO_TYPE_BOOLEAN:
				{
					SerializePrimitive<bool>(archive, entityProperty, m_pMonoObject->GetManagedObject());
				}
				break;
			case MonoInternals::MONO_TYPE_STRING:
				{
					CMonoDomain* pDomain = static_cast<CMonoDomain*>(m_pMonoObject->GetClass()->GetAssembly()->GetDomain());
					string value;

					if (archive.isOutput())
					{
						// TODO: Wrap into IMonoProperty and handle exceptions
						MonoInternals::MonoString* pPropertyValue = (MonoInternals::MonoString*)MonoInternals::mono_property_get_value(entityProperty.pProperty->property, m_pMonoObject->GetManagedObject(), nullptr, nullptr);

						std::shared_ptr<CMonoString> pValue = std::static_pointer_cast<CMonoString>(pDomain->CreateString(pPropertyValue));
						value = pValue->GetString();
					}

					switch (entityProperty.serializationType)
					{
						case EEntityPropertyType::Primitive:
							archive(value, entityProperty.name, entityProperty.label);
							break;
						case EEntityPropertyType::Geometry:
							archive(Serialization::ModelFilename(value), entityProperty.name, entityProperty.label);
							break;
						case EEntityPropertyType::Texture:
							archive(Serialization::TextureFilename(value), entityProperty.name, entityProperty.label);
							break;
						case EEntityPropertyType::Particle:
							archive(Serialization::ParticleName(value), entityProperty.name, entityProperty.label);
							break;
						case EEntityPropertyType::AnyFile:
							archive(Serialization::GeneralFilename(value), entityProperty.name, entityProperty.label);
							break;
						case EEntityPropertyType::Sound:
							archive(Serialization::SoundFilename(value), entityProperty.name, entityProperty.label);
							break;
						case EEntityPropertyType::Material:
							archive(Serialization::MaterialPicker(value), entityProperty.name, entityProperty.label);
							break;
						case EEntityPropertyType::Animation:
							archive(Serialization::CharacterAnimationPicker(value), entityProperty.name, entityProperty.label);
							break;
					}

					if (archive.isInput())
					{
						std::shared_ptr<CMonoString> pValue = std::static_pointer_cast<CMonoString>(pDomain->CreateString(value));

						void* pParams[1];
						pParams[0] = pValue->GetManagedObject();

						MonoInternals::mono_property_set_value(entityProperty.pProperty->property, m_pMonoObject->GetManagedObject(), pParams, nullptr);
					}
				}
				break;
			case MonoInternals::MONO_TYPE_U1:
			case MonoInternals::MONO_TYPE_CHAR: // Char is unsigned by default for .NET
				{
					SerializePrimitive<uchar>(archive, entityProperty, m_pMonoObject->GetManagedObject());
				}
				break;
			case MonoInternals::MONO_TYPE_I1:
				{
					SerializePrimitive<char>(archive, entityProperty, m_pMonoObject->GetManagedObject());
				}
				break;
			case MonoInternals::MONO_TYPE_I2:
				{
					SerializePrimitive<int16>(archive, entityProperty, m_pMonoObject->GetManagedObject());
				}
				break;
			case MonoInternals::MONO_TYPE_U2:
				{
					SerializePrimitive<uint16>(archive, entityProperty, m_pMonoObject->GetManagedObject());
				}
				break;
			case MonoInternals::MONO_TYPE_I4:
				{
					SerializePrimitive<int32>(archive, entityProperty, m_pMonoObject->GetManagedObject());
				}
				break;
			case MonoInternals::MONO_TYPE_U4:
				{
					SerializePrimitive<uint32>(archive, entityProperty, m_pMonoObject->GetManagedObject());
				}
				break;
			case MonoInternals::MONO_TYPE_I8:
				{
					SerializePrimitive<int64>(archive, entityProperty, m_pMonoObject->GetManagedObject());
				}
				break;
			case MonoInternals::MONO_TYPE_U8:
				{
					SerializePrimitive<uint64>(archive, entityProperty, m_pMonoObject->GetManagedObject());
				}
				break;
			case MonoInternals::MONO_TYPE_R4:
				{
					SerializePrimitive<float>(archive, entityProperty, m_pMonoObject->GetManagedObject());
				}
				break;
			case MonoInternals::MONO_TYPE_R8:
				{
					SerializePrimitive<double>(archive, entityProperty, m_pMonoObject->GetManagedObject());
				}
				break;
		}

		archive.doc(entityProperty.description);
	}
}