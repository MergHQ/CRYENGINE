// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AudioAssets.h"

#include "AudioControlsEditorPlugin.h"
#include "ImplementationManager.h"

#include <IEditor.h>
#include <IAudioSystemItem.h>
#include <CrySerialization/StringList.h>
#include <CrySerialization/Decorators/Range.h>
#include <Serialization/Decorators/EditorActionButton.h>
#include <Serialization/Decorators/ToggleButton.h>
#include <CryMath/Cry_Geo.h>
#include <Util/Math.h>
#include <ConfigurationManager.h>

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
void CAudioAsset::SetParent(CAudioAsset* const pParent)
{
	m_pParent = pParent;
	SetModified(true);
}

//////////////////////////////////////////////////////////////////////////
void CAudioAsset::AddChild(CAudioAsset* const pChildControl)
{
	m_children.push_back(pChildControl);
	SetModified(true);
}

//////////////////////////////////////////////////////////////////////////
void CAudioAsset::RemoveChild(CAudioAsset const* const pChildControl)
{
	m_children.erase(std::remove(m_children.begin(), m_children.end(), pChildControl), m_children.end());
	SetModified(true);
}

//////////////////////////////////////////////////////////////////////////
void CAudioAsset::SetHiddenDefault(bool const isHiddenDefault)
{
	m_isHiddenDefault = isHiddenDefault;

	if (GetType() == EItemType::Switch)
	{
		for (auto const pChild : m_children)
		{
			pChild->SetHiddenDefault(isHiddenDefault);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioAsset::SetModified(bool const isModified, bool const isForced /* = false */)
{
	if ((m_pParent != nullptr) && (!CAudioControlsEditorPlugin::GetAssetsManager()->IsLoading() || isForced))
	{
		CAudioControlsEditorPlugin::GetAssetsManager()->SetAssetModified(this);
		m_isModified = isModified;
		// Note: This need to get changed once undo is working.
		// Then we can't set the parent to be not modified if it still could contain other modified children.
		m_pParent->SetModified(isModified, isForced);
	}
}

//////////////////////////////////////////////////////////////////////////
CAudioControl::CAudioControl(string const& controlName, CID const id, EItemType const type)
	: CAudioAsset(controlName)
	, m_id(id)
	, m_type(type)
	, m_modifiedSignalEnabled(true)
{
	m_scope = Utils::GetGlobalScope();
}

//////////////////////////////////////////////////////////////////////////
CAudioControl::~CAudioControl()
{
	m_connectedControls.clear();
}

//////////////////////////////////////////////////////////////////////////
void CAudioControl::SetName(string const& name)
{
	if (name != m_name)
	{
		SignalControlAboutToBeModified();
		m_name = Utils::GenerateUniqueControlName(name, GetType(), *CAudioControlsEditorPlugin::GetAssetsManager());
		SignalControlModified();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControl::SetScope(Scope const scope)
{
	if (m_scope != scope)
	{
		SignalControlAboutToBeModified();
		m_scope = scope;
		SignalControlModified();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControl::SetAutoLoad(bool const isAutoLoad)
{
	if (isAutoLoad != m_isAutoLoad)
	{
		SignalControlAboutToBeModified();
		m_isAutoLoad = isAutoLoad;
		SignalControlModified();
	}
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CAudioControl::GetConnectionAt(int const index) const
{
	if (index < m_connectedControls.size())
	{
		return m_connectedControls[index];
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CAudioControl::GetConnection(CID const id) const
{
	for (auto const pConnection : m_connectedControls)
	{
		if ((pConnection != nullptr) && (pConnection->GetID() == id))
		{
			return pConnection;
		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CAudioControl::GetConnection(IAudioSystemItem const* const pAudioSystemControl) const
{
	return GetConnection(pAudioSystemControl->GetId());
}

//////////////////////////////////////////////////////////////////////////
void CAudioControl::AddConnection(ConnectionPtr const pConnection)
{
	if (pConnection != nullptr)
	{
		IAudioSystemEditor* const pAudioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManger()->GetImplementation();

		if (pAudioSystemImpl != nullptr)
		{
			IAudioSystemItem* const pAudioSystemControl = pAudioSystemImpl->GetControl(pConnection->GetID());

			if (pAudioSystemControl != nullptr)
			{
				pAudioSystemImpl->EnableConnection(pConnection);

				pConnection->signalConnectionChanged.Connect(this, &CAudioControl::SignalConnectionModified);
				m_connectedControls.push_back(pConnection);

				if (m_matchRadiusAndAttenuation)
				{
					MatchRadiusToAttenuation();
				}

				SignalConnectionAdded(pAudioSystemControl);
				SignalControlModified();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControl::RemoveConnection(ConnectionPtr const pConnection)
{
	if (pConnection != nullptr)
	{
		auto const it = std::find(m_connectedControls.begin(), m_connectedControls.end(), pConnection);

		if (it != m_connectedControls.end())
		{
			IAudioSystemEditor* const pAudioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManger()->GetImplementation();

			if (pAudioSystemImpl != nullptr)
			{
				IAudioSystemItem* const pAudioSystemControl = pAudioSystemImpl->GetControl(pConnection->GetID());

				if (pAudioSystemControl != nullptr)
				{
					pAudioSystemImpl->DisableConnection(pConnection);
					m_connectedControls.erase(it);

					if (m_matchRadiusAndAttenuation)
					{
						MatchRadiusToAttenuation();
					}

					SignalConnectionRemoved(pAudioSystemControl);
					SignalControlModified();
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControl::ClearConnections()
{
	if (!m_connectedControls.empty())
	{
		IAudioSystemEditor* const pAudioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManger()->GetImplementation();

		if (pAudioSystemImpl != nullptr)
		{
			for (ConnectionPtr const& connection : m_connectedControls)
			{
				pAudioSystemImpl->DisableConnection(connection);
				IAudioSystemItem* const pAudioSystemControl = pAudioSystemImpl->GetControl(connection->GetID());

				if (pAudioSystemControl != nullptr)
				{
					SignalConnectionRemoved(pAudioSystemControl);
				}
			}
		}
		m_connectedControls.clear();

		if (m_matchRadiusAndAttenuation)
		{
			MatchRadiusToAttenuation();
		}

		SignalControlModified();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControl::RemoveConnection(IAudioSystemItem* const pAudioSystemControl)
{
	if (pAudioSystemControl != nullptr)
	{
		CID const id = pAudioSystemControl->GetId();
		auto it = m_connectedControls.begin();
		auto const end = m_connectedControls.end();
		IAudioSystemEditor* const pAudioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManger()->GetImplementation();

		for (; it != end; ++it)
		{
			if ((*it)->GetID() == id)
			{
				if (pAudioSystemImpl != nullptr)
				{
					pAudioSystemImpl->DisableConnection(*it);
				}

				m_connectedControls.erase(it);

				if (m_matchRadiusAndAttenuation)
				{
					MatchRadiusToAttenuation();
				}

				SignalConnectionRemoved(pAudioSystemControl);
				SignalControlModified();
				break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControl::SignalControlModified()
{
	if (m_modifiedSignalEnabled && (!CAudioControlsEditorPlugin::GetAssetsManager()->IsLoading()))
	{
		CAudioControlsEditorPlugin::GetAssetsManager()->OnControlModified(this);
		SetModified(true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControl::SignalControlAboutToBeModified()
{
	if (m_modifiedSignalEnabled && (!CAudioControlsEditorPlugin::GetAssetsManager()->IsLoading()))
	{
		CAudioControlsEditorPlugin::GetAssetsManager()->OnControlAboutToBeModified(this);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControl::SignalConnectionAdded(IAudioSystemItem* const pMiddlewareControl)
{
	CAudioControlsEditorPlugin::GetAssetsManager()->OnConnectionAdded(this, pMiddlewareControl);
}

//////////////////////////////////////////////////////////////////////////
void CAudioControl::SignalConnectionRemoved(IAudioSystemItem* const pMiddlewareControl)
{
	CAudioControlsEditorPlugin::GetAssetsManager()->OnConnectionRemoved(this, pMiddlewareControl);
}

//////////////////////////////////////////////////////////////////////////
void CAudioControl::SignalConnectionModified()
{
	if (m_matchRadiusAndAttenuation)
	{
		MatchRadiusToAttenuation();
	}

	SignalControlModified();
}

//////////////////////////////////////////////////////////////////////////
void CAudioControl::ReloadConnections()
{
	IAudioSystemEditor const* const pAudioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManger()->GetImplementation();

	if (pAudioSystemImpl != nullptr)
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
void CAudioControl::LoadConnectionFromXML(XmlNodeRef const xmlNode, int const platformIndex)
{
	IAudioSystemEditor* const pAudioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManger()->GetImplementation();

	if (pAudioSystemImpl != nullptr)
	{
		ConnectionPtr pConnection = pAudioSystemImpl->CreateConnectionFromXMLNode(xmlNode, GetType());

		if (pConnection != nullptr)
		{
			if (GetType() == EItemType::Preload)
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
		}
		else if ((GetType() == EItemType::Preload) && (platformIndex == -1))
		{
			// If it's a preload connection from another middleware and the platform
			// wasn't found (old file format) fall back to adding them to all the platforms
			std::vector<dll_string> const& platforms = GetIEditor()->GetConfigurationManager()->GetPlatformNames();
			size_t const count = platforms.size();

			for (size_t i = 0; i < count; ++i)
			{
				AddRawXMLConnection(xmlNode, false, i);
			}

			return;
		}

		AddRawXMLConnection(xmlNode, pConnection != nullptr, platformIndex);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControl::Serialize(Serialization::IArchive& ar)
{
	if (ar.openBlock("properties", "+Properties"))
	{
		// Name
		string const newName = m_name;
		ar(newName, "name", "Name");

		// Scope
		Serialization::StringList scopeList;
		ScopeInfoList scopeInfoList;
		CAudioControlsEditorPlugin::GetAssetsManager()->GetScopeInfoList(scopeInfoList);
		
		for (auto const& scope : scopeInfoList)
		{
			scopeList.push_back(scope.name);
		}

		Serialization::StringListValue const selectedScope(scopeList, CAudioControlsEditorPlugin::GetAssetsManager()->GetScopeInfo(m_scope).name);
		Scope newScope = m_scope;

		if (m_type != EItemType::State)
		{
			ar(selectedScope, "scope", "Scope");
			newScope = CAudioControlsEditorPlugin::GetAssetsManager()->GetScope(scopeList[selectedScope.index()]);
		}

		// Auto Load
		bool isAutoLoad = m_isAutoLoad;

		if (m_type == EItemType::Preload)
		{
			ar(isAutoLoad, "auto_load", "Auto Load");
		}

		// Max Radius
		float radius = m_radius;
		float fadeOutDistance = m_occlusionFadeOutDistance;

		if ((m_type == EItemType::Trigger) && (ar.openBlock("activity_radius", "Activity Radius")))
		{
			bool hasPlaceholderConnections = false;
			IAudioSystemEditor const* const pAudioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManger()->GetImplementation();

			if (pAudioSystemImpl != nullptr)
			{
				radius = 0.0f;

				for (auto const pConnection : m_connectedControls)
				{
					IAudioSystemItem const* const pItem = pAudioSystemImpl->GetControl(pConnection->GetID());

					if ((pItem != nullptr) && (!pItem->IsPlaceholder()))
					{
						radius = std::max(radius, pItem->GetRadius());
					}
					else
					{
						// If control has placeholder connection we cannot enforce the link between activity radius
						// and attenuation as the user could be missing the middleware project
						hasPlaceholderConnections = true;
					}
				}
			}

			if (m_matchRadiusAndAttenuation && !hasPlaceholderConnections)
			{
				ar(Serialization::Range<float>(radius, 0.0f, std::numeric_limits<float>::max()), "max_radius", "!^");
			}
			else
			{
				ar(Serialization::Range<float>(m_radius, 0.0f, std::numeric_limits<float>::max()), "max_radius", "^");
			}

			if (!hasPlaceholderConnections)
			{
				if (ar.openBlock("attenuation", "Attenuation"))
				{
					ar(radius, "attenuation", "!^");
					ar(Serialization::ToggleButton(m_matchRadiusAndAttenuation, "icons:Navigation/Tools_Link.ico", "icons:Navigation/Tools_Link_Unlink.ico"), "link", "^");

					if (!m_matchRadiusAndAttenuation)
					{
						radius = m_radius;
					}

					ar.closeBlock();
				}
			}

			ar(Serialization::Range<float>(fadeOutDistance, 0.0f, radius), "fadeOutDistance", "Occlusion Fade-Out Distance");
		}

		if (ar.isInput())
		{
			SignalControlAboutToBeModified();
			m_modifiedSignalEnabled = false; // we are manually sending the signals and don't want the other SetX functions to send anymore
			SetName(newName);
			SetScope(newScope);
			SetAutoLoad(isAutoLoad);
			SetRadius(radius);
			SetOcclusionFadeOutDistance(fadeOutDistance);
			m_modifiedSignalEnabled = true;
			SignalControlModified();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControl::SetOcclusionFadeOutDistance(float const fadeOutDistance)
{
	if (fadeOutDistance != m_occlusionFadeOutDistance)
	{
		SignalControlAboutToBeModified();
		m_occlusionFadeOutDistance = fadeOutDistance;
		SignalControlModified();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControl::AddRawXMLConnection(XmlNodeRef const xmlNode, bool const isValid, int const platformIndex /*= -1*/)
{
	m_connectionNodes[platformIndex].emplace_back(xmlNode, isValid);
}

//////////////////////////////////////////////////////////////////////////
XMLNodeList& CAudioControl::GetRawXMLConnections(int const platformIndex /*= -1*/)
{
	return m_connectionNodes[platformIndex];
}

//////////////////////////////////////////////////////////////////////////
void CAudioControl::MatchRadiusToAttenuation()
{
	IAudioSystemEditor const* const pAudioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManger()->GetImplementation();

	if (pAudioSystemImpl != nullptr)
	{
		float radius = 0.0f;

		for (auto const pConnection : m_connectedControls)
		{
			IAudioSystemItem const* const pItem = pAudioSystemImpl->GetControl(pConnection->GetID());

			if (pItem != nullptr)
			{
				if (pItem->IsPlaceholder())
				{
					// We don't match controls that have placeholder
					// connections as we don't know what the real values should be
					return;
				}
				else
				{
					radius = std::max(radius, pItem->GetRadius());
				}
			}
		}

		SetRadius(radius);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioLibrary::SetModified(bool const isModified, bool const isForced /* = false */)
{
	if (!CAudioControlsEditorPlugin::GetAssetsManager()->IsLoading() || isForced)
	{
		CAudioControlsEditorPlugin::GetAssetsManager()->SetAssetModified(this);
		m_isModified = isModified;
	}
}
} // namespace ACE
