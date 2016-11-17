// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Script/Elements/IScriptInterfaceImpl.h>
#include <Schematyc/SerializationUtils/MultiPassSerializer.h>

#include "Script/ScriptElementBase.h"

namespace Schematyc
{
// Forward declare interfaces.
struct IEnvInterface;

class CScriptInterfaceImpl : public CScriptElementBase<IScriptInterfaceImpl>, public CMultiPassSerializer
{
public:

	CScriptInterfaceImpl();
	CScriptInterfaceImpl(const SGUID& guid, EDomain domain, const SGUID& refGUID);

	// IScriptElement
	virtual EScriptElementAccessor GetAccessor() const override;
	virtual void                   EnumerateDependencies(const ScriptDependencyEnumerator& enumerator, EScriptDependencyType type) const override;
	virtual void                   RemapDependencies(IGUIDRemapper& guidRemapper) override;
	virtual void                   ProcessEvent(const SScriptEvent& event) override;
	virtual void                   Serialize(Serialization::IArchive& archive) override;
	// ~IScriptElement

	// IScriptInterfaceImpl
	virtual EDomain GetDomain() const override;
	virtual SGUID   GetRefGUID() const override;
	// ~IScriptInterfaceImpl

protected:

	// CMultiPassSerializer
	virtual void LoadDependencies(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void Save(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void Validate(Serialization::IArchive& archive, const ISerializationContext& context) override;
	// ~CMultiPassSerializer

private:

	void RefreshEnvInterfaceFunctions(const IEnvInterface& envInterface);
	//void RefreshScriptInterfaceFunctions(const IScriptFile& interfaceFile);
	//void RefreshScriptInterfaceTasks(const IScriptFile& iInterfaceFile);
	//void RefreshScriptInterfaceTaskPropertiess(const IScriptFile& interfaceFile, const SGUID& taskGUID);

private:

	// #SchematycTODO : Replace m_domain and m_refGUID with SElementId!!!
	EDomain m_domain;
	SGUID   m_refGUID;
};
} // Schematyc
