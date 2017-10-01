// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryString/CryString.h>
#include <IAudioConnection.h>
#include <ACETypes.h>
#include <CrySystem/XML/IXml.h>
#include <CrySerialization/Forward.h>

namespace ACE
{
class IAudioSystemItem;

struct SRawConnectionData
{
	SRawConnectionData(XmlNodeRef const node, bool const isValid_)
		: xmlNode(node)
		, isValid(isValid_) {}

	XmlNodeRef xmlNode;
	bool       isValid; // indicates if the connection is valid for the currently loaded middle-ware
};

using XMLNodeList = std::vector<SRawConnectionData>;

class CAudioAsset
{
public:

	CAudioAsset(string const& name) : m_name(name) {}

	virtual EItemType GetType() const { return EItemType::Invalid; }

	CAudioAsset*      GetParent() const { return m_pParent; }
	void              SetParent(CAudioAsset* const pParent);

	size_t            ChildCount() const { return m_children.size(); }
	CAudioAsset*      GetChild(size_t const index) const { return m_children[index]; }
	void              AddChild(CAudioAsset* const pChildControl);
	void              RemoveChild(CAudioAsset const* const pChildControl);

	string            GetName() const { return m_name; }
	virtual void      SetName(string const& name) { m_name = name; }

	bool              IsHiddenDefault() const { return m_isHiddenDefault; }
	void              SetHiddenDefault(bool const isHiddenDefault);

	virtual bool      IsModified() const { return m_isModified; }
	virtual void      SetModified(bool const isModified, bool const isForced = false);

	bool              HasPlaceholderConnection() const { return m_hasPlaceholderConnection; }
	void              SetHasPlaceholderConnection(bool const hasPlaceholder) { m_hasPlaceholderConnection = hasPlaceholder; }
	
	bool              HasConnection() const { return m_hasConnection; }
	void              SetHasConnection(bool const hasConnection) { m_hasConnection = hasConnection; }
	
	bool              HasControl() const { return m_hasControl; }
	void              SetHasControl(bool const hasControl) { m_hasControl = hasControl; }

protected:

	CAudioAsset*              m_pParent = nullptr;
	std::vector<CAudioAsset*> m_children;
	string                    m_name;
	bool                      m_isHiddenDefault = false;
	bool                      m_isModified = false;
	bool                      m_hasPlaceholderConnection = false;
	bool                      m_hasConnection = false;
	bool                      m_hasControl = false;
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

	bool          IsAutoLoad() const { return m_isAutoLoad; }
	void          SetAutoLoad(bool const isAutoLoad);

	float         GetRadius() const { return m_radius; }
	void          SetRadius(float const radius) { m_radius = radius; }

	float         GetOcclusionFadeOutDistance() const { return m_occlusionFadeOutDistance; }
	void          SetOcclusionFadeOutDistance(float const fadeOutArea);

	size_t        GetConnectionCount() const { return m_connectedControls.size(); }
	void          AddConnection(ConnectionPtr const pConnection);
	void          RemoveConnection(ConnectionPtr const pConnection);
	void          RemoveConnection(IAudioSystemItem* const pAudioSystemControl);
	void          ClearConnections();
	ConnectionPtr GetConnectionAt(int const index) const;
	ConnectionPtr GetConnection(CID const id) const;
	ConnectionPtr GetConnection(IAudioSystemItem const* const pAudioSystemControl) const;
	void          ReloadConnections();
	void          LoadConnectionFromXML(XmlNodeRef const xmlNode, int const platformIndex = -1);

	void          MatchRadiusToAttenuation();
	bool          IsMatchRadiusToAttenuationEnabled() const { return m_matchRadiusAndAttenuation; }
	void          SetMatchRadiusToAttenuationEnabled(bool const isEnabled) { m_matchRadiusAndAttenuation = isEnabled; }

	void          Serialize(Serialization::IArchive& ar);

private:

	void SignalControlAboutToBeModified();
	void SignalControlModified();
	void SignalConnectionAdded(IAudioSystemItem* const pMiddlewareControl);
	void SignalConnectionRemoved(IAudioSystemItem* const pMiddlewareControl);
	void SignalConnectionModified();

	CID                        m_id = ACE_INVALID_ID;
	EItemType                  m_type = EItemType::Trigger;
	Scope                      m_scope = 0;
	std::vector<ConnectionPtr> m_connectedControls;
	float                      m_radius = 0.0f;
	float                      m_occlusionFadeOutDistance = 0.0f;
	bool                       m_isAutoLoad = true;
	bool                       m_matchRadiusAndAttenuation = true;

	// All the raw connection nodes. Used for reloading the data when switching middleware.
	void         AddRawXMLConnection(XmlNodeRef const xmlNode, bool const isValid, int const platformIndex = -1);
	XMLNodeList& GetRawXMLConnections(int const platformIndex = -1);
	std::map<int, XMLNodeList> m_connectionNodes;

	bool                       m_modifiedSignalEnabled = true;
};

class CAudioLibrary final : public CAudioAsset
{
public:

	CAudioLibrary(string const& name) : CAudioAsset(name) {}

	virtual EItemType GetType() const override { return EItemType::Library; }
	virtual void      SetModified(bool const isModified, bool const isForced = false) override;
};

class CAudioFolder final : public CAudioAsset
{
public:

	CAudioFolder(string const& name) : CAudioAsset(name) {}

	virtual EItemType GetType() const override { return EItemType::Folder; }
};
} // namespace ACE
