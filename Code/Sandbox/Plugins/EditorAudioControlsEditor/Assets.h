// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IConnection.h>
#include <SharedData.h>

#include <CryString/CryString.h>
#include <CrySystem/XML/IXml.h>
#include <CrySerialization/Forward.h>

namespace ACE
{
struct IImplItem;

enum class EAssetFlags
{
	None                     = 0,
	IsDefaultControl         = BIT(0),
	IsInternalControl        = BIT(1),
	IsModified               = BIT(2),
	HasPlaceholderConnection = BIT(3),
	HasConnection            = BIT(4),
	HasControl               = BIT(5),
};
CRY_CREATE_ENUM_FLAG_OPERATORS(EAssetFlags);

class CAsset
{
public:

	EAssetType   GetType() const { return m_type; }
	char const*  GetTypeName() const;

	CAsset*      GetParent() const { return m_pParent; }
	void         SetParent(CAsset* const pParent);

	size_t       ChildCount() const { return m_children.size(); }
	CAsset*      GetChild(size_t const index) const;
	void         AddChild(CAsset* const pChildControl);
	void         RemoveChild(CAsset const* const pChildControl);

	string       GetName() const { return m_name; }
	virtual void SetName(string const& name);

	void         UpdateNameOnMove(CAsset* const pParent);

	string       GetDescription() const { return m_description; }
	virtual void SetDescription(string const& description);

	bool         IsDefaultControl() const { return (m_flags& EAssetFlags::IsDefaultControl) != 0; }
	void         SetDefaultControl(bool const isDefaultControl);

	bool         IsInternalControl() const { return (m_flags& EAssetFlags::IsInternalControl) != 0; }
	void         SetInternalControl(bool const isInternal);

	virtual bool IsModified() const { return (m_flags& EAssetFlags::IsModified) != 0; }
	virtual void SetModified(bool const isModified, bool const isForced = false);

	bool         HasPlaceholderConnection() const { return (m_flags& EAssetFlags::HasPlaceholderConnection) != 0; }
	void         SetHasPlaceholderConnection(bool const hasPlaceholder);

	bool         HasConnection() const { return (m_flags& EAssetFlags::HasConnection) != 0; }
	void         SetHasConnection(bool const hasConnection);

	bool         HasControl() const { return (m_flags& EAssetFlags::HasControl) != 0; }
	void         SetHasControl(bool const hasControl);

	string       GetFullHierarchyName() const;

	bool         HasDefaultControlChildren(std::vector<string>& names) const;

	virtual void Serialize(Serialization::IArchive& ar);

protected:

	explicit CAsset(string const& name, EAssetType const type);

	CAsset() = delete;

	CAsset*              m_pParent = nullptr;
	std::vector<CAsset*> m_children;
	string               m_name;
	string               m_description = "";
	EAssetType const     m_type;
	EAssetFlags          m_flags;
};

class CControl final : public CAsset
{
public:

	explicit CControl(string const& name, ControlId const id, EAssetType const type);
	~CControl();

	CControl() = delete;

	// CSystemAsset
	virtual void SetName(string const& name) override;
	virtual void SetDescription(string const& description) override;
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CSystemAsset

	ControlId                     GetId() const    { return m_id; }

	Scope                         GetScope() const { return m_scope; }
	void                          SetScope(Scope const scope);

	bool                          IsAutoLoad() const { return m_isAutoLoad; }
	void                          SetAutoLoad(bool const isAutoLoad);

	float                         GetRadius() const { return m_radius; }
	void                          SetRadius(float const radius);

	std::vector<ControlId> const& GetSelectedConnections() const                                       { return m_selectedConnectionIds; }
	void                          SetSelectedConnections(std::vector<ControlId> selectedConnectionIds) { m_selectedConnectionIds = selectedConnectionIds; }

	size_t                        GetConnectionCount() const                                           { return m_connections.size(); }
	void                          AddConnection(ConnectionPtr const pConnection);
	void                          RemoveConnection(ConnectionPtr const pConnection);
	void                          RemoveConnection(IImplItem* const pIImplItem);
	void                          ClearConnections();
	ConnectionPtr                 GetConnectionAt(size_t const index) const;
	ConnectionPtr                 GetConnection(ControlId const id) const;
	ConnectionPtr                 GetConnection(IImplItem const* const pIImplItem) const;
	void                          BackupAndClearConnections();
	void                          ReloadConnections();
	void                          LoadConnectionFromXML(XmlNodeRef const xmlNode, int const platformIndex = -1);

	void                          MatchRadiusToAttenuation();

private:

	void SignalControlAboutToBeModified();
	void SignalControlModified();
	void SignalConnectionAdded(IImplItem* const pIImplItem);
	void SignalConnectionRemoved(IImplItem* const pIImplItem);
	void SignalConnectionModified();

	ControlId const            m_id;
	Scope                      m_scope = 0;
	std::vector<ConnectionPtr> m_connections;
	float                      m_radius = 0.0f;
	bool                       m_isAutoLoad = true;

	using XMLNodeList = std::vector<XmlNodeRef>;
	std::map<int, XMLNodeList> m_rawConnections;
	std::vector<ControlId>     m_selectedConnectionIds;
};

enum class EPakStatus
{
	None   = 0,
	InPak  = BIT(0),
	OnDisk = BIT(1),
};
CRY_CREATE_ENUM_FLAG_OPERATORS(EPakStatus);

class CLibrary final : public CAsset
{
public:

	explicit CLibrary(string const& name);

	CLibrary() = delete;

	virtual void SetModified(bool const isModified, bool const isForced = false) override;

	void         SetPakStatus(EPakStatus const pakStatus, bool const exists);
	bool         IsExactPakStatus(EPakStatus const pakStatus) const { return((m_pakStatusFlags & pakStatus) == pakStatus); }
	bool         IsAnyPakStatus(EPakStatus const pakStatus) const   { return((m_pakStatusFlags & pakStatus) != 0); }

private:

	EPakStatus m_pakStatusFlags;
};

class CFolder final : public CAsset
{
public:

	explicit CFolder(string const& name);

	CFolder() = delete;
};
} // namespace ACE
