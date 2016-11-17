// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/FundamentalTypes.h>
#include <Schematyc/IObjectProperties.h>
#include <Schematyc/Runtime/IRuntimeClass.h>
#include <Schematyc/Services/ITimerSystem.h>
#include <Schematyc/Utils/GUID.h>
#include <Schematyc/Utils/Scratchpad.h>

namespace Schematyc
{
// Forward declare classes.
class CAnyConstRef;
class CAnyRef;
class CAnyValue;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(CAnyValue)

class CObjectProperties : public IObjectProperties
{
private:

	struct SProperty
	{
		SProperty();
		SProperty(const char* _szName, const CAnyValuePtr& _pValue, EOverridePolicy _overridePolicy);
		SProperty(const SProperty& rhs);

		void Serialize(Serialization::IArchive& archive);

		void Edit();
		void Revert();

		string          name;
		CAnyValuePtr    pValue;
		EOverridePolicy overridePolicy;
	};

	typedef std::map<SGUID, SProperty>                                        Properties;   // #SchematycTODO : Replace with unordered map once YASLI support is provided.
	typedef std::map<const char*, SProperty&, stl::less_stricmp<const char*>> PropertiesByName;

public:

	CObjectProperties();
	CObjectProperties(const CObjectProperties& rhs);

	// IObjectProperties
	virtual IObjectPropertiesPtr Clone() const override;
	virtual void                 Serialize(Serialization::IArchive& archive) override;
	virtual bool                 Read(const CAnyRef& value, const SGUID& guid) const override;
	// ~IObjectProperties

	void Add(const SGUID& guid, const char* szName, const CAnyConstRef& value);

private:

	Properties m_properties;
};
} // Schematyc
