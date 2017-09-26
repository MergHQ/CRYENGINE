// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryString/CryString.h>
#include <IAudioConnection.h>
#include <ACETypes.h>
#include <CrySystem/XML/IXml.h>
#include <CrySerialization/Forward.h>

namespace ACE
{
class CAudioAssetsManager;
class IAudioSystemItem;

struct SRawConnectionData
{
	SRawConnectionData(XmlNodeRef node, bool const bIsValid)
		: xmlNode(node)
		, bValid(bIsValid) {}

	XmlNodeRef xmlNode;
	bool       bValid; // indicates if the connection is valid for the currently loaded middle-ware
};

using XMLNodeList = std::vector<SRawConnectionData>;

class CAudioAsset
{
public:

	CAudioAsset(string const& name) : m_name(name) {}

	virtual EItemType GetType() const { return EItemType::Invalid; }

	CAudioAsset*      GetParent() const { return m_pParent; }
	void              SetParent(CAudioAsset* pParent);

	size_t            ChildCount() const { return m_children.size(); }
	CAudioAsset*      GetChild(size_t const index) const { return m_children[index]; }
	void              AddChild(CAudioAsset* pChildControl);
	void              RemoveChild(CAudioAsset* pChildControl);

	string            GetName() const { return m_name; }
	virtual void      SetName(string const& name) { m_name = name; }

	virtual bool      IsModified() const { return m_bModified; }
	virtual void      SetModified(bool const bModified, bool const bForce = false);

	bool              HasPlaceholderConnection() const { return m_bHasPlaceholderConnection; }
	void              SetHasPlaceholderConnection(bool const bHasPlaceholder) { m_bHasPlaceholderConnection = bHasPlaceholder; }
	
	bool              HasConnection() const { return m_bHasConnection; }
	void              SetHasConnection(bool const bHasConnection) { m_bHasConnection = bHasConnection; }
	
	bool              HasControl() const { return m_bHasControl; }
	void              SetHasControl(bool const bHasControl) { m_bHasControl = bHasControl; }

protected:

	CAudioAsset*              m_pParent = nullptr;
	std::vector<CAudioAsset*> m_children;
	string                    m_name;
	bool                      m_bModified = false;
	bool                      m_bHasPlaceholderConnection = false;
	bool                      m_bHasConnection = false;
	bool                      m_bHasControl = false;
};

class CAudioControl final : public CAudioAsset
{
	friend class CAudioControlsLoader;
	friend class CAudioControlsWriter;
	friend class CUndoControlModified;

public:

	CAudioControl() = default;
	CAudioControl(string const& controlName, CID const id, EItemType const type);
	~CAudioControl();

	CID           GetId() const { return m_id; }
	EItemType     GetType() const override { return m_type; }

	virtual void  SetName(string const& name) override;

	Scope         GetScope() const { return m_scope; }
	void          SetScope(Scope const scope);

	bool          IsAutoLoad() const { return m_bAutoLoad; }
	void          SetAutoLoad(bool const bAutoLoad);

	float         GetRadius() const { return m_radius; }
	void          SetRadius(float const radius) { m_radius = radius; }

	float         GetOcclusionFadeOutDistance() const { return m_occlusionFadeOutDistance; }
	void          SetOcclusionFadeOutDistance(float const fadeOutArea);

	size_t        GetConnectionCount() const { return m_connectedControls.size(); }
	void          AddConnection(ConnectionPtr pConnection);
	void          RemoveConnection(ConnectionPtr pConnection);
	void          RemoveConnection(IAudioSystemItem* pAudioSystemControl);
	void          ClearConnections();
	ConnectionPtr GetConnectionAt(int const index) const;
	ConnectionPtr GetConnection(CID const id) const;
	ConnectionPtr GetConnection(IAudioSystemItem* pAudioSystemControl) const;
	void          ReloadConnections();
	void          LoadConnectionFromXML(XmlNodeRef xmlNode, int const platformIndex = -1);

	void          MatchRadiusToAttenuation();
	bool          IsMatchRadiusToAttenuationEnabled() const { return m_bMatchRadiusAndAttenuation; }
	void          SetMatchRadiusToAttenuationEnabled(bool const bEnabled) { m_bMatchRadiusAndAttenuation = bEnabled; }

	void          Serialize(Serialization::IArchive& ar);

private:

	void SignalControlAboutToBeModified();
	void SignalControlModified();
	void SignalConnectionAdded(IAudioSystemItem* pMiddlewareControl);
	void SignalConnectionRemoved(IAudioSystemItem* pMiddlewareControl);
	void SignalConnectionModified();

	CID                        m_id = ACE_INVALID_ID;
	EItemType                  m_type = EItemType::Trigger;
	Scope                      m_scope = 0;
	std::vector<ConnectionPtr> m_connectedControls;
	float                      m_radius = 0.0f;
	float                      m_occlusionFadeOutDistance = 0.0f;
	bool                       m_bAutoLoad = true;
	bool                       m_bMatchRadiusAndAttenuation = true;

	// All the raw connection nodes. Used for reloading the data when switching middleware.
	void         AddRawXMLConnection(XmlNodeRef xmlNode, bool const bValid, int const platformIndex = -1);
	XMLNodeList& GetRawXMLConnections(int const platformIndex = -1);
	std::map<int, XMLNodeList> m_connectionNodes;

	bool                       m_modifiedSignalEnabled = true;
};

class CAudioLibrary final : public CAudioAsset
{
public:

	CAudioLibrary(string const& name) : CAudioAsset(name) {}

	virtual EItemType GetType() const override { return EItemType::Library; }
	virtual void      SetModified(bool const bModified, bool const bForce = false) override;
};

class CAudioFolder final : public CAudioAsset
{
public:

	CAudioFolder(string const& name) : CAudioAsset(name) {}

	virtual EItemType GetType() const override { return EItemType::Folder; }
};
} // namespace ACE
