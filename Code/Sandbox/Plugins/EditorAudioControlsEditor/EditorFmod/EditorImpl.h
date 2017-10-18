// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ImplControl.h"	// Remove when ProjectLoader and ImplControls are done.

#include <IEditorImpl.h>
#include <ImplItem.h>

#include <CrySystem/File/CryFile.h>
#include <CryString/CryPath.h>

namespace ACE
{
namespace Fmod
{
class CImplSettings final : public IImplSettings
{
public:

	CImplSettings()
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

class CEditorImpl final : public IEditorImpl
{
public:

	CEditorImpl();
	virtual ~CEditorImpl();

	// IAudioSystemEditor
	virtual void                 Reload(bool const preserveConnectionStatus = true) override;
	virtual CImplItem*           GetRoot() override { return &m_root; }
	virtual CImplItem*           GetControl(CID const id) const override;
	virtual TImplControlTypeMask GetCompatibleTypes(ESystemItemType const systemType) const override;
	virtual char const*          GetTypeIcon(CImplItem const* const pImplItem) const override;
	virtual string               GetName() const override;
	virtual IImplSettings*       GetSettings() override { return &m_implSettings; }
	virtual ESystemItemType      ImplTypeToSystemType(CImplItem const* const pImplItem) const override;
	virtual ConnectionPtr        CreateConnectionToControl(ESystemItemType const controlType, CImplItem* const pImplItem) override;
	virtual ConnectionPtr        CreateConnectionFromXMLNode(XmlNodeRef pNode, ESystemItemType const controlType) override;
	virtual XmlNodeRef           CreateXMLNodeFromConnection(ConnectionPtr const pConnection, ESystemItemType const controlType) override;
	virtual void                 EnableConnection(ConnectionPtr const pConnection) override;
	virtual void                 DisableConnection(ConnectionPtr const pConnection) override;
	// ~IAudioSystemEditor

private:

	CImplItem* CreateItem(EImpltemType const type, CImplItem* const pParent, string const& name);
	CID        GetId(EImpltemType const type, string const& name, CImplItem* const pParent) const;
	string     GetFullPathName(CImplItem const* const pImplItem) const;
	string     GetTypeName(EImpltemType const type) const;

	void       ParseFolder(string const& folderPath);
	void       ParseFile(string const& filepath);

	CImplItem* GetContainer(string const& id, EImpltemType const type);
	CImplItem* LoadContainer(XmlNodeRef const pNode, EImpltemType const type, string const& relationshipParamName);
	CImplItem* LoadFolder(XmlNodeRef const pNode);
	CImplItem* LoadGroup(XmlNodeRef const pNode);

	CImplItem* LoadItem(XmlNodeRef const pNode, EImpltemType const type);
	CImplItem* LoadEvent(XmlNodeRef const pNode);
	CImplItem* LoadSnapshot(XmlNodeRef const pNode);
	CImplItem* LoadReturn(XmlNodeRef const pNode);
	CImplItem* LoadParameter(XmlNodeRef const pNode, EImpltemType const type, CImplItem& parentEvent);

	CImplItem* GetItemFromPath(string const& fullpath);
	CImplItem* CreatePlaceholderFolderPath(string const& path);

	std::map<CID, int>           m_connectionsByID;

	CImplItem                    m_root;
	std::vector<CImplItem*>      m_items;
	std::map<string, CImplItem*> m_containerIdMap;
	CImplSettings                m_implSettings;
};
} // namespace Fmod
} // namespace ACE
