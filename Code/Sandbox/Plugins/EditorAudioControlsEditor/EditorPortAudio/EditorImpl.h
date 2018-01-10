// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IEditorImpl.h>

#include "ImplControls.h"

#include <CryAudio/IAudioSystem.h>
#include <CrySystem/File/CryFile.h>
#include <CryString/CryPath.h>

namespace ACE
{
namespace PortAudio
{
class CConnection;

class CImplSettings final : public IImplSettings
{
public:

	CImplSettings();

	// IImplSettings
	virtual char const* GetAssetsPath() const override              { return m_assetAndProjectPath.c_str(); }
	virtual char const* GetProjectPath() const override             { return m_assetAndProjectPath.c_str(); }
	virtual void        SetProjectPath(char const* szPath) override {}
	virtual bool        IsProjectPathEditable() const override      { return false; }
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
	virtual void            Reload(bool const preserveConnectionStatus = true) override;
	virtual CImplItem*      GetRoot() override { return &m_rootControl; }
	virtual CImplItem*      GetControl(CID const id) const override;
	virtual char const*     GetTypeIcon(CImplItem const* const pImplItem) const override;
	virtual string const&   GetName() const override;
	virtual string const&   GetFolderName() const override;
	virtual IImplSettings*  GetSettings() override { return &m_implSettings; }
	virtual bool            IsTypeCompatible(ESystemItemType const systemType, CImplItem const* const pImplItem) const override;
	virtual ESystemItemType ImplTypeToSystemType(CImplItem const* const pImplItem) const override;
	virtual ConnectionPtr   CreateConnectionToControl(ESystemItemType const controlType, CImplItem* const pImplItem) override;
	virtual ConnectionPtr   CreateConnectionFromXMLNode(XmlNodeRef pNode, ESystemItemType const controlType) override;
	virtual XmlNodeRef      CreateXMLNodeFromConnection(ConnectionPtr const pConnection, ESystemItemType const controlType) override;
	virtual void            EnableConnection(ConnectionPtr const pConnection) override;
	virtual void            DisableConnection(ConnectionPtr const pConnection) override;
	// ~IEditorImpl

private:

	void Clear();
	void CreateControlCache(CImplItem const* const pParent);
	CID  GetId(string const& name) const;

	static string const s_itemNameTag;
	static string const s_pathNameTag;
	static string const s_eventConnectionTag;
	static string const s_sampleConnectionTag;
	static string const s_connectionTypeTag;
	static string const s_panningEnabledTag;
	static string const s_attenuationEnabledTag;
	static string const s_attenuationDistMin;
	static string const s_attenuationDistMax;
	static string const s_volumeTag;
	static string const s_loopCountTag;

	using ControlsCache = std::map<CID, CImplItem*>;
	using ConnectionsMap = std::map<CID, int>;

	CImplItem           m_rootControl;
	ControlsCache       m_controlsCache; // cache of the controls stored by id for faster access
	ConnectionsMap      m_connectionsByID;
	CImplSettings       m_implSettings;
	CryAudio::SImplInfo m_implInfo;
	string              m_implName;
	string              m_implFolderName;
};
} // namespace PortAudio
} // namespace ACE
