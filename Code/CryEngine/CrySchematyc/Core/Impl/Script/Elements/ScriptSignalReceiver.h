// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Script/Elements/IScriptSignalReceiver.h>
#include <Schematyc/SerializationUtils/MultiPassSerializer.h>
#include <Schematyc/Utils/GUID.h>

#include "Script/ScriptElementBase.h"
#include "Script/ScriptUserDocumentation.h"

namespace Schematyc
{
class CScriptSignalReceiver : public CScriptElementBase<IScriptSignalReceiver>, public CMultiPassSerializer
{
public:

	CScriptSignalReceiver();
	CScriptSignalReceiver(const SGUID& guid, const char* szName, EScriptSignalReceiverType type, const SGUID& signalGUID);

	// IScriptElement
	virtual EScriptElementAccessor GetAccessor() const override;
	virtual void                   EnumerateDependencies(const ScriptDependencyEnumerator& enumerator, EScriptDependencyType type) const override;
	virtual void                   RemapDependencies(IGUIDRemapper& guidRemapper) override;
	virtual void                   ProcessEvent(const SScriptEvent& event) override;
	virtual void                   Serialize(Serialization::IArchive& archive) override;
	// ~IScriptElement

	// IScriptSignalReceiver
	virtual EScriptSignalReceiverType GetType() const override;
	virtual SGUID                     GetSignalGUID() const override;
	// ~IScriptSignalReceiver

protected:

	// CMultiPassSerializer
	virtual void LoadDependencies(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void Load(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void Save(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void Edit(Serialization::IArchive& archive, const ISerializationContext& context) override;
	virtual void Validate(Serialization::IArchive& archive, const ISerializationContext& context) override;
	// ~CMultiPassSerializer

private:

	void CreateGraph();
	void GoToSignal();

private:

	EScriptSignalReceiverType m_type;
	SGUID                     m_signalGUID;
	SScriptUserDocumentation  m_userDocumentation;
};
} // Schematyc
