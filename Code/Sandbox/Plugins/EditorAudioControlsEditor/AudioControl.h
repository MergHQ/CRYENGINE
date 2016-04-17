// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryString/CryString.h>
#include <IAudioConnection.h>
#include <ACETypes.h>
#include <CrySystem/XML/IXml.h>

namespace ACE
{
extern const string g_sDefaultGroup;

class CATLControlsModel;

struct SRawConnectionData
{
	SRawConnectionData(XmlNodeRef node, bool bIsValid)
		: xmlNode(node)
		, bValid(bIsValid) {}
	XmlNodeRef xmlNode;

	// indicates if the connection is valid for the currently loaded middle-ware
	bool bValid;
};

typedef std::vector<SRawConnectionData> TXMLNodeList;
typedef std::map<string, TXMLNodeList>  TConnectionPerGroup;

class CATLControl
{
	friend class CAudioControlsLoader;
	friend class CAudioControlsWriter;
	friend class CUndoControlModified;

public:
	CATLControl();
	CATLControl(const string& sControlName, CID nID, EACEControlType eType, CATLControlsModel* pModel);
	~CATLControl();

	CID             GetId() const;

	EACEControlType GetType() const;

	string          GetName() const;
	void            SetName(const string& name);

	bool            HasScope() const;
	string          GetScope() const;
	void            SetScope(const string& sScope);

	bool            IsAutoLoad() const;
	void            SetAutoLoad(bool bAutoLoad);

	CATLControl*    GetParent() const { return m_pParent; }
	void            SetParent(CATLControl* pParent);

	size_t          ChildCount() const                      { return m_children.size(); }
	CATLControl*    GetChild(uint index) const              { return m_children[index]; }
	void            AddChild(CATLControl* pChildControl)    { m_children.push_back(pChildControl); }
	void            RemoveChild(CATLControl* pChildControl) { m_children.erase(std::remove(m_children.begin(), m_children.end(), pChildControl), m_children.end()); }

	// Controls can group connection according to a platform
	// This is used primarily for the Preload Requests
	int           GetGroupForPlatform(const string& platform) const;
	void          SetGroupForPlatform(const string& platform, int connectionGroupId);

	size_t        GetConnectionCount();
	void          AddConnection(ConnectionPtr pConnection);
	void          RemoveConnection(ConnectionPtr pConnection);
	void          RemoveConnection(IAudioSystemItem* pAudioSystemControl);
	void          ClearConnections();
	ConnectionPtr GetConnectionAt(int index);
	ConnectionPtr GetConnection(CID id, const string& group = g_sDefaultGroup);
	ConnectionPtr GetConnection(IAudioSystemItem* m_pAudioSystemControl, const string& group = g_sDefaultGroup);
	void          ReloadConnections();

	void          SignalControlAboutToBeModified();
	void          SignalControlModified();
	void          SignalConnectionAdded(IAudioSystemItem* pMiddlewareControl);
	void          SignalConnectionRemoved(IAudioSystemItem* pMiddlewareControl);

private:
	void SetId(CID id);
	void SetType(EACEControlType type);
	CID                        m_nID;
	EACEControlType            m_eType;
	string                     m_sName;
	string                     m_sScope;
	std::map<string, int>      m_groupPerPlatform;
	std::vector<ConnectionPtr> m_connectedControls;
	std::vector<CATLControl*>  m_children;
	CATLControl*               m_pParent;
	bool                       m_bAutoLoad;
	bool                       m_bModified;

	// All the raw connection nodes. Used for reloading the data when switching middleware.
	TConnectionPerGroup m_connectionNodes;
	CATLControlsModel*  m_pModel;
};

typedef std::shared_ptr<CATLControl> ATLControlPtr;
}
