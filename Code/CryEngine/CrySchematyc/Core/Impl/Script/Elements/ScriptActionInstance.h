// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Script/Elements/IScriptActionInstance.h>
#include <Schematyc/SerializationUtils/MultiPassSerializer.h>
#include <Schematyc/Utils/ClassProperties.h>

#include "Script/ScriptElementBase.h"

namespace Schematyc
{

class CScriptActionInstance : public CScriptElementBase<IScriptActionInstance>, public CMultiPassSerializer
{
public:

	CScriptActionInstance();
	CScriptActionInstance(const SGUID& guid, const char* szName, const SGUID& actionTypeGUID, const SGUID& componentInstanceGUID);

	// IScriptElement
	virtual void EnumerateDependencies(const ScriptDependencyEnumerator& enumerator, EScriptDependencyType type) const override;
	virtual void RemapDependencies(IGUIDRemapper& guidRemapper) override;
	virtual void ProcessEvent(const SScriptEvent& event) override;
	virtual void Serialize(Serialization::IArchive& archive) override;
	// ~IScriptElement

	// IScriptActionInstance
	virtual SGUID                   GetActionTypeGUID() const override;
	virtual SGUID                   GetComponentInstanceGUID() const override;
	virtual const CClassProperties& GetProperties() const override;
	// ~IScriptActionInstance

protected:

	// CMultiPassSerializer
	virtual void LoadDependencies(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void Load(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void Save(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void Edit(Serialization::IArchive& archive, const ISerializationContext& context) override;
	// ~CMultiPassSerializer

private:

	void RefreshProperties();
	void SerializeProperties(Serialization::IArchive& archive);

private:

	SGUID            m_actionTypeGUID;
	SGUID            m_componentInstanceGUID;
	CClassProperties m_properties;
};

} // Schematyc
