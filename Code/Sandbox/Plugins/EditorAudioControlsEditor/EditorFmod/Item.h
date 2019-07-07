// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "../Common/IItem.h"
#include <PoolObject.h>

namespace ACE
{
namespace Impl
{
namespace Fmod
{
enum class EItemType : CryAudio::EnumFlagsType
{
	None,
	Bank,
	Snapshot,
	Return,
	VCA,
	Parameter,
	Key,
	Event,
	MixerGroup,
	Folder,
	EditorFolder, };

class CItem final : public IItem, public CryAudio::CPoolObject<CItem, stl::PSyncNone>
{
public:

	CItem() = delete;
	CItem(CItem const&) = delete;
	CItem(CItem&&) = delete;
	CItem& operator=(CItem const&) = delete;
	CItem& operator=(CItem&&) = delete;

	explicit CItem(
		string const& name,
		ControlId const id,
		EItemType const type,
		EItemFlags const flags = EItemFlags::None,
		EPakStatus const pakStatus = EPakStatus::None,
		string const& filePath = "")
		: m_name(name)
		, m_id(id)
		, m_type(type)
		, m_flags(flags)
		, m_pakStatus(pakStatus)
		, m_filePath(filePath)
		, m_pParent(nullptr)
		, m_pathName("")
	{}

	virtual ~CItem() override = default;

	// IItem
	virtual ControlId     GetId() const override                        { return m_id; }
	virtual string const& GetName() const override                      { return m_name; }
	virtual size_t        GetNumChildren() const override               { return m_children.size(); }
	virtual IItem*        GetChildAt(size_t const index) const override { return m_children[index]; }
	virtual IItem*        GetParent() const override                    { return m_pParent; }
	virtual EItemFlags    GetFlags() const override                     { return m_flags; }
	// ~IItem

	EItemType     GetType() const                     { return m_type; }
	string const& GetFilePath() const                 { return m_filePath; }
	EPakStatus    GetPakStatus() const                { return m_pakStatus; }

	void          SetFlags(EItemFlags const flags)    { m_flags = flags; }

	void          SetPathName(string const& pathName) { m_pathName = pathName; }
	string const& GetPathName() const                 { return m_pathName; }

	void          AddChild(CItem* const pChild);
	void          RemoveChild(CItem* const pChild);
	void          Clear();

private:

	void SetParent(CItem* const pParent) { m_pParent = pParent; }

	string const        m_name;
	ControlId const     m_id;
	EItemType const     m_type;
	EItemFlags          m_flags;
	EPakStatus const    m_pakStatus;
	string const        m_filePath;
	CItem*              m_pParent;
	string              m_pathName;
	std::vector<CItem*> m_children;

};

using ItemCache = std::map<ControlId, CItem*>;
} // namespace Fmod
} // namespace Impl
} // namespace ACE
