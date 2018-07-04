// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Item.h"

#include <CrySystem/XML/IXml.h>

namespace ACE
{
namespace Impl
{
namespace Fmod
{
class CImpl;

class CProjectLoader final
{
public:

	explicit CProjectLoader(string const& projectPath, string const& soundbanksPath, CItem& rootItem, ItemCache& itemCache, CImpl const& impl);

	CProjectLoader() = delete;

private:

	CItem* CreateItem(
		string const& name,
		EItemType const type,
		CItem* const pParent,
		EItemFlags const flags,
		EPakStatus const pakStatus = EPakStatus::None,
		string const& filePath = "");

	void   LoadBanks(string const& folderPath, bool const isLocalized, CItem& parent);
	void   ParseFolder(string const& folderPath, CItem& editorFolder, CItem& parent);
	void   ParseFile(string const& filepath, CItem& parent, EPakStatus const pakStatus);
	void   ParseKeysFile(string const& filePath, CItem& parent);
	void   RemoveEmptyMixerGroups();
	void   RemoveEmptyEditorFolders(CItem* const pEditorFolder);

	CItem* GetContainer(string const& id, EItemType const type, CItem& parent, EPakStatus const pakStatus);
	void   LoadContainer(XmlNodeRef const pNode, EItemType const type, string const& relationshipParamName, CItem& parent, EPakStatus const pakStatus);
	void   LoadSnapshotGroup(XmlNodeRef const pNode, CItem& parent, EPakStatus const pakStatus);
	void   LoadFolder(XmlNodeRef const pNode, CItem& parent, EPakStatus const pakStatus);
	void   LoadMixerGroup(XmlNodeRef const pNode, CItem& parent, EPakStatus const pakStatus);

	CItem* LoadItem(XmlNodeRef const pNode, EItemType const type, CItem& parent, EPakStatus const pakStatus);
	CItem* LoadEvent(XmlNodeRef const pNode, CItem& parent, EPakStatus const pakStatus);
	void   LoadSnapshot(XmlNodeRef const pNode, CItem& parent, EPakStatus const pakStatus);
	void   LoadReturn(XmlNodeRef const pNode, CItem& parent, EPakStatus const pakStatus);
	void   LoadParameter(XmlNodeRef const pNode, CItem& parent, EPakStatus const pakStatus);
	void   LoadVca(XmlNodeRef const pNode, CItem& parent, EPakStatus const pakStatus);
	void   LoadAudioTable(XmlNodeRef const pNode);
	void   LoadKeys(CItem& parent);

	using ItemIds = std::map<string, CItem*>;

	CImpl const&        m_impl;
	CItem&              m_rootItem;
	ItemCache&          m_itemCache;
	ItemIds             m_containerIds;
	ItemIds             m_snapshotGroupItems;
	std::vector<CItem*> m_emptyMixerGroups;
	string const        m_projectPath;
	std::set<string>    m_audioTableDirectories;
};
} // namespace Fmod
} // namespace Impl
} // namespace ACE
