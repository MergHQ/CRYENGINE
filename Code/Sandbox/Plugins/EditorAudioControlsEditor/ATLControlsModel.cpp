// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ATLControlsModel.h"
#include <CryString/StringUtils.h>
#include "AudioControlsEditorUndo.h"
#include <IEditor.h>

namespace ACE
{
CID CATLControlsModel::m_nextId = 1;

CATLControlsModel::CATLControlsModel()
	: m_bSuppressMessages(false)
{
	ClearDirtyFlags();
}

CATLControlsModel::~CATLControlsModel()
{
	Clear();
}

CATLControl* CATLControlsModel::CreateControl(const string& sControlName, EACEControlType eType, CATLControl* pParent)
{
	std::shared_ptr<CATLControl> pControl = std::make_shared<CATLControl>(sControlName, GenerateUniqueId(), eType, this);
	if (pControl)
	{
		if (pParent)
		{
			pControl->SetParent(pParent);
		}

		InsertControl(pControl);

		if (!CUndo::IsSuspended())
		{
			CUndo undo("Audio Control Created");
			CUndo::Record(new CUndoControlAdd(pControl->GetId()));
		}
	}
	return pControl.get();
}

void CATLControlsModel::RemoveControl(CID id)
{
	if (id >= 0)
	{
		for (auto it = m_controls.begin(); it != m_controls.end(); ++it)
		{
			std::shared_ptr<CATLControl>& pControl = *it;
			if (pControl && pControl->GetId() == id)
			{

				OnControlRemoved(pControl.get());
				pControl->ClearConnections();

				// Remove control from parent
				CATLControl* pParent = pControl->GetParent();
				if (pParent)
				{
					pParent->RemoveChild(pControl.get());
				}

				if (!CUndo::IsSuspended())
				{
					CUndo::Record(new CUndoControlRemove(pControl));
				}
				m_controls.erase(it, it + 1);
				break;
			}
		}
	}
}

CATLControl* CATLControlsModel::GetControlByID(CID id) const
{
	if (id >= 0)
	{
		size_t size = m_controls.size();
		for (size_t i = 0; i < size; ++i)
		{
			if (m_controls[i]->GetId() == id)
			{
				return m_controls[i].get();
			}
		}
	}
	return nullptr;
}

bool CATLControlsModel::IsNameValid(const string& name, EACEControlType type, const string& scope, const CATLControl* const pParent) const
{
	const size_t size = m_controls.size();
	for (size_t i = 0; i < size; ++i)
	{
		ATLControlPtr pControl = m_controls[i];
		if (pControl)
		{
			if ((pControl->GetType() == type &&
			     (pControl->GetName().compareNoCase(name) == 0)) &&
			    (pControl->GetScope() == "" || pControl->GetScope() == scope) &&
			    (pControl->GetParent() == pParent))
			{
				return false;
			}
		}
	}
	return true;
}

string CATLControlsModel::GenerateUniqueName(const string& sRootName, EACEControlType eType, const string& sScope, const CATLControl* const pParent) const
{
	string sFinalName = sRootName;
	int nNumber = 1;
	while (!IsNameValid(sFinalName, eType, sScope, pParent))
	{
		sFinalName = sRootName + "_" + CryStringUtils::toString(nNumber);
		++nNumber;
	}

	return sFinalName;
}

void CATLControlsModel::AddScope(const string& name, bool bLocalOnly)
{
	string scopeName = name;
	scopeName.MakeLower();
	const size_t size = m_scopes.size();
	for (int i = 0; i < size; ++i)
	{
		if (m_scopes[i].name == scopeName)
		{
			return;
		}
	}
	m_scopes.push_back(SControlScope(scopeName, bLocalOnly));
}

void CATLControlsModel::ClearScopes()
{
	m_scopes.clear();
}

bool CATLControlsModel::ScopeExists(const string& name) const
{
	string scopeName = name;
	scopeName.MakeLower();
	const size_t size = m_scopes.size();
	for (int i = 0; i < size; ++i)
	{
		if (m_scopes[i].name == name)
		{
			return true;
		}
	}
	return false;
}

int CATLControlsModel::GetScopeCount() const
{
	return m_scopes.size();
}

SControlScope CATLControlsModel::GetScopeAt(int index) const
{
	if (index < m_scopes.size())
	{
		return m_scopes[index];
	}
	return SControlScope();
}

string CATLControlsModel::GetPlatformAt(uint index)
{
	if (index < m_platforms.size())
	{
		return m_platforms[index];
	}
	return "";
}

uint CATLControlsModel::GetPlatformCount()
{
	return m_platforms.size();
}

void CATLControlsModel::AddPlatform(const string& name)
{
	string lowercaseName = CryStringUtils::toLower(name);
	if (std::find(m_platforms.begin(), m_platforms.end(), lowercaseName) == m_platforms.end())
	{
		m_platforms.push_back(lowercaseName);
	}
}

void CATLControlsModel::Clear()
{
	m_controls.clear();
	m_scopes.clear();
	m_platforms.clear();
	m_connectionGroups.clear();
	ClearDirtyFlags();
}

void CATLControlsModel::AddConnectionGroup(const string& name)
{
	if (std::find(m_connectionGroups.begin(), m_connectionGroups.end(), name) == m_connectionGroups.end())
	{
		m_connectionGroups.push_back(name);
	}
}

int CATLControlsModel::GetConnectionGroupId(const string& name)
{
	size_t size = m_connectionGroups.size();
	for (int i = 0; i < size; ++i)
	{
		if (m_connectionGroups[i].compare(name) == 0)
		{
			return i;
		}
	}
	return -1;
}

int CATLControlsModel::GetConnectionGroupCount() const
{
	return m_connectionGroups.size();
}

string CATLControlsModel::GetConnectionGroupAt(int index) const
{
	if (index < m_connectionGroups.size())
	{
		return m_connectionGroups[index];
	}
	return "";
}

void CATLControlsModel::AddListener(IATLControlModelListener* pListener)
{
	stl::push_back_unique(m_listeners, pListener);
}

void CATLControlsModel::RemoveListener(IATLControlModelListener* pListener)
{
	stl::find_and_erase(m_listeners, pListener);
}

void CATLControlsModel::OnControlAdded(CATLControl* pControl)
{
	if (!m_bSuppressMessages)
	{
		for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
		{
			(*iter)->OnControlAdded(pControl);
		}
		m_bControlTypeModified[pControl->GetType()] = true;
	}
}

void CATLControlsModel::OnControlRemoved(CATLControl* pControl)
{
	if (!m_bSuppressMessages)
	{
		for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
		{
			(*iter)->OnControlRemoved(pControl);
		}
		m_bControlTypeModified[pControl->GetType()] = true;
	}
}

void CATLControlsModel::OnConnectionAdded(CATLControl* pControl, IAudioSystemItem* pMiddlewareControl)
{
	if (!m_bSuppressMessages)
	{
		for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
		{
			(*iter)->OnConnectionAdded(pControl, pMiddlewareControl);
		}
	}
}

void CATLControlsModel::OnConnectionRemoved(CATLControl* pControl, IAudioSystemItem* pMiddlewareControl)
{
	if (!m_bSuppressMessages)
	{
		for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
		{
			(*iter)->OnConnectionRemoved(pControl, pMiddlewareControl);
		}
	}
}

void CATLControlsModel::OnControlModified(CATLControl* pControl)
{
	if (!m_bSuppressMessages)
	{
		for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
		{
			(*iter)->OnControlModified(pControl);
		}
		m_bControlTypeModified[pControl->GetType()] = true;
	}
}

void CATLControlsModel::SetSuppressMessages(bool bSuppressMessages)
{
	m_bSuppressMessages = bSuppressMessages;
}

bool CATLControlsModel::IsDirty()
{
	for (int i = 0; i < eACEControlType_NumTypes; ++i)
	{
		if (m_bControlTypeModified[i])
		{
			return true;
		}
	}
	return false;
}

bool CATLControlsModel::IsTypeDirty(EACEControlType eType)
{
	if (eType != eACEControlType_NumTypes)
	{
		return m_bControlTypeModified[eType];
	}
	return true;
}

void CATLControlsModel::ClearDirtyFlags()
{
	for (int i = 0; i < eACEControlType_NumTypes; ++i)
	{
		m_bControlTypeModified[i] = false;
	}
}

CATLControl* CATLControlsModel::FindControl(const string& sControlName, EACEControlType eType, const string& sScope, CATLControl* pParent) const
{
	if (pParent)
	{
		const size_t size = pParent->ChildCount();
		for (size_t i = 0; i < size; ++i)
		{
			CATLControl* pControl = pParent->GetChild(i);
			if (pControl &&
			    pControl->GetName() == sControlName &&
			    pControl->GetType() == eType &&
			    pControl->GetScope() == sScope)
			{
				return pControl;
			}
		}
	}
	else
	{
		const size_t size = m_controls.size();
		for (size_t i = 0; i < size; ++i)
		{
			CATLControl* pControl = m_controls[i].get();
			if (pControl &&
			    pControl->GetName() == sControlName &&
			    pControl->GetType() == eType &&
			    pControl->GetScope() == sScope)
			{
				return pControl;
			}
		}
	}
	return nullptr;
}

std::shared_ptr<CATLControl> CATLControlsModel::TakeControl(CID nID)
{
	const size_t size = m_controls.size();
	for (size_t i = 0; i < size; ++i)
	{
		if (m_controls[i]->GetId() == nID)
		{
			std::shared_ptr<CATLControl> pControl = m_controls[i];
			RemoveControl(nID);
			return pControl;
		}
	}
	return nullptr;
}

void CATLControlsModel::InsertControl(std::shared_ptr<CATLControl> pControl)
{
	if (pControl)
	{
		m_controls.push_back(pControl);

		CATLControl* pParent = pControl->GetParent();
		if (pParent)
		{
			pParent->AddChild(pControl.get());
		}

		OnControlAdded(pControl.get());
	}
}

void CATLControlsModel::ClearAllConnections()
{
	const size_t size = m_controls.size();
	for (size_t i = 0; i < size; ++i)
	{
		CATLControl* pControl = m_controls[i].get();
		if (pControl)
		{
			pControl->ClearConnections();
		}
	}
}

void CATLControlsModel::ReloadAllConnections()
{
	const size_t size = m_controls.size();
	for (size_t i = 0; i < size; ++i)
	{
		CATLControl* pControl = m_controls[i].get();
		if (pControl)
		{
			pControl->ReloadConnections();
		}
	}
}

}
