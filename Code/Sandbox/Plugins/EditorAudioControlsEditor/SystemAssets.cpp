// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SystemAssets.h"

#include "AudioControlsEditorPlugin.h"
#include "ImplementationManager.h"

#include <IEditor.h>
#include <ImplItem.h>
#include <CrySerialization/StringList.h>
#include <CryMath/Cry_Geo.h>
#include <Util/Math.h>
#include <ConfigurationManager.h>

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
CSystemAsset::CSystemAsset(string const& name, ESystemItemType const type)
	: m_name(name)
	, m_type(type)
	, m_flags(ESystemAssetFlags::None)
{
}

//////////////////////////////////////////////////////////////////////////
char const* CSystemAsset::GetTypeName() const
{
	char const* typeName = nullptr;

	switch (m_type)
	{
	case ESystemItemType::Trigger:
		typeName = "Trigger";
		break;
	case ESystemItemType::Parameter:
		typeName = "Parameter";
		break;
	case ESystemItemType::Switch:
		typeName = "Switch";
		break;
	case ESystemItemType::State:
		typeName = "State";
		break;
	case ESystemItemType::Environment:
		typeName = "Environment";
		break;
	case ESystemItemType::Preload:
		typeName = "Preload";
		break;
	case ESystemItemType::Folder:
		typeName = "Folder";
		break;
	case ESystemItemType::Library:
		typeName = "Library";
		break;
	default:
		typeName = nullptr;
		break;
	}

	return typeName;
}

//////////////////////////////////////////////////////////////////////////
CSystemAsset* CSystemAsset::GetChild(size_t const index) const
{
	CRY_ASSERT_MESSAGE(index < m_children.size(), "Asset child index out of bounds.");

	CSystemAsset* pAsset = nullptr;

	if (index < m_children.size())
	{
		pAsset = m_children[index];
	}

	return pAsset;
}

//////////////////////////////////////////////////////////////////////////
void CSystemAsset::SetParent(CSystemAsset* const pParent)
{
	m_pParent = pParent;
	SetModified(true);
}

//////////////////////////////////////////////////////////////////////////
void CSystemAsset::AddChild(CSystemAsset* const pChildControl)
{
	m_children.emplace_back(pChildControl);
	SetModified(true);
}

//////////////////////////////////////////////////////////////////////////
void CSystemAsset::RemoveChild(CSystemAsset const* const pChildControl)
{
	m_children.erase(std::remove(m_children.begin(), m_children.end(), pChildControl), m_children.end());
	SetModified(true);
}

//////////////////////////////////////////////////////////////////////////
void CSystemAsset::SetName(string const& name)
{
	if ((!name.IsEmpty()) && (name != m_name))
	{
		if (m_type == ESystemItemType::Library)
		{
			m_name = Utils::GenerateUniqueLibraryName(name, *CAudioControlsEditorPlugin::GetAssetsManager());
			SetModified(true);
			CAudioControlsEditorPlugin::GetAssetsManager()->OnAssetRenamed();
		}
		else if (m_type == ESystemItemType::Folder)
		{
			m_name = Utils::GenerateUniqueName(name, m_type, m_pParent);
			SetModified(true);
			CAudioControlsEditorPlugin::GetAssetsManager()->OnAssetRenamed();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemAsset::UpdateNameOnMove(CSystemAsset* const pParent)
{
	m_name = Utils::GenerateUniqueName(m_name, m_type, pParent);
}

//////////////////////////////////////////////////////////////////////////
void CSystemAsset::SetDescription(string const& description)
{
	if (description != m_description)
	{
		m_description = description;
		SetModified(true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemAsset::SetInternalControl(bool const isInternal)
{
	if (isInternal)
	{
		m_flags |= ESystemAssetFlags::IsInternalControl;
	}
	else
	{
		m_flags &= ~ESystemAssetFlags::IsInternalControl;
	}

	if (m_type == ESystemItemType::Switch)
	{
		for (auto const pChild : m_children)
		{
			pChild->SetInternalControl(isInternal);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemAsset::SetModified(bool const isModified, bool const isForced /* = false */)
{
	if ((m_pParent != nullptr) && (!CAudioControlsEditorPlugin::GetAssetsManager()->IsLoading() || isForced))
	{
		CAudioControlsEditorPlugin::GetAssetsManager()->SetAssetModified(this, isModified);

		if (isModified)
		{
			m_flags |= ESystemAssetFlags::IsModified;
		}
		else
		{
			m_flags &= ~ESystemAssetFlags::IsModified;
		}

		// Note: This need to get changed once undo is working.
		// Then we can't set the parent to be not modified if it still could contain other modified children.
		m_pParent->SetModified(isModified, isForced);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemAsset::SetHasPlaceholderConnection(bool const hasPlaceholder)
{
	if (hasPlaceholder)
	{
		m_flags |= ESystemAssetFlags::HasPlaceholderConnection;
	}
	else
	{
		m_flags &= ~ESystemAssetFlags::HasPlaceholderConnection;
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemAsset::SetHasConnection(bool const hasConnection)
{
	if (hasConnection)
	{
		m_flags |= ESystemAssetFlags::HasConnection;
	}
	else
	{
		m_flags &= ~ESystemAssetFlags::HasConnection;
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemAsset::SetHasControl(bool const hasControl)
{
	if (hasControl)
	{
		m_flags |= ESystemAssetFlags::HasControl;
	}
	else
	{
		m_flags &= ~ESystemAssetFlags::HasControl;
	}
}

//////////////////////////////////////////////////////////////////////////
string CSystemAsset::GetFullHierarchyName() const
{
	string name = m_name;
	CSystemAsset* pParent = m_pParent;

	while (pParent != nullptr)
	{
		name = pParent->GetName() + "/" + name;
		pParent = pParent->GetParent();
	}

	return name;
}

//////////////////////////////////////////////////////////////////////////
bool CSystemAsset::HasDefaultControlChildren(std::vector<string>& names) const
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
			CSystemAsset const* const pChild = GetChild(i);

			if ((pChild != nullptr) && (pChild->HasDefaultControlChildren(names)))
			{
				hasDefaultControlChildren = true;
			}
		}
	}

	return hasDefaultControlChildren;
}

//////////////////////////////////////////////////////////////////////////
void CSystemAsset::Serialize(Serialization::IArchive& ar)
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
CSystemControl::CSystemControl(string const& name, CID const id, ESystemItemType const type)
	: CSystemAsset(name, type)
	, m_id(id)
	, m_scope(Utils::GetGlobalScope())
{
}

//////////////////////////////////////////////////////////////////////////
CSystemControl::~CSystemControl()
{
	m_connectedControls.clear();
}

//////////////////////////////////////////////////////////////////////////
void CSystemControl::SetName(string const& name)
{
	if ((!name.IsEmpty()) && (name != m_name))
	{
		SignalControlAboutToBeModified();

		if (m_type != ESystemItemType::State)
		{
			m_name = Utils::GenerateUniqueControlName(name, m_type, *CAudioControlsEditorPlugin::GetAssetsManager());
		}
		else
		{
			m_name = Utils::GenerateUniqueName(name, m_type, m_pParent);
		}

		SignalControlModified();
		CAudioControlsEditorPlugin::GetAssetsManager()->OnAssetRenamed();
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemControl::SetDescription(string const& description)
{
	if (description != m_description)
	{
		SignalControlAboutToBeModified();
		m_description = description;
		SignalControlModified();
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemAsset::SetDefaultControl(bool const isDefaultControl)
{
	if (isDefaultControl)
	{
		m_flags |= ESystemAssetFlags::IsDefaultControl;
	}
	else
	{
		m_flags &= ~ESystemAssetFlags::IsDefaultControl;
	}

	if (isDefaultControl && (m_type != ESystemItemType::Library) && (m_type != ESystemItemType::Folder))
	{
		CSystemControl* const pControl = static_cast<CSystemControl*>(this);

		if (pControl != nullptr)
		{
			pControl->SetScope(Utils::GetGlobalScope());
		}
	}

	if (m_type == ESystemItemType::Switch)
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
void CSystemControl::SetScope(Scope const scope)
{
	if (m_scope != scope)
	{
		SignalControlAboutToBeModified();
		m_scope = scope;
		SignalControlModified();
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemControl::SetAutoLoad(bool const isAutoLoad)
{
	if (isAutoLoad != m_isAutoLoad)
	{
		SignalControlAboutToBeModified();
		m_isAutoLoad = isAutoLoad;
		SignalControlModified();
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemControl::SetRadius(float const radius)
{
	if (radius != m_radius)
	{
		SignalControlAboutToBeModified();
		m_radius = radius;
		SignalControlModified();
	}
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CSystemControl::GetConnectionAt(size_t const index) const
{
	ConnectionPtr pConnection = nullptr;

	if (index < m_connectedControls.size())
	{
		pConnection = m_connectedControls[index];
	}

	return pConnection;
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CSystemControl::GetConnection(CID const id) const
{
	ConnectionPtr pConnection = nullptr;

	for (auto const& control : m_connectedControls)
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
ConnectionPtr CSystemControl::GetConnection(CImplItem const* const pAudioSystemControl) const
{
	return GetConnection(pAudioSystemControl->GetId());
}

//////////////////////////////////////////////////////////////////////////
void CSystemControl::AddConnection(ConnectionPtr const pConnection)
{
	if (pConnection != nullptr)
	{
		if (g_pEditorImpl != nullptr)
		{
			CImplItem* const pImplControl = g_pEditorImpl->GetControl(pConnection->GetID());

			if (pImplControl != nullptr)
			{
				g_pEditorImpl->EnableConnection(pConnection);
				pConnection->SignalConnectionChanged.Connect(this, &CSystemControl::SignalConnectionModified);
				m_connectedControls.emplace_back(pConnection);
				MatchRadiusToAttenuation();
				SignalConnectionAdded(pImplControl);
				SignalControlModified();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemControl::RemoveConnection(ConnectionPtr const pConnection)
{
	if (pConnection != nullptr)
	{
		auto const it = std::find(m_connectedControls.begin(), m_connectedControls.end(), pConnection);

		if (it != m_connectedControls.end())
		{
			if (g_pEditorImpl != nullptr)
			{
				CImplItem* const pImplControl = g_pEditorImpl->GetControl(pConnection->GetID());

				if (pImplControl != nullptr)
				{
					g_pEditorImpl->DisableConnection(pConnection);
					m_connectedControls.erase(it);
					MatchRadiusToAttenuation();
					SignalConnectionRemoved(pImplControl);
					SignalControlModified();
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemControl::ClearConnections()
{
	if (!m_connectedControls.empty())
	{
		if (g_pEditorImpl != nullptr)
		{
			for (ConnectionPtr const& connection : m_connectedControls)
			{
				g_pEditorImpl->DisableConnection(connection);
				CImplItem* const pImplControl = g_pEditorImpl->GetControl(connection->GetID());

				if (pImplControl != nullptr)
				{
					SignalConnectionRemoved(pImplControl);
				}
			}
		}

		m_connectedControls.clear();
		MatchRadiusToAttenuation();
		SignalControlModified();
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemControl::RemoveConnection(CImplItem* const pImplControl)
{
	if (pImplControl != nullptr)
	{
		CID const id = pImplControl->GetId();
		auto it = m_connectedControls.begin();
		auto const end = m_connectedControls.end();

		for (; it != end; ++it)
		{
			if ((*it)->GetID() == id)
			{
				if (g_pEditorImpl != nullptr)
				{
					g_pEditorImpl->DisableConnection(*it);
				}

				m_connectedControls.erase(it);
				MatchRadiusToAttenuation();
				SignalConnectionRemoved(pImplControl);
				SignalControlModified();
				break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemControl::SignalControlModified()
{
	if (!CAudioControlsEditorPlugin::GetAssetsManager()->IsLoading())
	{
		CAudioControlsEditorPlugin::GetAssetsManager()->OnControlModified(this);
		SetModified(true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemControl::SignalControlAboutToBeModified()
{
	if (!CAudioControlsEditorPlugin::GetAssetsManager()->IsLoading())
	{
		CAudioControlsEditorPlugin::GetAssetsManager()->OnControlAboutToBeModified(this);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemControl::SignalConnectionAdded(CImplItem* const pImplControl)
{
	CAudioControlsEditorPlugin::GetAssetsManager()->OnConnectionAdded(this, pImplControl);
}

//////////////////////////////////////////////////////////////////////////
void CSystemControl::SignalConnectionRemoved(CImplItem* const pImplControl)
{
	CAudioControlsEditorPlugin::GetAssetsManager()->OnConnectionRemoved(this, pImplControl);
}

//////////////////////////////////////////////////////////////////////////
void CSystemControl::SignalConnectionModified()
{
	MatchRadiusToAttenuation();
	SignalControlModified();
}

//////////////////////////////////////////////////////////////////////////
void CSystemControl::ReloadConnections()
{
	if (g_pEditorImpl != nullptr)
	{
		std::map<int, XMLNodeList> connectionNodes;
		std::swap(connectionNodes, m_connectionNodes);

		for (auto const& connectionPair : connectionNodes)
		{
			for (auto const& connection : connectionPair.second)
			{
				LoadConnectionFromXML(connection.xmlNode, connectionPair.first);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemControl::LoadConnectionFromXML(XmlNodeRef const xmlNode, int const platformIndex)
{
	if (g_pEditorImpl != nullptr)
	{
		ConnectionPtr pConnection = g_pEditorImpl->CreateConnectionFromXMLNode(xmlNode, m_type);

		if (pConnection != nullptr)
		{
			if (m_type == ESystemItemType::Preload)
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
					pConnection->EnableForPlatform(platformIndex, true);
				}
			}
			else
			{
				AddConnection(pConnection);
			}

			AddRawXMLConnection(xmlNode, true, platformIndex);
		}
		else if ((m_type == ESystemItemType::Preload) && (platformIndex == -1))
		{
			// If it's a preload connection from another middleware and the platform
			// wasn't found (old file format) fall back to adding them to all the platforms
			std::vector<dll_string> const& platforms = GetIEditor()->GetConfigurationManager()->GetPlatformNames();
			size_t const count = platforms.size();

			for (size_t i = 0; i < count; ++i)
			{
				AddRawXMLConnection(xmlNode, false, i);
			}
		}
		else
		{
			AddRawXMLConnection(xmlNode, false, platformIndex);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemControl::Serialize(Serialization::IArchive& ar)
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

	if (!IsDefaultControl() && (m_type != ESystemItemType::State))
	{
		Serialization::StringList scopeList;
		ScopeInfoList scopeInfoList;
		CAudioControlsEditorPlugin::GetAssetsManager()->GetScopeInfoList(scopeInfoList);

		for (auto const& scopeInfo : scopeInfoList)
		{
			scopeList.emplace_back(scopeInfo.name);
		}

		Serialization::StringListValue const selectedScope(scopeList, CAudioControlsEditorPlugin::GetAssetsManager()->GetScopeInfo(m_scope).name);
		ar(selectedScope, "scope", "Scope");
		scope = CAudioControlsEditorPlugin::GetAssetsManager()->GetScope(scopeList[selectedScope.index()]);
	}

	// Auto Load
	bool isAutoLoad = m_isAutoLoad;

	if (m_type == ESystemItemType::Preload)
	{
		ar(isAutoLoad, "auto_load", "Auto Load");
	}

	// Max Radius
	float radius = m_radius;

	if (!IsDefaultControl() && (m_type == ESystemItemType::Trigger))
	{
		if (g_pEditorImpl != nullptr)
		{
			bool hasPlaceholderConnections = false;
			float connectionMaxRadius = 0.0f;

			for (auto const& connection : m_connectedControls)
			{
				CImplItem const* const pImplControl = g_pEditorImpl->GetControl(connection->GetID());

				if ((pImplControl != nullptr) && !pImplControl->IsPlaceholder())
				{
					connectionMaxRadius = std::max(connectionMaxRadius, pImplControl->GetRadius());
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
void CSystemControl::AddRawXMLConnection(XmlNodeRef const xmlNode, bool const isValid, int const platformIndex /*= -1*/)
{
	m_connectionNodes[platformIndex].emplace_back(xmlNode, isValid);
}

//////////////////////////////////////////////////////////////////////////
XMLNodeList& CSystemControl::GetRawXMLConnections(int const platformIndex /*= -1*/)
{
	return m_connectionNodes[platformIndex];
}

//////////////////////////////////////////////////////////////////////////
void CSystemControl::MatchRadiusToAttenuation()
{
	if (g_pEditorImpl != nullptr)
	{
		float radius = 0.0f;
		bool isPlaceHolder = false;

		for (auto const& connection : m_connectedControls)
		{
			CImplItem const* const pImplControl = g_pEditorImpl->GetControl(connection->GetID());

			if ((pImplControl != nullptr) && !pImplControl->IsPlaceholder())
			{
				radius = std::max(radius, pImplControl->GetRadius());
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
CSystemLibrary::CSystemLibrary(string const& name)
	: CSystemAsset(name, ESystemItemType::Library)
{
}

//////////////////////////////////////////////////////////////////////////
void CSystemLibrary::SetModified(bool const isModified, bool const isForced /* = false */)
{
	if (!CAudioControlsEditorPlugin::GetAssetsManager()->IsLoading() || isForced)
	{
		CAudioControlsEditorPlugin::GetAssetsManager()->SetAssetModified(this, isModified);

		if (isModified)
		{
			m_flags |= ESystemAssetFlags::IsModified;
		}
		else
		{
			m_flags &= ~ESystemAssetFlags::IsModified;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CSystemFolder::CSystemFolder(string const& name)
	: CSystemAsset(name, ESystemItemType::Folder)
{
}
} // namespace ACE
