// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryString/CryString.h>
#include "AudioControl.h"
#include <IAudioConnection.h>

namespace ACE
{
// available levels where the controls can be stored
struct SControlScope
{
	SControlScope() {}
	SControlScope(const string& _name, bool _bOnlyLocal) : name(_name), bOnlyLocal(_bOnlyLocal) {}
	string name;

	// if true, there is a level in the game audio
	// data that doesn't exist in the global list
	// of levels for your project
	bool bOnlyLocal;
};

struct IATLControlModelListener
{
	virtual void OnControlAdded(CATLControl* pControl)                                            {}
	virtual void OnControlModified(CATLControl* pControl)                                         {}
	virtual void OnControlRemoved(CATLControl* pControl)                                          {}
	virtual void OnConnectionAdded(CATLControl* pControl, IAudioSystemItem* pMiddlewareControl)   {}
	virtual void OnConnectionRemoved(CATLControl* pControl, IAudioSystemItem* pMiddlewareControl) {}
};

class CATLControlsModel
{
	friend class IUndoControlOperation;
	friend class CUndoControlModified;
	friend class CATLControl;

public:
	CATLControlsModel();
	~CATLControlsModel();

	void         Clear();
	CATLControl* CreateControl(const string& sControlName, EACEControlType type, CATLControl* pParent = nullptr);
	void         RemoveControl(CID id);

	CATLControl* GetControlByID(CID id) const;
	CATLControl* FindControl(const string& sControlName, EACEControlType eType, const string& sScope, CATLControl* pParent = nullptr) const;

	// Platforms
	string GetPlatformAt(uint index);
	void   AddPlatform(const string& name);
	uint   GetPlatformCount();

	// Connection Groups
	void   AddConnectionGroup(const string& name);
	int    GetConnectionGroupId(const string& name);
	int    GetConnectionGroupCount() const;
	string GetConnectionGroupAt(int index) const;

	// Scope
	void          AddScope(const string& name, bool bLocalOnly = false);
	void          ClearScopes();
	int           GetScopeCount() const;
	SControlScope GetScopeAt(int index) const;
	bool          ScopeExists(const string& name) const;

	// Helper functions
	bool   IsNameValid(const string& name, EACEControlType type, const string& scope, const CATLControl* const pParent = nullptr) const;
	string GenerateUniqueName(const string& sRootName, EACEControlType eType, const string& sScope, const CATLControl* const pParent = nullptr) const;
	void   ClearAllConnections();
	void   ReloadAllConnections();

	void   AddListener(IATLControlModelListener* pListener);
	void   RemoveListener(IATLControlModelListener* pListener);
	void   SetSuppressMessages(bool bSuppressMessages);
	bool   IsTypeDirty(EACEControlType eType);
	bool   IsDirty();
	void   ClearDirtyFlags();

private:
	void                         OnControlAdded(CATLControl* pControl);
	void                         OnControlModified(CATLControl* pControl);
	void                         OnConnectionAdded(CATLControl* pControl, IAudioSystemItem* pMiddlewareControl);
	void                         OnConnectionRemoved(CATLControl* pControl, IAudioSystemItem* pMiddlewareControl);
	void                         OnControlRemoved(CATLControl* pControl);

	CID                          GenerateUniqueId() { return m_nextId++; }

	std::shared_ptr<CATLControl> TakeControl(CID nID);
	void                         InsertControl(std::shared_ptr<CATLControl> pControl);

	static CID                                m_nextId;
	std::vector<std::shared_ptr<CATLControl>> m_controls;
	std::vector<string>                       m_platforms;
	std::vector<SControlScope>                m_scopes;
	std::vector<string>                       m_connectionGroups;

	std::vector<IATLControlModelListener*>    m_listeners;
	bool m_bSuppressMessages;
	bool m_bControlTypeModified[eACEControlType_NumTypes];
};
}
