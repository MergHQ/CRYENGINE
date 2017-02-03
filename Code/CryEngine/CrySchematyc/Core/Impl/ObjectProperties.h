// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/FundamentalTypes.h>
#include <Schematyc/IObjectProperties.h>
#include <Schematyc/Runtime/IRuntimeClass.h>
#include <Schematyc/Services/ITimerSystem.h>
#include <Schematyc/Utils/ClassProperties.h>
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

	struct SComponent
	{
		SComponent();
		SComponent(const char* _szName, const CClassProperties& _properties, EOverridePolicy _overridePolicy);
		SComponent(const SComponent& rhs);

		void Serialize(Serialization::IArchive& archive);

		void Edit();
		void Revert();

		string           name;
		CClassProperties properties;
		EOverridePolicy  overridePolicy = EOverridePolicy::Default;
	};

	typedef std::map<SGUID, SComponent>                                        Components;
	typedef std::map<const char*, SComponent&, stl::less_stricmp<const char*>> ComponentsByName;

	struct SVariable
	{
		SVariable();
		SVariable(const char* _szName, const CAnyValuePtr& _pValue, EOverridePolicy _overridePolicy);
		SVariable(const SVariable& rhs);

		void Serialize(Serialization::IArchive& archive);

		void Edit();
		void Revert();

		string          name;
		CAnyValuePtr    pValue;
		EOverridePolicy overridePolicy = EOverridePolicy::Default;
	};

	typedef std::map<SGUID, SVariable>                                        Variables;
	typedef std::map<const char*, SVariable&, stl::less_stricmp<const char*>> VariablesByName;

public:

	CObjectProperties();
	CObjectProperties(const CObjectProperties& rhs);

	// IObjectProperties
	virtual IObjectPropertiesPtr    Clone() const override;
	virtual void                    Serialize(Serialization::IArchive& archive) override;
	virtual const CClassProperties* GetComponentProperties(const SGUID& guid) const override;
	virtual bool                    ReadVariable(const CAnyRef& value, const SGUID& guid) const override;
	// ~IObjectProperties

	void AddComponent(const SGUID& guid, const char* szName, const CClassProperties& properties);
	void AddVariable(const SGUID& guid, const char* szName, const CAnyConstRef& value);

private:

	Components m_components;
	Variables  m_variables;
};

} // Schematyc
