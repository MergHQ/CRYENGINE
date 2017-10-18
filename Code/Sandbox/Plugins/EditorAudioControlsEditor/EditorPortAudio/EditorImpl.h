// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IEditorImpl.h>
#include <ImplConnection.h>
#include <ImplItem.h>

#include <CrySystem/File/CryFile.h>
#include <CryString/CryPath.h>

#include <CrySerialization/Decorators/Range.h>
#include <CrySerialization/Enum.h>
#include <CrySerialization/ClassFactory.h>
#include "ProjectLoader.h"
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace ACE
{
namespace PortAudio
{
enum class EConnectionType
{
	Start = 0,
	Stop,
	NumTypes,
};

class CConnection : public CImplConnection
{
public:

	explicit CConnection(CID const id)
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

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		ar(m_type, "action", "Action");
		ar(m_isPanningEnabled, "panning", "Enable Panning");

		if (ar.openBlock("DistanceAttenuation", "+Distance Attenuation"))
		{
			ar(m_isAttenuationEnabled, "attenuation", "Enable");

			if (m_isAttenuationEnabled)
			{
				if (ar.isInput())
				{
					float minAtt = m_minAttenuation;
					float maxAtt = m_maxAttenuation;
					ar(minAtt, "min_att", "Min Distance");
					ar(maxAtt, "max_att", "Max Distance");

					if (minAtt > maxAtt)
					{
						if (minAtt != m_minAttenuation)
						{
							maxAtt = minAtt;
						}
						else
						{
							minAtt = maxAtt;
						}
					}

					m_minAttenuation = minAtt;
					m_maxAttenuation = maxAtt;
				}
				else
				{
					ar(m_minAttenuation, "min_att", "Min Distance");
					ar(m_maxAttenuation, "max_att", "Max Distance");
				}
			}
			else
			{
				ar(m_minAttenuation, "min_att", "!Min Distance");
				ar(m_maxAttenuation, "max_att", "!Max Distance");
			}

			ar.closeBlock();
		}

		ar(Serialization::Range(m_volume, -96.0f, 0.0f), "vol", "Volume (dB)");

		if (ar.openBlock("Looping", "+Looping"))
		{
			ar(m_isInfiniteLoop, "infinite", "Infinite");

			if (m_isInfiniteLoop)
			{
				ar(m_loopCount, "loop_count", "!Count");
			}
			else
			{
				ar(m_loopCount, "loop_count", "Count");
			}

			ar.closeBlock();
		}

		if (ar.isInput())
		{
			signalConnectionChanged();
		}
	}

	EConnectionType m_type;
	float           m_minAttenuation;
	float           m_maxAttenuation;
	float           m_volume;
	uint            m_loopCount;
	bool            m_isPanningEnabled;
	bool            m_isAttenuationEnabled;
	bool            m_isInfiniteLoop;
};

typedef std::shared_ptr<CConnection> PortAudioConnectionPtr;

class CImplSettings final : public IImplSettings
{
public:

	CImplSettings()
		: m_projectPath(PathUtil::GetGameFolder() + CRY_NATIVE_PATH_SEPSTR AUDIO_SYSTEM_DATA_ROOT CRY_NATIVE_PATH_SEPSTR "portaudio")
		, m_soundBanksPath(PathUtil::GetGameFolder() + CRY_NATIVE_PATH_SEPSTR AUDIO_SYSTEM_DATA_ROOT CRY_NATIVE_PATH_SEPSTR "portaudio")
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
	// ~IAudioSystemEditor

private:

	CID  GetId(string const& name) const;
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

	typedef std::map<CID, std::vector<PortAudioConnectionPtr>> Connections;

	CImplItem               m_root;
	Connections             m_connectionsByID;
	std::vector<CImplItem*> m_controlsCache;
	CImplSettings           m_implSettings;
};
} // namespace PortAudio
} // namespace ACE
