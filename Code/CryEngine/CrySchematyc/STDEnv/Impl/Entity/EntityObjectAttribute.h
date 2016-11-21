// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryEntitySystem/IEntity.h>
#include <CryEntitySystem/IEntityClass.h>

namespace Schematyc
{
// Forward declare interfaces.
struct IObjectProperties;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(IObjectProperties)

typedef CSignal<void ()> EntityPropertiesChangeSignal;

class CEntityObjectAttribute : public IEntityAttribute
{
private:

	struct SSignals
	{
		EntityPropertiesChangeSignal change;
	};

public:

	CEntityObjectAttribute(const SGUID& guid, const IObjectProperties& defaultProperties);
	CEntityObjectAttribute(const CEntityObjectAttribute& rhs);

	// IEntityAttribute
	virtual const char*         GetName() const override;
	virtual const char*         GetLabel() const override;
	virtual void                Serialize(Serialization::IArchive& archive) override;
	virtual IEntityAttributePtr Clone() const override;
	// ~IEntityAttribute

	SGUID                                GetGUID() const;
	IObjectPropertiesConstPtr            GetProperties() const;
	EntityPropertiesChangeSignal::Slots& GetChangeSignalSlots();

private:

	void DisplayDetails(Serialization::IArchive& archive);
	void ShowInSchematyc();

public:

	static const char* ms_szAttributeName;

private:

	SGUID                m_guid;
	IObjectPropertiesPtr m_pProperties;
	SSignals             m_signals;
};

DECLARE_SHARED_POINTERS(CEntityObjectAttribute)
} // Schematyc
