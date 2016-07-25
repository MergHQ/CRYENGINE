// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/XML/IXml.h>
#include <ACETypes.h>
#include <IAudioSystemEditor.h>
#include "AudioSystemControl_wwise.h"

namespace ACE
{

class CAudioWwiseLoader
{
public:
	CAudioWwiseLoader(const string& projectPath, const string& soundbanksPath, IAudioSystemItem& root);

private:
	void              LoadSoundBanks(const string& rootFolder, const bool bLocalized, IAudioSystemItem& parent);
	void              LoadFolder(const string& folderPath, IAudioSystemItem& parent);
	void              LoadFile(const string& filename, const string& path, IAudioSystemItem& parent);
	void              LoadXml(XmlNodeRef root, IAudioSystemItem& parent, const string& filePath);
	IAudioSystemItem* CreateItem(const string& name, ItemType type, IAudioSystemItem& pParent, const string& path = "");
	void              LoadEventsMetadata(const string& soundbanksPath);

	IAudioSystemItem* GetControlByName(const string& name, bool bIsLocalised = false, IAudioSystemItem* pParent = nullptr) const;

private:

	struct SEventInfo
	{
		float maxRadius;
	};
	typedef std::map<uint32, SEventInfo> EventsInfoMap;
	EventsInfoMap                    m_eventsInfoMap;
	IAudioSystemItem&                m_root;
	std::map<CID, IAudioSystemItem*> m_controlsCache;
	string m_projectRoot;
};
}
