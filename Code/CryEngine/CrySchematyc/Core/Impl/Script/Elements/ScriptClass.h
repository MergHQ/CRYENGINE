// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Env/Elements/IEnvClass.h>
#include <Schematyc/SerializationUtils/MultiPassSerializer.h>
#include <Schematyc/Script/Elements/IScriptClass.h>

#include "Script/ScriptElementBase.h"
#include "Script/ScriptUserDocumentation.h"

namespace Schematyc
{
class CScriptClass : public CScriptElementBase<IScriptClass>, public CMultiPassSerializer
{
public:

	CScriptClass();
	CScriptClass(const SGUID& guid, const char* szName);

	// IScriptElement
	virtual EScriptElementAccessor GetAccessor() const override;
	virtual void                   EnumerateDependencies(const ScriptDependencyEnumerator& enumerator, EScriptDependencyType type) const override;
	virtual void                   RemapDependencies(IGUIDRemapper& guidRemapper) override;
	virtual void                   ProcessEvent(const SScriptEvent& event) override;
	virtual void                   Serialize(Serialization::IArchive& archive) override;
	// ~IScriptElement

	// IScriptClass
	virtual const char* GetAuthor() const override;
	virtual const char* GetDescription() const override;
	// ~IScriptClass

protected:

	// CMultiPassSerializer
	virtual void Load(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void Save(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void Edit(Serialization::IArchive& archive, const ISerializationContext& context) override;
	// ~CMultiPassSerializer

private:

	SScriptUserDocumentation m_userDocumentation;
};
}
