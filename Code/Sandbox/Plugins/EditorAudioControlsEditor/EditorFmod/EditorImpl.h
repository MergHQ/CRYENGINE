// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IEditorImpl.h>

#include "Item.h"

#include <CryAudio/IAudioSystem.h>
#include <CrySystem/File/CryFile.h>

namespace ACE
{
namespace Fmod
{
static Platforms s_platforms;

class CSettings final : public IImplSettings
{
public:

	CSettings();

	// IImplSettings
	virtual char const* GetAssetsPath() const override    { return m_assetsPath.c_str(); }
	virtual char const* GetProjectPath() const override   { return m_projectPath.c_str(); }
	virtual void        SetProjectPath(char const* szPath) override;
	virtual bool        SupportsProjects() const override { return true; }
	// ~IImplSettings

	void Serialize(Serialization::IArchive& ar);

private:

	string       m_projectPath;
	string const m_assetsPath;
};

class CEditorImpl final : public IEditorImpl
{
public:

	CEditorImpl();
	virtual ~CEditorImpl() override;

	// IEditorImpl
	virtual void           Reload(bool const preserveConnectionStatus = true) override;
	virtual void           SetPlatforms(Platforms const& platforms) override { s_platforms = platforms; }
	virtual IImplItem*     GetRoot() override                                { return &m_rootItem; }
	virtual IImplItem*     GetItem(ControlId const id) const override;
	virtual char const*    GetTypeIcon(IImplItem const* const pIImplItem) const override;
	virtual string const&  GetName() const override;
	virtual string const&  GetFolderName() const override;
	virtual IImplSettings* GetSettings() override                                           { return &m_settings; }
	virtual bool           IsSystemTypeSupported(EAssetType const assetType) const override { return true; }
	virtual bool           IsTypeCompatible(EAssetType const assetType, IImplItem const* const pIImplItem) const override;
	virtual EAssetType     ImplTypeToAssetType(IImplItem const* const pIImplItem) const override;
	virtual ConnectionPtr  CreateConnectionToControl(EAssetType const assetType, IImplItem const* const pIImplItem) override;
	virtual ConnectionPtr  CreateConnectionFromXMLNode(XmlNodeRef pNode, EAssetType const assetType) override;
	virtual XmlNodeRef     CreateXMLNodeFromConnection(ConnectionPtr const pConnection, EAssetType const assetType) override;
	virtual void           EnableConnection(ConnectionPtr const pConnection) override;
	virtual void           DisableConnection(ConnectionPtr const pConnection) override;
	// ~IEditorImpl

private:

	void   Clear();
	CItem* CreateItem(string const& name, EItemType const type, CItem* const pParent);
	CItem* GetItemFromPath(string const& fullpath);
	CItem* CreatePlaceholderFolderPath(string const& path);

	using ConnectionIds = std::map<ControlId, int>;

	CItem               m_rootItem { "", s_aceInvalidId, EItemType::None };
	ItemCache           m_itemCache; // cache of the items stored by id for faster access
	ConnectionIds       m_connectionsByID;
	CSettings           m_settings;
	CryAudio::SImplInfo m_implInfo;
	string              m_implName;
	string              m_implFolderName;
};
} // namespace Fmod
} // namespace ACE
