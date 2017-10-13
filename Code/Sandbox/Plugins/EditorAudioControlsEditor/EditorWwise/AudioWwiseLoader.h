// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "AudioSystemControl_wwise.h"
#include <CrySystem/XML/IXml.h>
#include <ACETypes.h>
#include <IAudioSystemEditor.h>

namespace ACE
{
class CAudioWwiseLoader
{
public:

	CAudioWwiseLoader(string const& projectPath, string const& soundbanksPath, IAudioSystemItem& root);

private:

	void              LoadSoundBanks(string const& folderPath, bool const isLocalized, IAudioSystemItem& parent);
	void              LoadFolder(string const& folderPath, string const& folderName, IAudioSystemItem& parent);
	void              LoadWorkUnitFile(string const& filePath, IAudioSystemItem& parent);
	void              LoadXml(XmlNodeRef const root, IAudioSystemItem& parent);
	IAudioSystemItem* CreateItem(string const& name, EWwiseItemTypes const type, IAudioSystemItem& pParent);
	void              LoadEventsMetadata(string const& soundbanksPath);

	IAudioSystemItem* GetControlByName(string const& name, bool const isLocalised = false, IAudioSystemItem const* const pParent = nullptr) const;

	void              BuildFileCache(string const& folderPath);

private:

	struct SEventInfo
	{
		float maxRadius;
	};

	typedef std::map<uint32, SEventInfo>        EventsInfoMap;
	typedef std::map<CID, IAudioSystemItem*>    ControlsCache;
	typedef std::map<uint32, string>            FilesCache;
	typedef std::map<uint32, IAudioSystemItem*> Items;

	EventsInfoMap     m_eventsInfoMap;
	IAudioSystemItem& m_root;
	ControlsCache     m_controlsCache;
	string            m_projectRoot;

	// This maps holds the items with the internal IDs given in the Wwise files.
	Items m_items;

	// Cache with the file path to each work unit file
	FilesCache m_filesCache;

	// List of already loaded work unit files
	std::set<uint32> m_filesLoaded;
};
} // namespace ACE
