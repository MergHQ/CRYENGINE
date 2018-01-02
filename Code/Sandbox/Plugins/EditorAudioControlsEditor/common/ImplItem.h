// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "SystemTypes.h"

#include <CryCore/Platform/platform.h>
#include <CryString/CryString.h>

namespace ACE
{

enum class EImplItemFlags
{
	None          = 0,
	IsPlaceHolder = BIT(0),
	IsLocalized   = BIT(1),
	IsModified    = BIT(2),
	IsConnected   = BIT(3),
	IsContainer   = BIT(4),
};
CRY_CREATE_ENUM_FLAG_OPERATORS(EImplItemFlags);

class CImplItem
{
public:

	CImplItem() = default;
	CImplItem(string const& name, CID const id, ItemType const type)
		: m_name(name)
		, m_id(id)
		, m_type(type)
		, m_flags(EImplItemFlags::None)
		, m_filePath("")
	{
	}

	virtual ~CImplItem() = default;

	// unique id for this control
	CID           GetId() const                            { return m_id; }
	void          SetId(CID const id)                      { m_id = id; }

	ItemType      GetType() const                          { return m_type; }
	void          SetType(ItemType const type)             { m_type = type; }

	string        GetName() const                          { return m_name; }
	void          SetName(string const& name)              { m_name = name; }

	string const& GetFilePath() const                      { return m_filePath; }
	void          SetFilePath(string const& filePath)      { m_filePath = filePath; }

	size_t        ChildCount() const                       { return m_children.size(); }
	void          AddChild(CImplItem* const pChild)        { m_children.emplace_back(pChild); pChild->SetParent(this); }
	void          RemoveChild(CImplItem* const pChild)     { stl::find_and_erase(m_children, pChild); pChild->SetParent(nullptr); }
	CImplItem*    GetChildAt(size_t const index) const     { return m_children[index]; }
	CImplItem*    GetParent() const                        { return m_pParent; }
	void          SetParent(CImplItem* const pParent)      { m_pParent = pParent; }

	float         GetRadius() const                        { return m_radius; }
	void          SetRadius(float const radius)            { m_radius = radius; }

	bool          IsPlaceholder() const                    { return (m_flags & EImplItemFlags::IsPlaceHolder) != 0; }
	bool          IsLocalised() const                      { return (m_flags & EImplItemFlags::IsLocalized) != 0; }
	bool          IsConnected() const                      { return (m_flags & EImplItemFlags::IsConnected) != 0; }
	bool          IsContainer() const                      { return (m_flags & EImplItemFlags::IsContainer) != 0; }

	void SetPlaceholder(bool const isPlaceholder)
	{
		if (isPlaceholder)
		{
			m_flags |= EImplItemFlags::IsPlaceHolder;
		}
		else
		{
			m_flags &= ~EImplItemFlags::IsPlaceHolder;
		}
	}

	void SetLocalised(bool const isLocalised)
	{
		if (isLocalised)
		{
			m_flags |= EImplItemFlags::IsLocalized;
		}
		else
		{
			m_flags &= ~EImplItemFlags::IsLocalized;
		}
	}

	void SetConnected(bool const isConnected)
	{
		if (isConnected)
		{
			m_flags |= EImplItemFlags::IsConnected;
		}
		else
		{
			m_flags &= ~EImplItemFlags::IsConnected;
		}
	}

	void SetContainer(bool const isContainer)
	{
		if (isContainer)
		{
			m_flags |= EImplItemFlags::IsContainer;
		}
		else
		{
			m_flags &= ~EImplItemFlags::IsContainer;
		}
	}

private:

	CID                     m_id = ACE_INVALID_ID;
	ItemType                m_type = AUDIO_SYSTEM_INVALID_TYPE;
	string                  m_name;
	string                  m_filePath;
	std::vector<CImplItem*> m_children;
	CImplItem*              m_pParent = nullptr;
	float                   m_radius = 0.0f;
	EImplItemFlags          m_flags;
};
} // namespace ACE
