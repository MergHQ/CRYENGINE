// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Control.h"

#include "Common.h"
#include "AudioControlsEditorPlugin.h"
#include "ImplementationManager.h"
#include "AssetUtils.h"
#include "Common/IConnection.h"
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
	if ((!name.IsEmpty()) && (name != m_name) && ((m_flags& EAssetFlags::IsDefaultControl) == 0))
	{
		SignalOnBeforeControlModified();

		if (m_type != EAssetType::State)
		{
			m_name = AssetUtils::GenerateUniqueControlName(name, m_type);
		}
		else
		{
			m_name = AssetUtils::GenerateUniqueName(name, m_type, m_pParent);
		}

		SignalOnAfterControlModified();
		g_assetsManager.OnAssetRenamed(this);
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::SetDescription(string const& description)
{
	if (description != m_description)
	{
		SignalOnBeforeControlModified();
		m_description = description;
		SignalOnAfterControlModified();
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::Serialize(Serialization::IArchive& ar)
{
	// Name
	string const name = m_name;

	if ((m_flags& EAssetFlags::IsDefaultControl) != 0)
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

	if ((m_flags& EAssetFlags::IsDefaultControl) != 0)
	{
		ar(description, "description", "!Description");
	}
	else
	{
		ar(description, "description", "Description");
	}

	ar.doc(description);

	// Scope
	Scope scope = m_scope;

	if (((m_flags& EAssetFlags::IsDefaultControl) == 0) && (m_type != EAssetType::State))
	{
		Serialization::StringList scopeList;
		ScopeInfos scopeInfos;
		g_assetsManager.GetScopeInfos(scopeInfos);

		for (auto const& scopeInfo : scopeInfos)
		{
			scopeList.emplace_back(scopeInfo.name);
		}

		std::sort(scopeList.begin(), scopeList.end());

		Serialization::StringListValue const selectedScope(scopeList, g_assetsManager.GetScopeInfo(m_scope).name);
		ar(selectedScope, "scope", "Scope");
		scope = g_assetsManager.GetScope(scopeList[selectedScope.index()]);
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
		SetScope(scope);
		SetAutoLoad(isAutoLoad);
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::SetScope(Scope const scope)
{
	if (m_scope != scope)
	{
		SignalOnBeforeControlModified();
		m_scope = scope;
		SignalOnAfterControlModified();
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::SetAutoLoad(bool const isAutoLoad)
{
	if (isAutoLoad != m_isAutoLoad)
	{
		SignalOnBeforeControlModified();
		m_isAutoLoad = isAutoLoad;
		SignalOnAfterControlModified();
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
	if (pIConnection != nullptr)
	{
		Impl::IItem* const pIItem = g_pIImpl->GetItem(pIConnection->GetID());

		if (pIItem != nullptr)
		{
			g_pIImpl->EnableConnection(pIConnection, g_assetsManager.IsLoading());
			pIConnection->SignalConnectionChanged.Connect(this, &CControl::SignalConnectionModified);
			m_connections.push_back(pIConnection);
			SignalConnectionAdded(pIItem);
			SignalOnAfterControlModified();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::RemoveConnection(Impl::IItem* const pIItem)
{
	if (pIItem != nullptr)
	{
		ControlId const id = pIItem->GetId();
		auto iter = m_connections.begin();
		auto const iterEnd = m_connections.end();

		while (iter != iterEnd)
		{
			auto const pIConnection = *iter;

			if (pIConnection->GetID() == id)
			{
				g_pIImpl->DisableConnection(pIConnection, g_assetsManager.IsLoading());
				pIConnection->SignalConnectionChanged.DisconnectById(reinterpret_cast<uintptr_t>(this));
				g_pIImpl->DestructConnection(pIConnection);

				m_connections.erase(iter);
				SignalConnectionRemoved(pIItem);
				SignalOnAfterControlModified();
				break;
			}

			++iter;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::ClearConnections()
{
	if (!m_connections.empty())
	{
		bool const isLoading = g_assetsManager.IsLoading();

		for (auto const pIConnection : m_connections)
		{
			g_pIImpl->DisableConnection(pIConnection, isLoading);
			pIConnection->SignalConnectionChanged.DisconnectById(reinterpret_cast<uintptr_t>(this));
			Impl::IItem* const pIItem = g_pIImpl->GetItem(pIConnection->GetID());
			g_pIImpl->DestructConnection(pIConnection);

			if (pIItem != nullptr)
			{
				SignalConnectionRemoved(pIItem);
			}
		}

		m_connections.clear();
		SignalOnAfterControlModified();
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::BackupAndClearConnections()
{
	// Raw connections are used to temporarily store connections in XML format
	// when middleware data gets reloaded.
	m_rawConnections.clear();

	if ((m_type != EAssetType::Preload) && (m_type != EAssetType::Setting))
	{
		for (auto const pIConnection : m_connections)
		{
			XmlNodeRef const pRawConnection = g_pIImpl->CreateXMLNodeFromConnection(pIConnection, m_type);

			if (pRawConnection != nullptr)
			{
				m_rawConnections[-1].push_back(pRawConnection);
			}
		}
	}
	else
	{
		auto const numPlatforms = static_cast<int>(g_platforms.size());

		for (auto const pIConnection : m_connections)
		{
			XmlNodeRef const pRawConnection = g_pIImpl->CreateXMLNodeFromConnection(pIConnection, m_type);

			if (pRawConnection != nullptr)
			{
				for (int i = 0; i < numPlatforms; ++i)
				{
					if (pIConnection->IsPlatformEnabled(static_cast<PlatformIndexType>(i)))
					{
						m_rawConnections[i].push_back(pRawConnection);
					}
				}
			}
		}
	}

	ClearConnections();
}

//////////////////////////////////////////////////////////////////////////
void CControl::ReloadConnections()
{
	if (!m_rawConnections.empty())
	{
		for (auto const& connectionPair : m_rawConnections)
		{
			for (auto const& connection : connectionPair.second)
			{
				LoadConnectionFromXML(connection, connectionPair.first);
			}
		}
	}

	m_rawConnections.clear();
}

//////////////////////////////////////////////////////////////////////////
void CControl::SignalOnAfterControlModified()
{
	if (!g_assetsManager.IsLoading())
	{
		g_assetsManager.OnAfterControlModified(this);
		SetModified(true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::SignalOnBeforeControlModified()
{
	if (!g_assetsManager.IsLoading())
	{
		g_assetsManager.OnBeforeControlModified(this);
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::SignalConnectionAdded(Impl::IItem* const pIItem)
{
	g_assetsManager.OnConnectionAdded(this, pIItem);
}

//////////////////////////////////////////////////////////////////////////
void CControl::SignalConnectionRemoved(Impl::IItem* const pIItem)
{
	g_assetsManager.OnConnectionRemoved(this, pIItem);
}

//////////////////////////////////////////////////////////////////////////
void CControl::SignalConnectionModified()
{
	SignalOnAfterControlModified();
}

//////////////////////////////////////////////////////////////////////////
void CControl::LoadConnectionFromXML(XmlNodeRef const xmlNode, int const platformIndex /*= -1*/)
{
	IConnection* pConnection = g_pIImpl->CreateConnectionFromXMLNode(xmlNode, m_type);

	if (pConnection != nullptr)
	{
		if ((m_type == EAssetType::Preload) || (m_type == EAssetType::Setting))
		{
			// The connection could already exist but using a different platform
			IConnection* const pPreviousConnection = GetConnection(pConnection->GetID());

			if (pPreviousConnection == nullptr)
			{
				if (platformIndex != -1)
				{
					pConnection->ClearPlatforms();
				}

				AddConnection(pConnection);
			}
			else
			{
				pConnection = pPreviousConnection;
			}

			if (platformIndex != -1)
			{
				pConnection->SetPlatformEnabled(static_cast<PlatformIndexType>(platformIndex), true);
			}
		}
		else
		{
			AddConnection(pConnection);
		}
	}
}
} // namespace ACE
