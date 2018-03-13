// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Assets.h"

#include "AudioControlsEditorPlugin.h"
#include "ImplementationManager.h"

#include <IImplItem.h>
#include <CrySerialization/StringList.h>
#include <CryMath/Cry_Geo.h>
#include <Util/Math.h>

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
CAsset::CAsset(string const& name, EAssetType const type)
	: m_name(name)
	, m_type(type)
	, m_flags(EAssetFlags::None)
{}

//////////////////////////////////////////////////////////////////////////
char const* CAsset::GetTypeName() const
{
	char const* szTypeName = nullptr;

	switch (m_type)
	{
	case EAssetType::Trigger:
		szTypeName = "Trigger";
		break;
	case EAssetType::Parameter:
		szTypeName = "Parameter";
		break;
	case EAssetType::Switch:
		szTypeName = "Switch";
		break;
	case EAssetType::State:
		szTypeName = "State";
		break;
	case EAssetType::Environment:
		szTypeName = "Environment";
		break;
	case EAssetType::Preload:
		szTypeName = "Preload";
		break;
	case EAssetType::Folder:
		szTypeName = "Folder";
		break;
	case EAssetType::Library:
		szTypeName = "Library";
		break;
	default:
		szTypeName = nullptr;
		break;
	}

	return szTypeName;
}

//////////////////////////////////////////////////////////////////////////
CAsset* CAsset::GetChild(size_t const index) const
{
	CRY_ASSERT_MESSAGE(index < m_children.size(), "Asset child index out of bounds.");

	CAsset* pAsset = nullptr;

	if (index < m_children.size())
	{
		pAsset = m_children[index];
	}

	return pAsset;
}

//////////////////////////////////////////////////////////////////////////
void CAsset::SetParent(CAsset* const pParent)
{
	m_pParent = pParent;
	SetModified(true);
}

//////////////////////////////////////////////////////////////////////////
void CAsset::AddChild(CAsset* const pChildControl)
{
	m_children.push_back(pChildControl);
	SetModified(true);
}

//////////////////////////////////////////////////////////////////////////
void CAsset::RemoveChild(CAsset const* const pChildControl)
{
	m_children.erase(std::remove(m_children.begin(), m_children.end(), pChildControl), m_children.end());
	SetModified(true);
}

//////////////////////////////////////////////////////////////////////////
void CAsset::SetName(string const& name)
{
	if ((!name.IsEmpty()) && (name != m_name) && !IsDefaultControl())
	{
		if (m_type == EAssetType::Library)
		{
			m_name = Utils::GenerateUniqueLibraryName(name);
			SetModified(true);
			g_assetsManager.OnAssetRenamed(this);
		}
		else if (m_type == EAssetType::Folder)
		{
			m_name = Utils::GenerateUniqueName(name, m_type, m_pParent);
			SetModified(true);
			g_assetsManager.OnAssetRenamed(this);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAsset::UpdateNameOnMove(CAsset* const pParent)
{
	m_name = Utils::GenerateUniqueName(m_name, m_type, pParent);
}

//////////////////////////////////////////////////////////////////////////
void CAsset::SetDescription(string const& description)
{
	if (description != m_description)
	{
		m_description = description;
		SetModified(true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAsset::SetDefaultControl(bool const isDefaultControl)
{
	if (isDefaultControl)
	{
		m_flags |= EAssetFlags::IsDefaultControl;
	}
	else
	{
		m_flags &= ~EAssetFlags::IsDefaultControl;
	}

	if (isDefaultControl && (m_type != EAssetType::Library) && (m_type != EAssetType::Folder))
	{
		CControl* const pControl = static_cast<CControl*>(this);

		if (pControl != nullptr)
		{
			pControl->SetScope(Utils::GetGlobalScope());
		}
	}

	if (m_type == EAssetType::Switch)
	{
		for (auto const pChild : m_children)
		{
			if (pChild != nullptr)
			{
				pChild->SetDefaultControl(isDefaultControl);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAsset::SetInternalControl(bool const isInternal)
{
	if (isInternal)
	{
		m_flags |= EAssetFlags::IsInternalControl;
	}
	else
	{
		m_flags &= ~EAssetFlags::IsInternalControl;
	}

	if (m_type == EAssetType::Switch)
	{
		for (auto const pChild : m_children)
		{
			pChild->SetInternalControl(isInternal);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAsset::SetModified(bool const isModified, bool const isForced /* = false */)
{
	if ((m_pParent != nullptr) && (!g_assetsManager.IsLoading() || isForced))
	{
		g_assetsManager.SetAssetModified(this, isModified);

		if (isModified)
		{
			m_flags |= EAssetFlags::IsModified;
		}
		else
		{
			m_flags &= ~EAssetFlags::IsModified;
		}

		// Note: This need to get changed once undo is working.
		// Then we can't set the parent to be not modified if it still could contain other modified children.
		m_pParent->SetModified(isModified, isForced);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAsset::SetHasPlaceholderConnection(bool const hasPlaceholder)
{
	if (hasPlaceholder)
	{
		m_flags |= EAssetFlags::HasPlaceholderConnection;
	}
	else
	{
		m_flags &= ~EAssetFlags::HasPlaceholderConnection;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAsset::SetHasConnection(bool const hasConnection)
{
	if (hasConnection)
	{
		m_flags |= EAssetFlags::HasConnection;
	}
	else
	{
		m_flags &= ~EAssetFlags::HasConnection;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAsset::SetHasControl(bool const hasControl)
{
	if (hasControl)
	{
		m_flags |= EAssetFlags::HasControl;
	}
	else
	{
		m_flags &= ~EAssetFlags::HasControl;
	}
}

//////////////////////////////////////////////////////////////////////////
string CAsset::GetFullHierarchyName() const
{
	string name = m_name;
	CAsset* pParent = m_pParent;

	while (pParent != nullptr)
	{
		name = pParent->GetName() + "/" + name;
		pParent = pParent->GetParent();
	}

	return name;
}

//////////////////////////////////////////////////////////////////////////
bool CAsset::HasDefaultControlChildren(std::vector<string>& names) const
{
	bool hasDefaultControlChildren = IsDefaultControl();

	if (hasDefaultControlChildren)
	{
		names.emplace_back(GetFullHierarchyName());
	}
	else
	{
		size_t const childCount = ChildCount();

		for (size_t i = 0; i < childCount; ++i)
		{
			CAsset const* const pChild = GetChild(i);

			if ((pChild != nullptr) && (pChild->HasDefaultControlChildren(names)))
			{
				hasDefaultControlChildren = true;
			}
		}
	}

	return hasDefaultControlChildren;
}

//////////////////////////////////////////////////////////////////////////
void CAsset::Serialize(Serialization::IArchive& ar)
{
	string const name = m_name;

	if (IsDefaultControl())
	{
		ar(name, "name", "!Name");
	}
	else
	{
		ar(name, "name", "Name");
	}

	ar.doc(name);

	string const description = m_description;

	if (IsDefaultControl())
	{
		ar(description, "description", "!Description");
	}
	else
	{
		ar(description, "description", "Description");
	}

	ar.doc(description);

	if (ar.isInput())
	{
		SetName(name);
		SetDescription(description);
	}
}

//////////////////////////////////////////////////////////////////////////
CControl::CControl(string const& name, ControlId const id, EAssetType const type)
	: CAsset(name, type)
	, m_id(id)
	, m_scope(Utils::GetGlobalScope())
{}

//////////////////////////////////////////////////////////////////////////
CControl::~CControl()
{
	m_connections.clear();
}

//////////////////////////////////////////////////////////////////////////
void CControl::SetName(string const& name)
{
	if ((!name.IsEmpty()) && (name != m_name) && !IsDefaultControl())
	{
		SignalControlAboutToBeModified();

		if (m_type != EAssetType::State)
		{
			m_name = Utils::GenerateUniqueControlName(name, m_type);
		}
		else
		{
			m_name = Utils::GenerateUniqueName(name, m_type, m_pParent);
		}

		SignalControlModified();
		g_assetsManager.OnAssetRenamed(this);
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::SetDescription(string const& description)
{
	if (description != m_description)
	{
		SignalControlAboutToBeModified();
		m_description = description;
		SignalControlModified();
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::Serialize(Serialization::IArchive& ar)
{
	// Name
	string const name = m_name;

	if (IsDefaultControl())
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

	if (IsDefaultControl())
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

	if (!IsDefaultControl() && (m_type != EAssetType::State))
	{
		Serialization::StringList scopeList;
		ScopeInfoList scopeInfoList;
		g_assetsManager.GetScopeInfoList(scopeInfoList);

		for (auto const& scopeInfo : scopeInfoList)
		{
			scopeList.emplace_back(scopeInfo.name);
		}

		Serialization::StringListValue const selectedScope(scopeList, g_assetsManager.GetScopeInfo(m_scope).name);
		ar(selectedScope, "scope", "Scope");
		scope = g_assetsManager.GetScope(scopeList[selectedScope.index()]);
	}

	// Auto Load
	bool isAutoLoad = m_isAutoLoad;

	if (m_type == EAssetType::Preload)
	{
		ar(isAutoLoad, "auto_load", "Auto Load");
	}

	// Max Radius
	float radius = m_radius;

	if (!IsDefaultControl() && (m_type == EAssetType::Trigger))
	{
		if (g_pEditorImpl != nullptr)
		{
			bool hasPlaceholderConnections = false;
			float connectionMaxRadius = 0.0f;

			for (auto const& connection : m_connections)
			{
				IImplItem const* const pIImplItem = g_pEditorImpl->GetItem(connection->GetID());

				if ((pIImplItem != nullptr) && !pIImplItem->IsPlaceholder())
				{
					connectionMaxRadius = std::max(connectionMaxRadius, pIImplItem->GetRadius());
				}
				else
				{
					// If control has placeholder connection we cannot enforce the link between activity radius
					// and attenuation as the user could be missing the middleware project.
					hasPlaceholderConnections = true;
					break;
				}
			}

			if (!hasPlaceholderConnections)
			{
				radius = connectionMaxRadius;
			}
		}
	}

	if (ar.isInput())
	{
		SetName(name);
		SetDescription(description);
		SetScope(scope);
		SetAutoLoad(isAutoLoad);
		SetRadius(radius);
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::SetScope(Scope const scope)
{
	if (m_scope != scope)
	{
		SignalControlAboutToBeModified();
		m_scope = scope;
		SignalControlModified();
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::SetAutoLoad(bool const isAutoLoad)
{
	if (isAutoLoad != m_isAutoLoad)
	{
		SignalControlAboutToBeModified();
		m_isAutoLoad = isAutoLoad;
		SignalControlModified();
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::SetRadius(float const radius)
{
	if (radius != m_radius)
	{
		SignalControlAboutToBeModified();
		m_radius = radius;
		SignalControlModified();
	}
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CControl::GetConnectionAt(size_t const index) const
{
	ConnectionPtr pConnection = nullptr;

	if (index < m_connections.size())
	{
		pConnection = m_connections[index];
	}

	return pConnection;
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CControl::GetConnection(ControlId const id) const
{
	ConnectionPtr pConnection = nullptr;

	for (auto const& control : m_connections)
	{
		if ((control != nullptr) && (control->GetID() == id))
		{
			pConnection = control;
			break;
		}
	}

	return pConnection;
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CControl::GetConnection(IImplItem const* const pIImplItem) const
{
	return GetConnection(pIImplItem->GetId());
}

//////////////////////////////////////////////////////////////////////////
void CControl::AddConnection(ConnectionPtr const pConnection)
{
	if (pConnection != nullptr)
	{
		if (g_pEditorImpl != nullptr)
		{
			IImplItem* const pIImplItem = g_pEditorImpl->GetItem(pConnection->GetID());

			if (pIImplItem != nullptr)
			{
				g_pEditorImpl->EnableConnection(pConnection);
				pConnection->SignalConnectionChanged.Connect(this, &CControl::SignalConnectionModified);
				m_connections.push_back(pConnection);
				MatchRadiusToAttenuation();
				SignalConnectionAdded(pIImplItem);
				SignalControlModified();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::RemoveConnection(ConnectionPtr const pConnection)
{
	if (pConnection != nullptr)
	{
		auto const it = std::find(m_connections.begin(), m_connections.end(), pConnection);

		if (it != m_connections.end())
		{
			if (g_pEditorImpl != nullptr)
			{
				IImplItem* const pIImplItem = g_pEditorImpl->GetItem(pConnection->GetID());

				if (pIImplItem != nullptr)
				{
					g_pEditorImpl->DisableConnection(pConnection);
					pConnection->SignalConnectionChanged.DisconnectById(reinterpret_cast<uintptr_t>(this));
					m_connections.erase(it);
					MatchRadiusToAttenuation();
					SignalConnectionRemoved(pIImplItem);
					SignalControlModified();
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::ClearConnections()
{
	if (!m_connections.empty())
	{
		if (g_pEditorImpl != nullptr)
		{
			for (ConnectionPtr const& connection : m_connections)
			{
				g_pEditorImpl->DisableConnection(connection);
				IImplItem* const pIImplItem = g_pEditorImpl->GetItem(connection->GetID());

				if (pIImplItem != nullptr)
				{
					SignalConnectionRemoved(pIImplItem);
				}
			}
		}

		m_connections.clear();
		MatchRadiusToAttenuation();
		SignalControlModified();
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::BackupAndClearConnections()
{
	m_rawConnections.clear();

	// Raw connections are used to temporarily store connections in XML format
	// when middleware data gets reloaded.
	if (g_pEditorImpl != nullptr)
	{
		if (m_type != EAssetType::Preload)
		{
			for (auto const& connection : m_connections)
			{
				XmlNodeRef const pRawConnection = g_pEditorImpl->CreateXMLNodeFromConnection(connection, m_type);

				if (pRawConnection != nullptr)
				{
					m_rawConnections[-1].push_back(pRawConnection);
				}
			}
		}
		else
		{
			auto const numPlatforms = static_cast<int>(g_platforms.size());

			for (auto const& connection : m_connections)
			{
				XmlNodeRef const pRawConnection = g_pEditorImpl->CreateXMLNodeFromConnection(connection, m_type);

				if (pRawConnection != nullptr)
				{
					for (int i = 0; i < numPlatforms; ++i)
					{
						if (connection->IsPlatformEnabled(static_cast<PlatformIndexType>(i)))
						{
							m_rawConnections[i].push_back(pRawConnection);
						}
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
	if (!m_rawConnections.empty() && (g_pEditorImpl != nullptr))
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
void CControl::RemoveConnection(IImplItem* const pIImplItem)
{
	if (pIImplItem != nullptr)
	{
		ControlId const id = pIImplItem->GetId();
		auto it = m_connections.begin();
		auto const end = m_connections.end();

		for (; it != end; ++it)
		{
			if ((*it)->GetID() == id)
			{
				if (g_pEditorImpl != nullptr)
				{
					g_pEditorImpl->DisableConnection(*it);
				}

				m_connections.erase(it);
				MatchRadiusToAttenuation();
				SignalConnectionRemoved(pIImplItem);
				SignalControlModified();
				break;
			}
		}
	}
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
void CControl::SignalControlAboutToBeModified()
{
	if (!g_assetsManager.IsLoading())
	{
		g_assetsManager.OnControlAboutToBeModified(this);
	}
}

//////////////////////////////////////////////////////////////////////////
void CControl::SignalConnectionAdded(IImplItem* const pIImplItem)
{
	g_assetsManager.OnConnectionAdded(this, pIImplItem);
}

//////////////////////////////////////////////////////////////////////////
void CControl::SignalConnectionRemoved(IImplItem* const pIImplItem)
{
	g_assetsManager.OnConnectionRemoved(this, pIImplItem);
}

//////////////////////////////////////////////////////////////////////////
void CControl::SignalConnectionModified()
{
	MatchRadiusToAttenuation();
	SignalControlModified();
}

//////////////////////////////////////////////////////////////////////////
void CControl::LoadConnectionFromXML(XmlNodeRef const xmlNode, int const platformIndex /*= -1*/)
{
	if (g_pEditorImpl != nullptr)
	{
		ConnectionPtr pConnection = g_pEditorImpl->CreateConnectionFromXMLNode(xmlNode, m_type);

		if (pConnection != nullptr)
		{
			if (m_type == EAssetType::Preload)
			{
				// The connection could already exist but using a different platform
				ConnectionPtr const pPreviousConnection = GetConnection(pConnection->GetID());

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
}

//////////////////////////////////////////////////////////////////////////
void CControl::MatchRadiusToAttenuation()
{
	if (g_pEditorImpl != nullptr)
	{
		float radius = 0.0f;
		bool isPlaceHolder = false;

		for (auto const& connection : m_connections)
		{
			IImplItem const* const pIImplItem = g_pEditorImpl->GetItem(connection->GetID());

			if ((pIImplItem != nullptr) && !pIImplItem->IsPlaceholder())
			{
				radius = std::max(radius, pIImplItem->GetRadius());
			}
			else
			{
				// We don't match controls that have placeholder
				// connections as we don't know what the real values should be.
				isPlaceHolder = true;
				break;
			}
		}

		if (!isPlaceHolder)
		{
			SetRadius(radius);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CLibrary::CLibrary(string const& name)
	: CAsset(name, EAssetType::Library)
	, m_pakStatusFlags(EPakStatus::None)
{}

//////////////////////////////////////////////////////////////////////////
void CLibrary::SetModified(bool const isModified, bool const isForced /* = false */)
{
	if (!g_assetsManager.IsLoading() || isForced)
	{
		g_assetsManager.SetAssetModified(this, isModified);

		if (isModified)
		{
			m_flags |= EAssetFlags::IsModified;
		}
		else
		{
			m_flags &= ~EAssetFlags::IsModified;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CLibrary::SetPakStatus(EPakStatus const pakStatus, bool const exists)
{
	if (exists)
	{
		m_pakStatusFlags |= pakStatus;
	}
	else
	{
		m_pakStatusFlags &= ~pakStatus;
	}
}

//////////////////////////////////////////////////////////////////////////
CFolder::CFolder(string const& name)
	: CAsset(name, EAssetType::Folder)
{}
} // namespace ACE
