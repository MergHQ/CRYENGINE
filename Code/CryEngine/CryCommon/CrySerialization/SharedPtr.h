// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/Serializer.h>

#include "ClassFactory.h"

template<class T>
class StdSharedPtrSerializer : public Serialization::IPointer
{
public:
	StdSharedPtrSerializer(std::shared_ptr<T>& ptr)
		: m_ptr(ptr)
	{
	}

	const char* registeredTypeName() const override
	{
		if (m_ptr)
			return factoryOverride().getRegisteredTypeName(m_ptr.get());
		else
			return "";
	}

	void create(const char* registeredTypeName) const override
	{
		CRY_ASSERT(!m_ptr || m_ptr.use_count() == 1);
		if (registeredTypeName && registeredTypeName[0] != '\0')
			m_ptr.reset(factoryOverride().create(registeredTypeName));
		else
			m_ptr.reset();
	}

	Serialization::TypeID baseType() const override
	{
		return Serialization::TypeID::get<T>();
	}

	virtual Serialization::SStruct serializer() const override
	{
		return Serialization::SStruct(*m_ptr);
	}

	void* get() const
	{
		return reinterpret_cast<void*>(m_ptr.get());
	}

	const void* handle() const
	{
		return &m_ptr;
	}

	Serialization::TypeID pointerType() const override
	{
		return Serialization::TypeID::get<std::shared_ptr<T>>();
	}

	Serialization::ClassFactory<T>* factory() const override
	{
		return &factoryOverride();
	}

	virtual Serialization::ClassFactory<T>& factoryOverride() const
	{
		return Serialization::ClassFactory<T>::the();
	}

protected:
	std::shared_ptr<T>& m_ptr;
};

namespace std
{
template<class T>
bool Serialize(Serialization::IArchive& ar, std::shared_ptr<T>& ptr, const char* name, const char* label)
{
	StdSharedPtrSerializer<T> serializer(ptr);
	return ar(static_cast<Serialization::IPointer&>(serializer), name, label);
}
}
