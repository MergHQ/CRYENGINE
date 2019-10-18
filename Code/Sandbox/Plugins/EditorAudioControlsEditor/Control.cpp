// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Control.h"

#include "AssetsManager.h"
#include "ContextManager.h"
#include "AssetUtils.h"
#include "Context.h"
#include "NameValidator.h"
#include "Common/IConnection.h"
#include "Common/IImpl.h"
#include "Common/IItem.h"

#include <CrySerialization/StringList.h>

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
CControl::~CControl()
{
	m_connections.clear();
}

//////////////////////////////////////////////////////////////////////////
void CControl::SetName(string const& name)
{
	string fixedName = name;
	g_nameValidator.FixupString(fixedName);

	if ((!fixedName.IsEmpty()) && (fixedName != m_name) && ((m_flags& EAssetFlags::IsDefaultControl) == EAssetFlags::None) && g_nameValidator.IsValid(fixedName))
	{
		string const oldName = m_name;

		if (m_type != EAssetType::State)
		{
			m_name = AssetUtils::GenerateUniqueControlName(fixedName, m_type, this);
			m_id = AssetUtils::GenerateUniqueAssetId(m_name, m_type);
		}
		else
		{
			m_name = AssetUtils::GenerateUniqueName(fixedName, m_type, this, m_pParent);
			m_id = AssetUtils::GenerateUniqueStateId(m_pParent->GetName(), m_name);
		}

		if (m_name != oldName)
		{
			SignalControlModified();
			g_assetsManager.OnAssetRenamed(this);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::SetDescription(string const& description)
{
	if (description != m_description)
	{
		m_description = description;
		SignalControlModified();
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::Serialize(Serialization::IArchive& ar)
{
	// Name
	string const name = m_name;

	if ((m_flags& EAssetFlags::IsDefaultControl) != EAssetFlags::None)
	{
		ar(name, "name", "!Name");
	}
	else
	{
		ar(name, "name", "Name");
	}

	ar.doc(name);

	// Description
	string const description = m_description;

	if ((m_flags& EAssetFlags::IsDefaultControl) != EAssetFlags::None)
	{
		ar(description, "description", "!Description");
	}
	else
	{
		ar(description, "description", "Description");
	}

	ar.doc(description);

	// Context
	CryAudio::ContextId contextId = m_contextId;

	if (((m_flags& EAssetFlags::IsDefaultControl) == EAssetFlags::None) && (m_type != EAssetType::State))
	{
		Serialization::StringList contextList;

		for (auto const pContext : g_contexts)
		{
			contextList.emplace_back(pContext->GetName());
		}

		std::sort(contextList.begin(), contextList.end());

		Serialization::StringListValue const selectedContext(contextList, g_contextManager.GetContextName(m_contextId));
		ar(selectedContext, "context", "Context");
		contextId = g_contextManager.GenerateContextId(contextList[selectedContext.index()]);
	}

	// Auto Load
	bool isAutoLoad = m_isAutoLoad;

	if ((m_type == EAssetType::Preload) || (m_type == EAssetType::Setting))
	{
		ar(isAutoLoad, "auto_load", "Auto Load");
	}

	if (ar.isInput())
	{
		SetName(name);
		SetDescription(description);
		SetContextId(contextId);
		SetAutoLoad(isAutoLoad);
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::SetContextId(CryAudio::ContextId const contextId)
{
	if (m_contextId != contextId)
	{
		m_contextId = contextId;
		SignalControlModified();
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::SetAutoLoad(bool const isAutoLoad)
{
	if (isAutoLoad != m_isAutoLoad)
	{
		m_isAutoLoad = isAutoLoad;
		SignalControlModified();
	}
}

//////////////////////////////////////////////////////////////////////////
IConnection* CControl::GetConnectionAt(size_t const index) const
{
	IConnection* pIConnection = nullptr;

	if (index < m_connections.size())
	{
		pIConnection = m_connections[index];
	}

	return pIConnection;
}

//////////////////////////////////////////////////////////////////////////
IConnection* CControl::GetConnection(ControlId const id) const
{
	IConnection* pIConnection = nullptr;

	for (auto const pITempConnection : m_connections)
	{
		if ((pITempConnection != nullptr) && (pITempConnection->GetID() == id))
		{
			pIConnection = pITempConnection;
			break;
		}
	}

	return pIConnection;
}

//////////////////////////////////////////////////////////////////////////
void CControl::AddConnection(IConnection* const pIConnection)
{
	Impl::IItem* const pIItem = g_pIImpl->GetItem(pIConnection->GetID());

	if (pIItem != nullptr)
	{
		g_pIImpl->EnableConnection(pIConnection);
		pIConnection->SignalConnectionChanged.Connect(this, &CControl::SignalConnectionModified);
		m_connections.push_back(pIConnection);
		SignalConnectionAdded();
		SignalControlModified();
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::RemoveConnection(Impl::IItem* const pIItem)
{
	ControlId const id = pIItem->GetId();
	auto iter = m_connections.begin();
	auto const iterEnd = m_connections.cend();

	while (iter != iterEnd)
	{
		auto const pIConnection = *iter;

		if (pIConnection->GetID() == id)
		{
			g_pIImpl->DisableConnection(pIConnection);
			pIConnection->SignalConnectionChanged.DisconnectById(reinterpret_cast<uintptr_t>(this));
			g_pIImpl->DestructConnection(pIConnection);

			m_connections.erase(iter);
			SignalConnectionRemoved();
			SignalControlModified();
			break;
		}

		++iter;
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::ClearConnections()
{
	if (!m_connections.empty())
	{
		for (auto const pIConnection : m_connections)
		{
			g_pIImpl->DisableConnection(pIConnection);
			pIConnection->SignalConnectionChanged.DisconnectById(reinterpret_cast<uintptr_t>(this));
			g_pIImpl->DestructConnection(pIConnection);

			SignalConnectionRemoved();
		}

		m_connections.clear();
		SignalControlModified();
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::BackupAndClearConnections()
{
	// Raw connections are used to temporarily store connections in XML format
	// when middleware data gets reloaded.
	m_rawConnections.clear();

	for (auto const pIConnection : m_connections)
	{
		XmlNodeRef const rawConnection = g_pIImpl->CreateXMLNodeFromConnection(pIConnection, m_type, m_contextId);

		if (rawConnection.isValid())
		{
			m_rawConnections.push_back(rawConnection);
		}
	}

	ClearConnections();
}

//////////////////////////////////////////////////////////////////////////
void CControl::ReloadConnections()
{
	if (!m_rawConnections.empty())
	{
		for (auto const& rawConnection : m_rawConnections)
		{
			LoadConnectionFromXML(rawConnection);
		}
	}

	m_rawConnections.clear();
}

//////////////////////////////////////////////////////////////////////////
void CControl::SignalControlModified()
{
	if (!g_assetsManager.IsLoading())
	{
		g_assetsManager.OnControlModified(this);
		SetModified(true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::SignalConnectionAdded()
{
	g_assetsManager.OnConnectionAdded(this);
}

//////////////////////////////////////////////////////////////////////////
void CControl::SignalConnectionRemoved()
{
	g_assetsManager.OnConnectionRemoved(this);
}

//////////////////////////////////////////////////////////////////////////
void CControl::SignalConnectionModified()
{
	SignalControlModified();
}

//////////////////////////////////////////////////////////////////////////
void CControl::LoadConnectionFromXML(XmlNodeRef const& node)
{
	IConnection* pConnection = g_pIImpl->CreateConnectionFromXMLNode(node, m_type);

	if (pConnection != nullptr)
	{
		AddConnection(pConnection);
	}
}
} // namespace ACE
