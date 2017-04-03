#include "StdAfx.h"
#include "ManagedEntityComponent.h"
#include "MonoRuntime.h"

#include <CryMono/IMonoAssembly.h>
#include <CryMono/IMonoClass.h>
#include <CryMono/IMonoDomain.h>

#include <CryPhysics/physinterface.h>

#include <CrySerialization/Decorators/Resources.h>

SManagedEntityComponentFactory::SManagedEntityComponentFactory(std::shared_ptr<IMonoClass> pMonoClass, CryInterfaceID identifier)
	: pClass(pMonoClass)
	, id(identifier)
	, eventMask(0)
{
	IMonoClass* pEntityComponentClass = gEnv->pMonoRuntime->GetCryCoreLibrary()->GetClass("CryEngine", "EntityComponent");

	pConstructorMethod = pMonoClass->FindMethod(".ctor");
	pInitializeMethod = pClass->FindMethodInInheritedClasses("Initialize", 2);

	if (pTransformChangedMethod = pClass->FindMethodWithDescInInheritedClasses("OnTransformChanged()", pEntityComponentClass))
	{
		eventMask |= BIT64(ENTITY_EVENT_XFORM);
	}
	if (pUpdateMethod = pClass->FindMethodWithDescInInheritedClasses("OnUpdate(single)", pEntityComponentClass))
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
	if (pCollisionMethod = pClass->FindMethodWithDescInInheritedClasses("OnCollision(CollisionEvent)", pEntityComponentClass))
	{
		eventMask |= BIT64(ENTITY_EVENT_COLLISION);
	}
	if (pPrePhysicsUpdateMethod = pClass->FindMethodWithDescInInheritedClasses("OnPrePhysicsUpdate(single)", pEntityComponentClass))
	{
		eventMask |= BIT64(ENTITY_EVENT_PREPHYSICSUPDATE);
	}
}

SManagedEntityComponentFactory::SProperty::SProperty(MonoReflectionPropertyInternal* pReflectionProperty, const char* szName, const char* szLabel, const char* szDesc, EEntityPropertyType serType)
	: pProperty(pReflectionProperty)
	, name(szName)
	, label(szLabel)
	, description(szDesc)
	, serializationType(serType)
{
	MonoMethod* pGetMethod = mono_property_get_get_method(pReflectionProperty->property);
	MonoMethodSignature* pGetMethodSignature = mono_method_get_signature(pGetMethod, mono_class_get_image(pReflectionProperty->klass), mono_method_get_token(pGetMethod));

	MonoType* pPropertyType = mono_signature_get_return_type(pGetMethodSignature);
	type = (MonoTypeEnum)mono_type_get_type(pPropertyType);
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
}

void CManagedEntityComponent::ProcessEvent(SEntityEvent &event)
{
	switch (event.event)
	{
		case ENTITY_EVENT_XFORM:
			{
				m_factory.pTransformChangedMethod->Invoke(m_pMonoObject.get());
			}
			break;
		case ENTITY_EVENT_UPDATE:
			{
				void* pParams[1];
				pParams[0] = &((SEntityUpdateContext*)event.nParam[0])->fFrameTime;

				m_factory.pUpdateMethod->Invoke(m_pMonoObject.get(), pParams);
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
	}
}

template <typename T>
void SerializePrimitive(Serialization::IArchive& archive, const SManagedEntityComponentFactory::SProperty& entityProperty, void* pObject)
{
	T value;
	if (archive.isOutput())
	{
		// TODO: Wrap into IMonoProperty and handle exceptions
		MonoObject *pValue = mono_property_get_value(entityProperty.pProperty->property, pObject, nullptr, nullptr);

		value = *(T*)mono_object_unbox(pValue);
	}

	archive(value, entityProperty.name, entityProperty.label);

	if (archive.isInput())
	{
		void* pParams[1];
		pParams[0] = &value;

		mono_property_set_value(entityProperty.pProperty->property, pObject, pParams, nullptr);
	}
}

void CManagedEntityComponent::SerializeProperties(Serialization::IArchive& archive)
{
	for(const SManagedEntityComponentFactory::SProperty& entityProperty : m_factory.properties)
	{
		switch (entityProperty.type)
		{
			case MONO_TYPE_BOOLEAN:
				{
					SerializePrimitive<bool>(archive, entityProperty, m_pMonoObject->GetHandle());
				}
				break;
			case MONO_TYPE_STRING:
				{
					string value;
					if (archive.isOutput())
					{
						// TODO: Wrap into IMonoProperty and handle exceptions
						MonoObject *pValue = mono_property_get_value(entityProperty.pProperty->property, m_pMonoObject->GetHandle(), nullptr, nullptr);

						value = mono_string_to_utf8((MonoString*)pValue);
					}

					switch (entityProperty.serializationType)
					{
						case EEntityPropertyType::Primitive:
							archive(value, entityProperty.name, entityProperty.label);
							break;
						case EEntityPropertyType::Object:
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
						IMonoDomain* pDomain = m_pMonoObject->GetClass()->GetAssembly()->GetDomain();

						void* pParams[1];
						pParams[0] = pDomain->CreateManagedString(value);

						mono_property_set_value(entityProperty.pProperty->property, m_pMonoObject->GetHandle(), pParams, nullptr);
					}
				}
				break;
			case MONO_TYPE_U1:
			case MONO_TYPE_CHAR: // Char is unsigned by default for .NET
				{
					SerializePrimitive<uchar>(archive, entityProperty, m_pMonoObject->GetHandle());
				}
				break;
			case MONO_TYPE_I1:
				{
					SerializePrimitive<char>(archive, entityProperty, m_pMonoObject->GetHandle());
				}
				break;
			case MONO_TYPE_I2:
				{
					SerializePrimitive<int16>(archive, entityProperty, m_pMonoObject->GetHandle());
				}
				break;
			case MONO_TYPE_U2:
				{
					SerializePrimitive<uint16>(archive, entityProperty, m_pMonoObject->GetHandle());
				}
				break;
			case MONO_TYPE_I4:
				{
					SerializePrimitive<int32>(archive, entityProperty, m_pMonoObject->GetHandle());
				}
				break;
			case MONO_TYPE_U4:
				{
					SerializePrimitive<uint32>(archive, entityProperty, m_pMonoObject->GetHandle());
				}
				break;
			case MONO_TYPE_I8:
				{
					SerializePrimitive<int64>(archive, entityProperty, m_pMonoObject->GetHandle());
				}
				break;
			case MONO_TYPE_U8:
				{
					SerializePrimitive<uint64>(archive, entityProperty, m_pMonoObject->GetHandle());
				}
				break;
			case MONO_TYPE_R4:
				{
					SerializePrimitive<float>(archive, entityProperty, m_pMonoObject->GetHandle());
				}
				break;
			case MONO_TYPE_R8:
				{
					SerializePrimitive<double>(archive, entityProperty, m_pMonoObject->GetHandle());
				}
				break;
		}

		archive.doc(entityProperty.description);
	}
}