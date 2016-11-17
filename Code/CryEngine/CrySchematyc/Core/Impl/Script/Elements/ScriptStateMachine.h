// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Script/Elements/IScriptStateMachine.h>
#include <Schematyc/SerializationUtils/MultiPassSerializer.h>

#include "Script/ScriptElementBase.h"

namespace Schematyc
{
class CScriptStateMachine : public CScriptElementBase<IScriptStateMachine>, public CMultiPassSerializer
{
public:

	CScriptStateMachine();
	CScriptStateMachine(const SGUID& guid, const char* szName, EScriptStateMachineLifetime lifetime, const SGUID& contextGUID, const SGUID& partnerGUID);

	// IScriptElement
	virtual EScriptElementAccessor GetAccessor() const override;
	virtual void                   EnumerateDependencies(const ScriptDependencyEnumerator& enumerator, EScriptDependencyType type) const override;
	virtual void                   RemapDependencies(IGUIDRemapper& guidRemapper) override;
	virtual void                   ProcessEvent(const SScriptEvent& event) override;
	virtual void                   Serialize(Serialization::IArchive& archive) override;
	// ~IScriptElement

	// IScriptStateMachine
	virtual EScriptStateMachineLifetime GetLifetime() const override;
	virtual SGUID                       GetContextGUID() const override;
	virtual SGUID                       GetPartnerGUID() const override;
	// ~IScriptStateMachine

protected:

	// CMultiPassSerializer
	virtual void Load(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void Save(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void Edit(Serialization::IArchive& archive, const ISerializationContext& context) override;
	// ~CMultiPassSerializer

private:

	void CreateTransitionGraph();
	void RefreshTransitionGraph();

private:

	EScriptStateMachineLifetime m_lifetime;
	SGUID                       m_contextGUID;
	SGUID                       m_partnerGUID;
	SGUID                       m_transitionGraphGUID;
};
} // Schematyc
