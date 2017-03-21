// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
// Means to serialize and edit CryExtension pointers.

#include <CryExtension/ICryFactory.h>
#include <CrySerialization/ClassFactory.h>

namespace Serialization
{
//! Allows us to have std::shared_ptr<TPointer> but serialize it by
//! interface-casting to TSerializable, i.e. implementing Serialization through
//! separate interface.
template<class TPointer, class TSerializable>
struct CryExtensionPointer : public Serialization::IPointer
{
	//! Provides Serialization::IClassFactory interface for classes registered with CryExtension to IArchive.
	//! TSerializable can be used to expose Serialize method through a separate interface, rathern than TBase.
	//! Safe to missing as QueryInterface is used to check its presence.
	class CFactory : public Serialization::IClassFactory
	{
	public:
		CFactory();

		size_t size() const override
		{
			return m_types.size();
		}

		const Serialization::TypeDescription* descriptionByIndex(int index) const override
		{
			if (size_t(index) >= m_types.size())
				return 0;
			return &m_types[index];
		}

		const Serialization::TypeDescription* descriptionByRegisteredName(const char* registeredName) const override
		{
			size_t count = m_types.size();
			for (size_t i = 0; i < m_types.size(); ++i)
				if (strcmp(m_types[i].name(), registeredName) == 0)
					return &m_types[i];
			return 0;
		}

		const char* findAnnotation(const char* typeName, const char* name) const override { return ""; }

		void        serializeNewByIndex(IArchive& ar, int index, const char* name, const char* label) override
		{
			if (size_t(index) >= m_types.size())
				return;
			std::shared_ptr<TPointer> ptr(create(m_types[index].name()));
			if (TSerializable* ser = cryinterface_cast<TSerializable>(ptr.get()))
			{
				ar(*ser, name, label);
			}
		}

		const char* getRegisteredTypeName(const std::shared_ptr<TPointer>& ptr) const;

		std::shared_ptr<TPointer> create(const char* registeredName);

		static CFactory& the()
		{
			static CFactory instance;
			return instance;
		}

	private:
		std::vector<Serialization::TypeDescription> m_types;
		std::vector<string>                         m_labels;
		std::vector<ICryFactory*>                   m_factories;
		std::vector<CryInterfaceID>                 m_classIds;
	};

	CryExtensionPointer(std::shared_ptr<TPointer>& ptr) : m_ptr(ptr) {}

	// Serialization::IPointer
	void create(const char* registeredTypeName) const override;

	Serialization::TypeID          baseType() const override { return Serialization::TypeID::get<TPointer>(); }
	virtual Serialization::SStruct serializer() const override;
	void*                                       get() const override { return reinterpret_cast<void*>(m_ptr.get()); }
	const void*                                 handle() const override { return &m_ptr; }
	Serialization::TypeID                       pointerType() const override { return Serialization::TypeID::get<std::shared_ptr<TPointer>>(); }
	const char* registeredTypeName() const override;
	virtual CFactory* factory() const override { return &CFactory::the(); }
	// ~Serialization::IPointer

	std::shared_ptr<TPointer>& m_ptr;
};

}

#include "CryExtensionImpl.h"