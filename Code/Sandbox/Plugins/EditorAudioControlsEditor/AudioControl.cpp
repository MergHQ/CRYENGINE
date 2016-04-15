// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AudioControl.h"
#include "ATLControlsModel.h"
#include "AudioControlsEditorUndo.h"
#include "IEditor.h"
#include <IAudioSystemItem.h>
#include <ACETypes.h>
#include "AudioControlsEditorPlugin.h"
#include "ImplementationManager.h"

namespace ACE
{

const string g_sDefaultGroup = "default";

CATLControl::CATLControl()
	: m_sName("")
	, m_nID(ACE_INVALID_ID)
	, m_eType(eACEControlType_RTPC)
	, m_sScope("")
	, m_bAutoLoad(true)
	, m_pParent(nullptr)
	, m_pModel(nullptr)
	, m_bModified(false)
{
}

CATLControl::CATLControl(const string& sControlName, CID nID, EACEControlType eType, CATLControlsModel* pModel)
	: m_sName(sControlName)
	, m_nID(nID)
	, m_eType(eType)
	, m_sScope("")
	, m_bAutoLoad(true)
	, m_pParent(nullptr)
	, m_pModel(pModel)
	, m_bModified(false)
{
}

CATLControl::~CATLControl()
{
	m_connectedControls.clear();
}

CID CATLControl::GetId() const
{
	return m_nID;
}

EACEControlType CATLControl::GetType() const
{
	return m_eType;
}

string CATLControl::GetName() const
{
	return m_sName;
}

void CATLControl::SetId(CID id)
{
	if (id != m_nID)
	{
		SignalControlAboutToBeModified();
		m_nID = id;
		SignalControlModified();
	}
}

void CATLControl::SetType(EACEControlType type)
{
	if (type != m_eType)
	{
		SignalControlAboutToBeModified();
		m_eType = type;
		SignalControlModified();
	}
}

void CATLControl::SetName(const string& name)
{
	if (name != m_sName)
	{
		SignalControlAboutToBeModified();
		m_sName = name;
		SignalControlModified();
	}
}

string CATLControl::GetScope() const
{
	return m_sScope;
}

void CATLControl::SetScope(const string& sScope)
{
	if (m_sScope != sScope)
	{
		SignalControlAboutToBeModified();
		m_sScope = sScope;
		SignalControlModified();
	}
}

bool CATLControl::HasScope() const
{
	return !m_sScope.empty();
}

bool CATLControl::IsAutoLoad() const
{
	return m_bAutoLoad;
}

void CATLControl::SetAutoLoad(bool bAutoLoad)
{
	if (bAutoLoad != m_bAutoLoad)
	{
		SignalControlAboutToBeModified();
		m_bAutoLoad = bAutoLoad;
		SignalControlModified();
	}
}

int CATLControl::GetGroupForPlatform(const string& platform) const
{
	auto it = m_groupPerPlatform.find(platform);
	if (it == m_groupPerPlatform.end())
	{
		return 0;
	}
	return it->second;
}

void CATLControl::SetGroupForPlatform(const string& platform, int connectionGroupId)
{
	if (m_groupPerPlatform[platform] != connectionGroupId)
	{
		SignalControlAboutToBeModified();
		m_groupPerPlatform[platform] = connectionGroupId;
		SignalControlModified();
	}
}

size_t CATLControl::GetConnectionCount()
{
	return m_connectedControls.size();
}

ConnectionPtr CATLControl::GetConnectionAt(int index)
{
	if (index < m_connectedControls.size())
	{
		return m_connectedControls[index];
	}
	return nullptr;
}

ConnectionPtr CATLControl::GetConnection(CID id, const string& group)
{
	if (id >= 0)
	{
		const size_t size = m_connectedControls.size();
		for (int i = 0; i < size; ++i)
		{
			ConnectionPtr pConnection = m_connectedControls[i];
			if (pConnection && pConnection->GetID() == id && pConnection->GetGroup() == group)
			{
				return pConnection;
			}
		}
	}
	return nullptr;
}

ConnectionPtr CATLControl::GetConnection(IAudioSystemItem* m_pAudioSystemControl, const string& group /*= ""*/)
{
	return GetConnection(m_pAudioSystemControl->GetId(), group);
}

void CATLControl::AddConnection(ConnectionPtr pConnection)
{
	if (pConnection)
	{
		SignalControlAboutToBeModified();
		m_connectedControls.push_back(pConnection);
		IAudioSystemEditor* pAudioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManger()->GetImplementation();
		if (pAudioSystemImpl)
		{
			IAudioSystemItem* pAudioSystemControl = pAudioSystemImpl->GetControl(pConnection->GetID());
			if (pAudioSystemControl)
			{
				pAudioSystemImpl->EnableConnection(pConnection);
				SignalConnectionAdded(pAudioSystemControl);
			}
		}
		SignalControlModified();
	}
}

void CATLControl::RemoveConnection(ConnectionPtr pConnection)
{
	if (pConnection)
	{
		auto it = std::find(m_connectedControls.begin(), m_connectedControls.end(), pConnection);
		if (it != m_connectedControls.end())
		{
			SignalControlAboutToBeModified();
			IAudioSystemEditor* pAudioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManger()->GetImplementation();
			if (pAudioSystemImpl)
			{
				IAudioSystemItem* pAudioSystemControl = pAudioSystemImpl->GetControl(pConnection->GetID());
				if (pAudioSystemControl)
				{
					pAudioSystemImpl->DisableConnection(pConnection);
					m_connectedControls.erase(it);
					SignalConnectionRemoved(pAudioSystemControl);
				}
			}
			SignalControlModified();
		}
	}
}

void CATLControl::ClearConnections()
{
	SignalControlAboutToBeModified();
	IAudioSystemEditor* pAudioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManger()->GetImplementation();
	if (pAudioSystemImpl)
	{
		for (ConnectionPtr& c : m_connectedControls)
		{
			pAudioSystemImpl->DisableConnection(c);
			IAudioSystemItem* pAudioSystemControl = pAudioSystemImpl->GetControl(c->GetID());
			if (pAudioSystemControl)
			{
				SignalConnectionRemoved(pAudioSystemControl);
			}
		}
	}
	m_connectedControls.clear();
	SignalControlModified();
}

void CATLControl::RemoveConnection(IAudioSystemItem* pAudioSystemControl)
{
	if (pAudioSystemControl)
	{
		const CID nID = pAudioSystemControl->GetId();
		auto it = m_connectedControls.begin();
		auto end = m_connectedControls.end();
		for (; it != end; ++it)
		{
			if ((*it)->GetID() == nID)
			{
				SignalControlAboutToBeModified();
				IAudioSystemEditor* pAudioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManger()->GetImplementation();
				if (pAudioSystemImpl)
				{
					pAudioSystemImpl->DisableConnection(*it);
				}
				m_connectedControls.erase(it);

				SignalConnectionRemoved(pAudioSystemControl);
				SignalControlModified();
				return;
			}
		}
	}
}

void CATLControl::SignalControlModified()
{
	if (m_pModel)
	{
		m_pModel->OnControlModified(this);
	}
}

void CATLControl::SignalControlAboutToBeModified()
{
	if (!CUndo::IsSuspended())
	{
		CUndo undo("ATL Control Modified");
		CUndo::Record(new CUndoControlModified(GetId()));
	}
}

void CATLControl::SignalConnectionAdded(IAudioSystemItem* pMiddlewareControl)
{
	if (m_pModel)
	{
		m_pModel->OnConnectionAdded(this, pMiddlewareControl);
	}
}

void CATLControl::SignalConnectionRemoved(IAudioSystemItem* pMiddlewareControl)
{
	if (m_pModel)
	{
		m_pModel->OnConnectionRemoved(this, pMiddlewareControl);
	}
}

void CATLControl::ReloadConnections()
{
	IAudioSystemEditor* pAudioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManger()->GetImplementation();
	if (pAudioSystemImpl)
	{
		TConnectionPerGroup::iterator it = m_connectionNodes.begin();
		TConnectionPerGroup::iterator end = m_connectionNodes.end();
		for (; it != end; ++it)
		{
			TXMLNodeList& nodeList = it->second;
			const size_t size = nodeList.size();
			for (size_t i = 0; i < size; ++i)
			{
				ConnectionPtr pConnection = pAudioSystemImpl->CreateConnectionFromXMLNode(nodeList[i].xmlNode, m_eType);
				if (pConnection)
				{
					AddConnection(pConnection);
					nodeList[i].bValid = true;
				}
				else
				{
					nodeList[i].bValid = false;
				}
			}
		}
	}
}

void CATLControl::SetParent(CATLControl* pParent)
{
	m_pParent = pParent;
	if (pParent)
	{
		SetScope(pParent->GetScope());
	}
}

}
