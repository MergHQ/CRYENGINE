// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IImplItem.h>
#include "ImplTypes.h"

namespace ACE
{
namespace SDLMixer
{
enum class EImplItemFlags
{
	None          = 0,
	IsPlaceHolder = BIT(0),
	IsConnected   = BIT(1),
	IsContainer   = BIT(2),
};
CRY_CREATE_ENUM_FLAG_OPERATORS(EImplItemFlags);

class CImplItem final : public IImplItem
{
public:

	explicit CImplItem(
	  string const& name,
	  CID const id,
	  ItemType const type,
	  EImplItemFlags const flags = EImplItemFlags::None,
	  string const& filePath = "")
		: m_name(name)
		, m_id(id)
		, m_type(type)
		, m_flags(flags)
		, m_filePath(filePath)
		, m_pParent(nullptr)
	{}

	virtual ~CImplItem() override = default;

	CImplItem() = delete;

	// IImplItem
	virtual CID           GetId() const override                        { return m_id; }
	virtual ItemType      GetType() const override                      { return m_type; }
	virtual string        GetName() const override                      { return m_name; }
	virtual string const& GetFilePath() const override                  { return m_filePath; }
	virtual float         GetRadius() const override                    { return 0.0f; }

	virtual size_t        GetNumChildren() const override               { return m_children.size(); }
	virtual IImplItem*    GetChildAt(size_t const index) const override { return m_children[index]; }
	virtual IImplItem*    GetParent() const override                    { return m_pParent; }

	virtual bool          IsPlaceholder() const override                { return (m_flags& EImplItemFlags::IsPlaceHolder) != 0; }
	virtual bool          IsLocalized() const override                  { return false; }
	virtual bool          IsConnected() const override                  { return (m_flags& EImplItemFlags::IsConnected) != 0; }
	virtual bool          IsContainer() const override                  { return (m_flags& EImplItemFlags::IsContainer) != 0; }
	// ~IImplItem

	void SetConnected(bool const isConnected);
	void AddChild(CImplItem* const pChild);

	void Clear();

private:

	void SetParent(CImplItem* const pParent) { m_pParent = pParent; }

	CID const               m_id;
	ItemType const          m_type;
	string const            m_name;
	string const            m_filePath;
	std::vector<CImplItem*> m_children;
	CImplItem*              m_pParent;
	EImplItemFlags          m_flags;
};
} // namespace SDLMixer
} // namespace ACE
