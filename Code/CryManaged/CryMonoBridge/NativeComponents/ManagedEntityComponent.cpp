#include "StdAfx.h"
#include "ManagedEntityComponent.h"

#include <CryMono/IMonoRuntime.h>
#include <CryMono/IMonoAssembly.h>
#include <CryMono/IMonoClass.h>
#include <CryMono/IMonoDomain.h>

#include <CryPhysics/physinterface.h>

#include <CrySerialization/Decorators/Resources.h>

CManagedEntityComponentFactory::CManagedEntityComponentFactory(std::shared_ptr<IMonoClass> pClass, CryInterfaceID id)
	: m_pClass(pClass)
	, m_id(id)
	, m_eventMask(0)
{
	IMonoClass* pEntityComponentClass = gEnv->pMonoRuntime->GetCryCoreLibrary()->GetClass("CryEngine", "EntityComponent");

	if (m_pClass->IsMethodImplemented(pEntityComponentClass, "OnTransformChanged()"))
	{
		m_eventMask |= BIT64(ENTITY_EVENT_XFORM);
	}
	if (m_pClass->IsMethodImplemented(pEntityComponentClass, "OnUpdate(single)"))
	{
		m_eventMask |= BIT64(ENTITY_EVENT_UPDATE);
	}
	if (m_pClass->IsMethodImplemented(pEntityComponentClass, "OnEditorGameModeChange(bool)"))
	{
		m_eventMask |= BIT64(ENTITY_EVENT_RESET);
	}
	if (m_pClass->IsMethodImplemented(pEntityComponentClass, "OnHide()"))
	{
		m_eventMask |= BIT64(ENTITY_EVENT_HIDE);
	}
	if (m_pClass->IsMethodImplemented(pEntityComponentClass, "OnUnhide()"))
	{
		m_eventMask |= BIT64(ENTITY_EVENT_UNHIDE);
	}
	if (m_pClass->IsMethodImplemented(pEntityComponentClass, "OnCollision(System.IntPtr, System.IntPtr)"))
	{
		m_eventMask |= BIT64(ENTITY_EVENT_COLLISION);
	}
	if (m_pClass->IsMethodImplemented(pEntityComponentClass, "OnPrePhysicsUpdate(single)"))
	{
		m_eventMask |= BIT64(ENTITY_EVENT_PREPHYSICSUPDATE);
	}
}

CManagedEntityComponentFactory::SProperty::SProperty(MonoReflectionPropertyInternal* pReflectionProperty, const char* szName, const char* szLabel, const char* szDesc, EEntityPropertyType serType)
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

CManagedEntityComponent::CManagedEntityComponent(const CManagedEntityComponentFactory& factory)
	: m_factory(factory)
	, m_eventMask(factory.GetEventMask())
{
	m_propertyLabel = factory.GetClass()->GetName();
	m_propertyLabel.append(" Properties");
}

void CManagedEntityComponent::Initialize()
{
	m_pMonoObject = m_factory.GetClass()->CreateUninitializedInstance();

	EntityId id = GetEntity()->GetId();

	void* pParams[2];
	pParams[0] = &m_pEntity;
	pParams[1] = &id;

	IMonoClass* pEntityComponentClass = gEnv->pMonoRuntime->GetCryCoreLibrary()->GetClass("CryEngine", "EntityComponent");

	pEntityComponentClass->InvokeMethod("Initialize", m_pMonoObject.get(), pParams, 2);

	m_pMonoObject->InvokeMethod(".ctor");
}

void CManagedEntityComponent::ProcessEvent(SEntityEvent &event)
{
	switch (event.event)
	{
		case ENTITY_EVENT_XFORM:
			{
				m_pMonoObject->InvokeMethod("OnTransformChanged");
			}
			break;
		case ENTITY_EVENT_UPDATE:
			{
				void* pParams[1];
				pParams[0] = &((SEntityUpdateContext*)event.nParam[0])->fFrameTime;

				m_pMonoObject->InvokeMethod("OnUpdate", pParams, 1);
			}
			break;
		case ENTITY_EVENT_RESET:
			{
				void* pParams[1];
				pParams[0] = &event.nParam[0];

				m_pMonoObject->InvokeMethod("OnEditorGameModeChange", pParams, 1);
			}
			break;
		case ENTITY_EVENT_HIDE:
			{
				m_pMonoObject->InvokeMethod("OnHide");
			}
			break;
		case ENTITY_EVENT_UNHIDE:
			{
				m_pMonoObject->InvokeMethod("OnUnhide");
			}
			break;
		case ENTITY_EVENT_COLLISION:
			{
				EventPhysCollision* pCollision = (EventPhysCollision*)event.nParam[0];
				
				void* pParams[2];
				pParams[0] = pCollision->pEntity[0];
				pParams[1] = pCollision->pEntity[1];

				m_pMonoObject->InvokeMethod("OnCollision", pParams, 2);
			}
			break;
		case ENTITY_EVENT_PREPHYSICSUPDATE:
			{
				void* pParams[1];
				pParams[0] = &event.fParam[0];

				m_pMonoObject->InvokeMethod("OnPrePhysicsUpdate", pParams, 1);
			}
			break;
	}
}

template <typename T>
void SerializePrimitive(Serialization::IArchive& archive, const CManagedEntityComponentFactory::SProperty& property, void* pObject)
{
	T value;
	if (archive.isOutput())
	{
		// TODO: Wrap into IMonoProperty and handle exceptions
		MonoObject *pValue = mono_property_get_value(property.pProperty->property, pObject, nullptr, nullptr);

		value = *(T*)mono_object_unbox(pValue);
	}

	archive(value, property.name, property.label);

	if (archive.isInput())
	{
		void* pParams[1];
		pParams[0] = &value;

		mono_property_set_value(property.pProperty->property, pObject, pParams, nullptr);
	}
}

void CManagedEntityComponent::SerializeProperties(Serialization::IArchive& archive)
{
	for (auto it = m_factory.GetProperties().begin(); it != m_factory.GetProperties().end(); ++it)
	{
		switch (it->type)
		{
			case MONO_TYPE_BOOLEAN:
				{
					SerializePrimitive<bool>(archive, *it, m_pMonoObject->GetHandle());
				}
				break;
			case MONO_TYPE_STRING:
				{
					string value;
					if (archive.isOutput())
					{
						// TODO: Wrap into IMonoProperty and handle exceptions
						MonoObject *pValue = mono_property_get_value(it->pProperty->property, m_pMonoObject->GetHandle(), nullptr, nullptr);

						value = mono_string_to_utf8((MonoString*)pValue);
					}

					switch (it->serializationType)
					{
						case EEntityPropertyType::Primitive:
							archive(value, it->name, it->label);
							break;
						case EEntityPropertyType::Object:
							archive(Serialization::ModelFilename(value), it->name, it->label);
							break;
						case EEntityPropertyType::Texture:
							archive(Serialization::TextureFilename(value), it->name, it->label);
							break;
						case EEntityPropertyType::Particle:
							archive(Serialization::ParticleName(value), it->name, it->label);
							break;
						case EEntityPropertyType::AnyFile:
							archive(Serialization::GeneralFilename(value), it->name, it->label);
							break;
						case EEntityPropertyType::Sound:
							archive(Serialization::SoundFilename(value), it->name, it->label);
							break;
						case EEntityPropertyType::Material:
							archive(Serialization::MaterialPicker(value), it->name, it->label);
							break;
						case EEntityPropertyType::Animation:
							archive(Serialization::CharacterAnimationPicker(value), it->name, it->label);
							break;
					}

					if (archive.isInput())
					{
						IMonoDomain* pDomain = m_pMonoObject->GetClass()->GetAssembly()->GetDomain();

						void* pParams[1];
						pParams[0] = pDomain->CreateManagedString(value);

						mono_property_set_value(it->pProperty->property, m_pMonoObject->GetHandle(), pParams, nullptr);
					}
				}
				break;
			case MONO_TYPE_U1:
			case MONO_TYPE_CHAR: // Char is unsigned by default for .NET
				{
					SerializePrimitive<uchar>(archive, *it, m_pMonoObject->GetHandle());
				}
				break;
			case MONO_TYPE_I1:
				{
					SerializePrimitive<char>(archive, *it, m_pMonoObject->GetHandle());
				}
				break;
			case MONO_TYPE_I2:
				{
					SerializePrimitive<int16>(archive, *it, m_pMonoObject->GetHandle());
				}
				break;
			case MONO_TYPE_U2:
				{
					SerializePrimitive<uint16>(archive, *it, m_pMonoObject->GetHandle());
				}
				break;
			case MONO_TYPE_I4:
				{
					SerializePrimitive<int32>(archive, *it, m_pMonoObject->GetHandle());
				}
				break;
			case MONO_TYPE_U4:
				{
					SerializePrimitive<uint32>(archive, *it, m_pMonoObject->GetHandle());
				}
				break;
			case MONO_TYPE_I8:
				{
					SerializePrimitive<int64>(archive, *it, m_pMonoObject->GetHandle());
				}
				break;
			case MONO_TYPE_U8:
				{
					SerializePrimitive<uint64>(archive, *it, m_pMonoObject->GetHandle());
				}
				break;
			case MONO_TYPE_R4:
				{
					SerializePrimitive<float>(archive, *it, m_pMonoObject->GetHandle());
				}
				break;
			case MONO_TYPE_R8:
				{
					SerializePrimitive<double>(archive, *it, m_pMonoObject->GetHandle());
				}
				break;
		}

		archive.doc(it->description);
	}
}