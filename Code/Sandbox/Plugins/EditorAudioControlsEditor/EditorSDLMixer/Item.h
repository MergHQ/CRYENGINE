// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IImplItem.h>

namespace ACE
{
namespace SDLMixer
{
enum class EItemType
{
	None,
	Event,
	Folder,
};

enum class EItemFlags
{
	None          = 0,
	IsPlaceHolder = BIT(0),
	IsConnected   = BIT(1),
	IsContainer   = BIT(2),
};
CRY_CREATE_ENUM_FLAG_OPERATORS(EItemFlags);

class CItem final : public IImplItem
{
public:

	explicit CItem(
	  string const& name,
	  ControlId const id,
	  EItemType const type,
	  EItemFlags const flags = EItemFlags::None,
	  string const& filePath = "")
		: m_name(name)
		, m_id(id)
		, m_type(type)
		, m_flags(flags)
		, m_filePath(filePath)
		, m_pParent(nullptr)
	{}

	virtual ~CItem() override = default;

	CItem() = delete;

	// IImplItem
	virtual ControlId     GetId() const override                        { return m_id; }
	virtual string        GetName() const override                      { return m_name; }
	virtual string const& GetFilePath() const override                  { return m_filePath; }
	virtual float         GetRadius() const override                    { return 0.0f; }
	virtual int           GetSortPriority() const override              { return static_cast<int>(m_type); }

	virtual size_t        GetNumChildren() const override               { return m_children.size(); }
	virtual IImplItem*    GetChildAt(size_t const index) const override { return m_children[index]; }
	virtual IImplItem*    GetParent() const override                    { return m_pParent; }

	virtual bool          IsPlaceholder() const override                { return (m_flags& EItemFlags::IsPlaceHolder) != 0; }
	virtual bool          IsLocalized() const override                  { return false; }
	virtual bool          IsConnected() const override                  { return (m_flags& EItemFlags::IsConnected) != 0; }
	virtual bool          IsContainer() const override                  { return (m_flags& EItemFlags::IsContainer) != 0; }
	// ~IImplItem

	EItemType GetType() const { return m_type; }

	void      SetConnected(bool const isConnected);
	void      AddChild(CItem* const pChild);

	void      Clear();

private:

	void SetParent(CItem* const pParent) { m_pParent = pParent; }

	ControlId const     m_id;
	EItemType const     m_type;
	string const        m_name;
	string const        m_filePath;
	std::vector<CItem*> m_children;
	CItem*              m_pParent;
	EItemFlags          m_flags;
};
} // namespace SDLMixer
} // namespace ACE

