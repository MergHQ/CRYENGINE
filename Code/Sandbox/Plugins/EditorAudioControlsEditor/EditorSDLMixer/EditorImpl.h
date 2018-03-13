// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IEditorImpl.h>

#include "Item.h"

#include <CryAudio/IAudioSystem.h>
#include <CrySystem/File/CryFile.h>

namespace ACE
{
namespace SDLMixer
{
class CEventConnection;

class CSettings final : public IImplSettings
{
public:

	CSettings();

	// IImplSettings
	virtual char const* GetAssetsPath() const override              { return m_assetAndProjectPath.c_str(); }
	virtual char const* GetProjectPath() const override             { return m_assetAndProjectPath.c_str(); }
	virtual void        SetProjectPath(char const* szPath) override {}
	virtual bool        SupportsProjects() const override           { return false; }
	// ~IImplSettings

private:

	string const m_assetAndProjectPath;
};

class CEditorImpl final : public IEditorImpl
{
public:

	CEditorImpl();
	virtual ~CEditorImpl() override;

	// IEditorImpl
	virtual void           Reload(bool const preserveConnectionStatus = true) override;
	virtual void           SetPlatforms(Platforms const& platforms) override {}
	virtual IImplItem*     GetRoot() override                                { return &m_rootItem; }
	virtual IImplItem*     GetItem(ControlId const id) const override;
	virtual char const*    GetTypeIcon(IImplItem const* const pIImplItem) const override;
	virtual string const&  GetName() const override;
	virtual string const&  GetFolderName() const override;
	virtual IImplSettings* GetSettings() override { return &m_settings; }
	virtual bool           IsSystemTypeSupported(EAssetType const assetType) const override;
	virtual bool           IsTypeCompatible(EAssetType const assetType, IImplItem const* const pIImplItem) const override;
	virtual EAssetType     ImplTypeToAssetType(IImplItem const* const pIImplItem) const override;
	virtual ConnectionPtr  CreateConnectionToControl(EAssetType const assetType, IImplItem const* const pIImplItem) override;
	virtual ConnectionPtr  CreateConnectionFromXMLNode(XmlNodeRef pNode, EAssetType const assetType) override;
	virtual XmlNodeRef     CreateXMLNodeFromConnection(ConnectionPtr const pConnection, EAssetType const assetType) override;
	virtual void           EnableConnection(ConnectionPtr const pConnection) override;
	virtual void           DisableConnection(ConnectionPtr const pConnection) override;
	// ~IEditorImpl

private:

	void      Clear();
	void      CreateItemCache(CItem const* const pParent);
	ControlId GetId(string const& name) const;

	using ItemCache = std::map<ControlId, CItem*>;
	using ConnectionIds = std::map<ControlId, int>;

	CItem               m_rootItem { "", s_aceInvalidId, EItemType::None };
	ItemCache           m_itemCache; // cache of the items stored by id for faster access
	ConnectionIds       m_connectionsByID;
	CSettings           m_settings;
	CryAudio::SImplInfo m_implInfo;
	string              m_implName;
	string              m_implFolderName;
};
} // namespace SDLMixer
} // namespace ACE
