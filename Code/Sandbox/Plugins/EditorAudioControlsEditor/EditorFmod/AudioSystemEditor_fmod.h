// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "IAudioSystemEditor.h"
#include "IAudioConnection.h"
#include "IAudioSystemItem.h"

#include <CrySystem/File/CryFile.h>
#include <CryString/CryPath.h>

namespace ACE
{
enum EFmodItemType
{
	eFmodItemType_Invalid = 0,
	eFmodItemType_Folder,
	eFmodItemType_Event,
	eFmodItemType_EventParameter,
	eFmodItemType_Snapshot,
	eFmodItemType_SnapshotParameter,
	eFmodItemType_Bank,
	eFmodItemType_Return,
	eFmodItemType_Group,
};

class CFmodFolder final : public IAudioSystemItem
{
public:

	CFmodFolder(string const& name, CID const id)
		: IAudioSystemItem(name, id, eFmodItemType_Folder)
	{}

	virtual bool     IsConnected() const override { return true; }
	virtual ItemType GetType() const override     { return eFmodItemType_Folder; }
};

class CFmodGroup final : public IAudioSystemItem
{
public:

	CFmodGroup(string const& name, CID const id)
		: IAudioSystemItem(name, id, eFmodItemType_Group)
	{}

	virtual bool     IsConnected() const override { return true; }
	virtual ItemType GetType() const override     { return eFmodItemType_Group; }
};

class CImplementationSettings_fmod final : public IImplementationSettings
{
public:

	CImplementationSettings_fmod()
		: m_projectPath(PathUtil::GetGameFolder() + CRY_NATIVE_PATH_SEPSTR AUDIO_SYSTEM_DATA_ROOT CRY_NATIVE_PATH_SEPSTR "fmod_project")
		, m_soundBanksPath(PathUtil::GetGameFolder() + CRY_NATIVE_PATH_SEPSTR AUDIO_SYSTEM_DATA_ROOT CRY_NATIVE_PATH_SEPSTR "fmod")
	{}

	virtual char const* GetSoundBanksPath() const { return m_soundBanksPath.c_str(); }
	virtual char const* GetProjectPath() const    { return m_projectPath.c_str(); }
	virtual void        SetProjectPath(char const* szPath);

	void                Serialize(Serialization::IArchive& ar)
	{
		ar(m_projectPath, "projectPath", "Project Path");
	}

private:

	string       m_projectPath;
	string const m_soundBanksPath;
};

class CAudioSystemEditor_fmod final : public IAudioSystemEditor
{
public:

	CAudioSystemEditor_fmod();
	virtual ~CAudioSystemEditor_fmod();

	// IAudioSystemEditor
	virtual void                     Reload(bool const preserveConnectionStatus = true) override;
	virtual IAudioSystemItem*        GetRoot() override { return &m_root; }
	virtual IAudioSystemItem*        GetControl(CID const id) const override;
	virtual TImplControlTypeMask     GetCompatibleTypes(EItemType const controlType) const override;
	virtual char const*              GetTypeIcon(ItemType const type) const override;
	virtual string                   GetName() const override;
	virtual IImplementationSettings* GetSettings() override { return &m_settings; }
	virtual EItemType                ImplTypeToSystemType(ItemType const itemType) const override;
	virtual ConnectionPtr            CreateConnectionToControl(EItemType const controlType, IAudioSystemItem* const pMiddlewareControl) override;
	virtual ConnectionPtr            CreateConnectionFromXMLNode(XmlNodeRef pNode, EItemType const controlType) override;
	virtual XmlNodeRef               CreateXMLNodeFromConnection(ConnectionPtr const pConnection, EItemType const controlType) override;
	virtual void                     EnableConnection(ConnectionPtr const pConnection) override;
	virtual void                     DisableConnection(ConnectionPtr const pConnection) override;
	// ~IAudioSystemEditor

private:

	IAudioSystemItem* CreateItem(EFmodItemType const type, IAudioSystemItem* const pParent, string const& name);
	CID               GetId(EFmodItemType const type, string const& name, IAudioSystemItem* const pParent) const;
	string            GetFullPathName(IAudioSystemItem const* const pItem) const;
	string            GetTypeName(EFmodItemType const type) const;

	void              ParseFolder(string const& folderPath);
	void              ParseFile(string const& filepath);

	IAudioSystemItem* GetContainer(string const& id, EFmodItemType const type);
	IAudioSystemItem* LoadContainer(XmlNodeRef const pNode, EFmodItemType const type, string const& relationshipParamName);
	IAudioSystemItem* LoadFolder(XmlNodeRef const pNode);
	IAudioSystemItem* LoadGroup(XmlNodeRef const pNode);

	IAudioSystemItem* LoadItem(XmlNodeRef const pNode, EFmodItemType const type);
	IAudioSystemItem* LoadEvent(XmlNodeRef const pNode);
	IAudioSystemItem* LoadSnapshot(XmlNodeRef const pNode);
	IAudioSystemItem* LoadReturn(XmlNodeRef const pNode);
	IAudioSystemItem* LoadParameter(XmlNodeRef const pNode, EFmodItemType const type, IAudioSystemItem& parentEvent);

	IAudioSystemItem* GetItemFromPath(string const& fullpath);
	IAudioSystemItem* CreatePlaceholderFolderPath(string const& path);

	std::map<CID, int>                  m_connectionsByID;

	IAudioSystemItem                    m_root;
	std::vector<IAudioSystemItem*>      m_items;
	std::map<string, IAudioSystemItem*> m_containerIdMap;
	CImplementationSettings_fmod        m_settings;
};
} // namespace ACE
