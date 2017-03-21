// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/StringList.h>
#include <CryExtension/ICryFactoryRegistry.h>
#include <CryExtension/CryTypeID.h>
#include <CrySystem/ISystem.h>

namespace Serialization 
{
	//! Generate user-friendly class name, e.g. convert "AnimationPoseModifier_FootStore" -> "Foot Store"
	inline string MakePrettyClassName(const char* className)
	{
		const char* firstSep = strchr(className, '_');
		if (!firstSep)
		{
			// name doesn't follow expected convention, return as is
			return className;
		}

		const char* start = firstSep + 1;
		string result;
		result.reserve(strlen(start) + 4);

		const char* p = start;
		while (*p != '\0')
		{
			if (*p >= 'A' && *p <= 'Z' &&
				*(p - 1) >= 'a' && *(p - 1) <= 'z')
			{
				result += ' ';
			}
			if (*p == '_')
				result += ' ';
			else
				result += *p;
			++p;
		}

		return result;
	}

	template<class TPointer, class TSerializable>
	CryExtensionPointer<TPointer, TSerializable>::CFactory::CFactory()
		: IClassFactory(Serialization::TypeID::get<TPointer>())
	{
		setNullLabel("[ None ]");
		ICryFactoryRegistry* factoryRegistry = gEnv->pSystem->GetCryFactoryRegistry();

		size_t factoryCount = 0;
		factoryRegistry->IterateFactories(cryiidof<TPointer>(), 0, factoryCount);
		std::vector<ICryFactory*> factories(factoryCount, nullptr);
		if (factoryCount)
			factoryRegistry->IterateFactories(cryiidof<TPointer>(), &factories[0], factoryCount);

		string sharedPrefix;
		bool hasSharedPrefix = true;
		for (size_t i = 0; i < factoryCount; ++i)
		{
			ICryFactory* factory = factories[i];
			if (factory->ClassSupports(cryiidof<TSerializable>()))
			{
				m_factories.push_back(factory);
				if (hasSharedPrefix)
				{
					// make sure that shared prefix is the same for all the names
					const char* name = factory->GetName();
					const char* lastPrefixCharacter = strchr(name, '_');
					if (lastPrefixCharacter == 0)
						hasSharedPrefix = false;
					else
					{
						if (!sharedPrefix.empty())
						{
							if (strncmp(name, sharedPrefix.c_str(), sharedPrefix.size()) != 0)
								hasSharedPrefix = false;
						}
						else
							sharedPrefix.assign(name, lastPrefixCharacter + 1);
					}
				}
			}
		}

		size_t usableFactoriesCount = m_factories.size();
		m_types.reserve(usableFactoriesCount);
		m_labels.reserve(usableFactoriesCount);

		for (size_t i = 0; i < usableFactoriesCount; ++i)
		{
			ICryFactory* factory = m_factories[i];
			m_classIds.push_back(factory->GetClassID());
			const char* name = factory->GetName();
			m_labels.push_back(MakePrettyClassName(name));
			if (hasSharedPrefix)
				name += sharedPrefix.size();
			m_types.push_back(Serialization::TypeDescription(Serialization::TypeID(), name, m_labels.back().c_str()));
		}
	}

	template<class TPointer, class TSerializable>
	std::shared_ptr<TPointer> CryExtensionPointer<TPointer, TSerializable>::CFactory::create(const char* registeredName)
	{
		size_t count = m_types.size();
		for (size_t i = 0; i < count; ++i)
			if (strcmp(m_types[i].name(), registeredName) == 0)
				return std::static_pointer_cast<TPointer>(m_factories[i]->CreateClassInstance());
		return std::shared_ptr<TPointer>();
	}

	template<class TPointer, class TSerializable>
	const char* CryExtensionPointer<TPointer, TSerializable>::CFactory::getRegisteredTypeName(const std::shared_ptr<TPointer>& ptr) const
	{
		if (!ptr.get())
			return "";
		CryInterfaceID id = std::static_pointer_cast<TPointer>(ptr)->GetFactory()->GetClassID();
		size_t count = m_classIds.size();
		for (size_t i = 0; i < count; ++i)
			if (m_classIds[i] == id)
				return m_types[i].name();
		return "";
	}

	template<class TPointer, class TSerializable>
	const char* CryExtensionPointer<TPointer, TSerializable>::registeredTypeName() const
	{
		if (m_ptr)
			return CFactory::the().getRegisteredTypeName(m_ptr);

		return "";
	}

	template<class TPointer, class TSerializable>
	void CryExtensionPointer<TPointer, TSerializable>::create(const char* registeredTypeName) const
	{
		if (registeredTypeName[0] != '\0')
			m_ptr = CFactory::the().create(registeredTypeName);
		else
			m_ptr.reset();
	}

	template<class TPointer, class TSerializable>
	Serialization::SStruct CryExtensionPointer<TPointer, TSerializable>::serializer() const
	{
		if (TSerializable* ser = cryinterface_cast<TSerializable>(m_ptr.get()))
			return Serialization::SStruct(*ser);

		return Serialization::SStruct();
	}
}