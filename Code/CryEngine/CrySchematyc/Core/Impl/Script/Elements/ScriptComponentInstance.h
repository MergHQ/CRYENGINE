// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Script/Elements/IScriptComponentInstance.h>
#include <Schematyc/SerializationUtils/MultiPassSerializer.h>
#include <Schematyc/Utils/Transform.h>

#include "Script/ScriptElementBase.h"

namespace Schematyc
{
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(IProperties)

class CScriptComponentInstance : public CScriptElementBase<IScriptComponentInstance>, public CMultiPassSerializer
{
public:

	CScriptComponentInstance();
	CScriptComponentInstance(const SGUID& guid, const char* szName, const SGUID& typeGUID);

	// IScriptElement
	virtual EScriptElementAccessor GetAccessor() const override;
	virtual void                   EnumerateDependencies(const ScriptDependencyEnumerator& enumerator, EScriptDependencyType type) const override;
	virtual void                   RemapDependencies(IGUIDRemapper& guidRemapper) override;
	virtual void                   ProcessEvent(const SScriptEvent& event) override;
	virtual void                   Serialize(Serialization::IArchive& archive) override;
	// ~IScriptElement

	// IScriptComponentInstance
	virtual SGUID                        GetTypeGUID() const override;
	virtual ScriptComponentInstanceFlags GetComponentInstanceFlags() const override;
	virtual bool                         HasTransform() const override;
	virtual void                         SetTransform(const CTransform& transform) override;
	virtual const CTransform&  GetTransform() const override;
	virtual const IProperties* GetProperties() const override;
	// ~IScriptComponentInstance

protected:

	// CMultiPassSerializer
	virtual void LoadDependencies(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void Load(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void Save(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void Edit(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void Validate(Serialization::IArchive& archive, const ISerializationContext& context) override;
	// ~CMultiPassSerializer

private:

	void ApplyComponent();

private:

	EScriptElementAccessor       m_accessor = EScriptElementAccessor::Private;
	SGUID                        m_typeGUID;
	ScriptComponentInstanceFlags m_flags;
	bool                         m_bHasTransform = false;
	CTransform                   m_transform;
	IPropertiesPtr               m_pProperties;
};
} // Schematyc
