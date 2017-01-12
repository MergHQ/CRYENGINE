// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Script/Elements/ScriptComponentInstance.h"

#include <CrySerialization/BlackBox.h>
#include <CrySerialization/IArchiveHost.h>
#include <CrySerialization/STL.h>
#include <Schematyc/Env/IEnvRegistry.h>
#include <Schematyc/Env/Elements/IEnvComponent.h>
#include <Schematyc/Env/Elements/IEnvInterface.h>
#include <Schematyc/SerializationUtils/ISerializationContext.h>
#include <Schematyc/SerializationUtils/SerializationUtils.h>
#include <Schematyc/Utils/Any.h>
#include <Schematyc/Utils/Assert.h>
#include <Schematyc/Utils/IGUIDRemapper.h>
#include <Schematyc/Utils/IProperties.h>

#include "CVars.h"
#include "Script/Graph/ScriptGraph.h"
#include "Script/Graph/ScriptGraphNode.h"
#include "Script/Graph/Nodes/ScriptGraphBeginNode.h"
#include "SerializationUtils/SerializationContext.h"

SERIALIZATION_ENUM_BEGIN_NESTED(Schematyc, EScriptComponentInstanceFlags, "Schematyc Script Component Instance Flags")
SERIALIZATION_ENUM(Schematyc::EScriptComponentInstanceFlags::EnvClass, "envClass", "Environment Class")
SERIALIZATION_ENUM_END()

namespace Schematyc
{
CScriptComponentInstance::CScriptComponentInstance()
	: CScriptElementBase(EScriptElementFlags::None)
{}

CScriptComponentInstance::CScriptComponentInstance(const SGUID& guid, const char* szName, const SGUID& typeGUID)
	: CScriptElementBase(guid, szName, EScriptElementFlags::None)
	, m_typeGUID(typeGUID)
{
	ApplyComponent();   // #SchematycTODO : Do this on EScriptEventId::Add?
}

EScriptElementAccessor CScriptComponentInstance::GetAccessor() const
{
	return m_accessor;
}

void CScriptComponentInstance::EnumerateDependencies(const ScriptDependencyEnumerator& enumerator, EScriptDependencyType type) const {}

void CScriptComponentInstance::RemapDependencies(IGUIDRemapper& guidRemapper)
{
	CScriptElementBase::RemapDependencies(guidRemapper);
}

void CScriptComponentInstance::ProcessEvent(const SScriptEvent& event)
{
	CScriptElementBase::ProcessEvent(event);
}

void CScriptComponentInstance::Serialize(Serialization::IArchive& archive)
{
	// #SchematycTODO : Shouldn't this be handled by CScriptElementBase itself?
	CScriptElementBase::Serialize(archive);
	CMultiPassSerializer::Serialize(archive);
	CScriptElementBase::SerializeExtensions(archive);
}

SGUID CScriptComponentInstance::GetTypeGUID() const
{
	return m_typeGUID;
}

ScriptComponentInstanceFlags CScriptComponentInstance::GetComponentInstanceFlags() const
{
	return m_flags;
}

bool CScriptComponentInstance::HasTransform() const
{
	return m_bHasTransform;
}

void CScriptComponentInstance::SetTransform(const CTransform& transform)
{
	m_transform = transform;
}

const CTransform& CScriptComponentInstance::GetTransform() const
{
	return m_transform;
}

const IProperties* CScriptComponentInstance::GetProperties() const
{
	return m_pProperties.get();
}

void CScriptComponentInstance::LoadDependencies(Serialization::IArchive& archive, const ISerializationContext& context)
{
	archive(m_typeGUID, "typeGUID");
}

void CScriptComponentInstance::Load(Serialization::IArchive& archive, const ISerializationContext& context)
{
	ApplyComponent();
	archive(m_accessor, "accessor");
	archive(m_transform, "transform");
	if (m_pProperties)
	{
		archive(*m_pProperties, "properties");
	}
}

void CScriptComponentInstance::Save(Serialization::IArchive& archive, const ISerializationContext& context)
{
	archive(m_accessor, "accessor");
	archive(m_typeGUID, "typeGUID");
	archive(m_transform, "transform");
	if (m_pProperties)
	{
		archive(*m_pProperties, "properties");
	}
}

void CScriptComponentInstance::Edit(Serialization::IArchive& archive, const ISerializationContext& context)
{
	if (m_pProperties)
	{
		bool bPublic = m_accessor == EScriptElementAccessor::Public;
		archive(bPublic, "bPublic", "Public");
		if (archive.isInput())
		{
			m_accessor = bPublic ? EScriptElementAccessor::Public : EScriptElementAccessor::Private;
		}
	}
	if (m_bHasTransform)
	{
		archive(m_transform, "transform", "Transform");
	}
	if (m_pProperties)
	{
		archive(*m_pProperties, "properties", "Properties");
	}
}

void CScriptComponentInstance::Validate(Serialization::IArchive& archive, const ISerializationContext& context)
{
	const IEnvComponent* pEnvComponent = gEnv->pSchematyc->GetEnvRegistry().GetComponent(m_typeGUID);
	if (!pEnvComponent)
	{
		CStackString guid;
		GUID::ToString(guid, m_typeGUID);

		CStackString message;
		message.Format("Unable to retrieve environment component: guid = %s", guid.c_str());

		archive.error(*this, message.c_str());
	}
}

void CScriptComponentInstance::ApplyComponent()
{
	const IEnvComponent* pEnvComponent = gEnv->pSchematyc->GetEnvRegistry().GetComponent(m_typeGUID);
	if (pEnvComponent)
	{
		m_bHasTransform = pEnvComponent->GetComponentFlags().Check(EEnvComponentFlags::Transform);

		const IProperties* pEnvComponentProperties = pEnvComponent->GetProperties();
		if (pEnvComponentProperties)
		{
			m_pProperties = pEnvComponentProperties->Clone();
		}
	}
}
} // Schematyc
