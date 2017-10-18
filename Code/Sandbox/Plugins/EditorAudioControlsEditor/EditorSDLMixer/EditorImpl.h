// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ProjectLoader.h"

#include <IEditorImpl.h>
#include <ImplConnection.h>
#include <ImplItem.h>

#include <CrySystem/File/CryFile.h>
#include <CryString/CryPath.h>

#include <CrySerialization/Decorators/Range.h>
#include <CrySerialization/Enum.h>
#include <CrySerialization/ClassFactory.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace ACE
{
namespace SDLMixer
{
enum class EConnectionType
{
	Start = 0,
	Stop,
	NumTypes,
};

class CSdlMixerConnection final : public CImplConnection
{
public:

	explicit CSdlMixerConnection(CID const id)
		: CImplConnection(id)
		, m_type(EConnectionType::Start)
		, m_isPanningEnabled(true)
		, m_isAttenuationEnabled(true)
		, m_minAttenuation(0.0f)
		, m_maxAttenuation(100.0f)
		, m_volume(-14.0f)
		, m_loopCount(1)
		, m_isInfiniteLoop(false)
	{}

	virtual bool HasProperties() const override { return true; }

	virtual void Serialize(Serialization::IArchive& ar) override;

	EConnectionType m_type;
	float           m_minAttenuation;
	float           m_maxAttenuation;
	float           m_volume;
	uint            m_loopCount;
	bool            m_isPanningEnabled;
	bool            m_isAttenuationEnabled;
	bool            m_isInfiniteLoop;
};

using SdlConnectionPtr = std::shared_ptr<CSdlMixerConnection>;

class CImplSettings final : public IImplSettings
{
public:

	CImplSettings()
		: m_projectPath(PathUtil::GetGameFolder() + CRY_NATIVE_PATH_SEPSTR AUDIO_SYSTEM_DATA_ROOT CRY_NATIVE_PATH_SEPSTR "sdlmixer")
		, m_soundBanksPath(PathUtil::GetGameFolder() + CRY_NATIVE_PATH_SEPSTR AUDIO_SYSTEM_DATA_ROOT CRY_NATIVE_PATH_SEPSTR "sdlmixer")
	{}

	virtual char const* GetSoundBanksPath() const override { return m_soundBanksPath.c_str(); }
	virtual char const* GetProjectPath() const override    { return m_projectPath.c_str(); }
	virtual void        SetProjectPath(char const* szPath) override;

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
	virtual ~CEditorImpl() override;

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

	CID  GetId(string const& sName) const;
	void CreateControlCache(CImplItem const* const pParent);
	void Clear();

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

	using SdlMixerConnections = std::map<CID, std::vector<SdlConnectionPtr>>;

	CImplItem               m_root;
	SdlMixerConnections     m_connectionsByID;
	std::vector<CImplItem*> m_controlsCache;
	CImplSettings           m_implSettings;
};
} // namespace SDLMixer
} // namespace ACE
