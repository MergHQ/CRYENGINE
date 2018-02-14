// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ImplConnection.h>
#include <SystemTypes.h>

#include <CryString/CryString.h>
#include <CrySystem/XML/IXml.h>
#include <CrySerialization/Forward.h>

namespace ACE
{
struct IImplItem;

struct SRawConnectionData
{
	SRawConnectionData(XmlNodeRef const node, bool const isValid_)
		: xmlNode(node)
		, isValid(isValid_) {}

	XmlNodeRef xmlNode;
	bool       isValid; // indicates if the connection is valid for the currently loaded middle-ware
};

using XMLNodeList = std::vector<SRawConnectionData>;

enum class ESystemAssetFlags
{
	None                     = 0,
	IsDefaultControl         = BIT(0),
	IsInternalControl        = BIT(1),
	IsModified               = BIT(2),
	HasPlaceholderConnection = BIT(3),
	HasConnection            = BIT(4),
	HasControl               = BIT(5),
};
CRY_CREATE_ENUM_FLAG_OPERATORS(ESystemAssetFlags);

class CSystemAsset
{
public:

	CSystemAsset(string const& name, ESystemItemType const type);

	ESystemItemType GetType() const { return m_type; }
	char const*     GetTypeName() const;

	CSystemAsset*   GetParent() const { return m_pParent; }
	void            SetParent(CSystemAsset* const pParent);

	size_t          ChildCount() const { return m_children.size(); }
	CSystemAsset*   GetChild(size_t const index) const;
	void            AddChild(CSystemAsset* const pChildControl);
	void            RemoveChild(CSystemAsset const* const pChildControl);

	string          GetName() const { return m_name; }
	virtual void    SetName(string const& name);

	void            UpdateNameOnMove(CSystemAsset* const pParent);

	string          GetDescription() const { return m_description; }
	virtual void    SetDescription(string const& description);

	bool            IsDefaultControl() const { return (m_flags& ESystemAssetFlags::IsDefaultControl) != 0; }
	void            SetDefaultControl(bool const isDefaultControl);

	bool            IsInternalControl() const { return (m_flags& ESystemAssetFlags::IsInternalControl) != 0; }
	void            SetInternalControl(bool const isInternal);

	virtual bool    IsModified() const { return (m_flags& ESystemAssetFlags::IsModified) != 0; }
	virtual void    SetModified(bool const isModified, bool const isForced = false);

	bool            HasPlaceholderConnection() const { return (m_flags& ESystemAssetFlags::HasPlaceholderConnection) != 0; }
	void            SetHasPlaceholderConnection(bool const hasPlaceholder);

	bool            HasConnection() const { return (m_flags& ESystemAssetFlags::HasConnection) != 0; }
	void            SetHasConnection(bool const hasConnection);

	bool            HasControl() const { return (m_flags& ESystemAssetFlags::HasControl) != 0; }
	void            SetHasControl(bool const hasControl);

	string          GetFullHierarchyName() const;

	bool            HasDefaultControlChildren(std::vector<string>& names) const;

	virtual void    Serialize(Serialization::IArchive& ar);

protected:

	CSystemAsset*              m_pParent = nullptr;
	std::vector<CSystemAsset*> m_children;
	string                     m_name;
	string                     m_description = "";
	ESystemItemType const      m_type;
	ESystemAssetFlags          m_flags;
};

class CSystemControl final : public CSystemAsset
{
public:

	CSystemControl() = default;
	CSystemControl(string const& name, CID const id, ESystemItemType const type);
	~CSystemControl();

	CID                     GetId() const { return m_id; }

	virtual void            SetName(string const& name) override;

	virtual void            SetDescription(string const& description) override;

	Scope                   GetScope() const { return m_scope; }
	void                    SetScope(Scope const scope);

	bool                    IsAutoLoad() const { return m_isAutoLoad; }
	void                    SetAutoLoad(bool const isAutoLoad);

	float                   GetRadius() const { return m_radius; }
	void                    SetRadius(float const radius);

	std::vector<CID> const& GetSelectedConnections() const                                 { return m_selectedConnectionIds; }
	void                    SetSelectedConnections(std::vector<CID> selectedConnectionIds) { m_selectedConnectionIds = selectedConnectionIds; }

	size_t                  GetConnectionCount() const                                     { return m_connectedControls.size(); }
	void                    AddConnection(ConnectionPtr const pConnection);
	void                    RemoveConnection(ConnectionPtr const pConnection);
	void                    RemoveConnection(IImplItem* const pImplItem);
	void                    ClearConnections();
	ConnectionPtr           GetConnectionAt(size_t const index) const;
	ConnectionPtr           GetConnection(CID const id) const;
	ConnectionPtr           GetConnection(IImplItem const* const pImplItem) const;
	void                    ReloadConnections();
	void                    LoadConnectionFromXML(XmlNodeRef const xmlNode, int const platformIndex = -1);

	void                    MatchRadiusToAttenuation();

	virtual void            Serialize(Serialization::IArchive& ar) override;

	// All the raw connection nodes. Used for reloading the data when switching middleware.
	void AddRawXMLConnection(XmlNodeRef const xmlNode, bool const isValid, int const platformIndex = -1);

private:

	void SignalControlAboutToBeModified();
	void SignalControlModified();
	void SignalConnectionAdded(IImplItem* const pImplItem);
	void SignalConnectionRemoved(IImplItem* const pImplItem);
	void SignalConnectionModified();

	CID                        m_id = ACE_INVALID_ID;
	Scope                      m_scope = 0;
	std::vector<ConnectionPtr> m_connectedControls;
	float                      m_radius = 0.0f;
	bool                       m_isAutoLoad = true;

	std::map<int, XMLNodeList> m_connectionNodes;
	std::vector<CID>           m_selectedConnectionIds;
};

class CSystemLibrary final : public CSystemAsset
{
public:

	CSystemLibrary(string const& name);

	virtual void SetModified(bool const isModified, bool const isForced = false) override;
};

class CSystemFolder final : public CSystemAsset
{
public:

	CSystemFolder(string const& name);
};
} // namespace ACE
