// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "IAudioConnection.h"
#include <CryCore/Platform/platform.h>
#include <CryString/CryString.h>

namespace ACE
{
class IAudioSystemItem
{
public:

	IAudioSystemItem() = default;
	IAudioSystemItem(string const& name, CID const id, ItemType const type)
		: m_name(name)
		, m_id(id)
		, m_type(type)
	{
	}

	virtual ~IAudioSystemItem() = default;

	// unique id for this control
	CID              GetId() const                { return m_id; }
	void             SetId(CID const id)          { m_id = id; }

	virtual ItemType GetType() const              { return m_type; }
	void             SetType(ItemType const type) { m_type = type; }

	string           GetName() const              { return m_name; }
	void             SetName(string const& name)
	{
		if (name != m_name)
		{
			m_name = name;
		}
	}

	virtual bool      IsPlaceholder() const                       { return m_isPlaceholder; }
	void              SetPlaceholder(bool const isPlaceholder)    { m_isPlaceholder = isPlaceholder; }

	virtual bool      IsLocalised() const                         { return m_isLocalised; }
	void              SetLocalised(bool const isLocalised)        { m_isLocalised = isLocalised; }

	virtual bool      IsConnected() const                         { return m_isConnected; }
	void              SetConnected(bool const isConnected)        { m_isConnected = isConnected; }

	size_t            ChildCount() const                          { return m_children.size(); }
	void              AddChild(IAudioSystemItem* const pChild)    { m_children.push_back(pChild); pChild->SetParent(this); }
	void              RemoveChild(IAudioSystemItem* const pChild) { stl::find_and_erase(m_children, pChild); pChild->SetParent(nullptr); }
	IAudioSystemItem* GetChildAt(size_t const index) const        { return m_children[index]; }
	IAudioSystemItem* GetParent() const { return m_parent; }
	void              SetParent(IAudioSystemItem* const pParent)  { m_parent = pParent; }

	float             GetRadius() const { return m_radius; }
	void              SetRadius(float const radius)               { m_radius = radius; }

private:

	CID                            m_id = ACE_INVALID_ID;
	ItemType                       m_type = AUDIO_SYSTEM_INVALID_TYPE;
	string                         m_name;
	bool                           m_isPlaceholder = false;
	bool                           m_isLocalised = false;
	bool                           m_isConnected = false;
	std::vector<IAudioSystemItem*> m_children;
	IAudioSystemItem*              m_parent = nullptr;
	float                          m_radius = 0.0f;
};
} // namespace ACE
