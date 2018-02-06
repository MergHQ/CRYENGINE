// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IEditorImpl.h>

#include "ImplControls.h"

#include <CryAudio/IAudioSystem.h>
#include <CrySystem/File/CryFile.h>
#include <CryString/CryPath.h>

namespace ACE
{
namespace Wwise
{
class CImplSettings final : public IImplSettings
{
public:

	CImplSettings();

	// IImplSettings
	virtual char const* GetAssetsPath() const override         { return m_assetsPath.c_str(); }
	virtual char const* GetProjectPath() const override        { return m_projectPath.c_str(); }
	virtual void        SetProjectPath(char const* szPath) override;
	virtual bool        IsProjectPathEditable() const override { return true; }
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
	virtual void            Reload(bool const preserveConnectionStatus = true) override;
	virtual CImplItem*      GetRoot() override { return &m_rootControl; }
	virtual CImplItem*      GetControl(CID const id) const override;
	virtual char const*     GetTypeIcon(CImplItem const* const pImplItem) const override;
	virtual string const&   GetName() const override;
	virtual string const&   GetFolderName() const override;
	virtual IImplSettings*  GetSettings() override                                                 { return &m_implSettings; }
	virtual bool            IsSystemTypeSupported(ESystemItemType const systemType) const override { return true; }
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

	// Generates the ID of the control given its full path name.
	CID GenerateID(string const& controlName, bool isLocalized, CImplItem* pParent) const;
	// Convenience function to form the full path name.
	// Controls can have the same name if they're under different parents so knowledge of the parent name is needed.
	// Localized controls live in different areas of disk so we also need to know if its localized.
	CID GenerateID(string const& fullPathName) const;

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
} // namespace Wwise
} // namespace ACE
