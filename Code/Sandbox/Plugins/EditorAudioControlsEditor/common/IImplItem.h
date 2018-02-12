// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "SystemTypes.h"

#include <CryCore/Platform/platform.h>
#include <CryString/CryString.h>

namespace ACE
{
struct IImplItem
{
	virtual ~IImplItem() = default;

	//! Returns id of the item.
	virtual CID GetId() const = 0;

	//! Returns type of the item.
	virtual ItemType GetType() const = 0;

	//! Returns name of the item.
	virtual string GetName() const = 0;  // char* const?

	//! Returns file path of the item.
	//! If the item is not a file, an empty path is returned.
	virtual string const& GetFilePath() const = 0;

	//! Returns radius of the item.
	//! The radius is used to calculate the activity radius of the connected audio system trigger.
	virtual float GetRadius() const = 0;

	//! Returns the number of children.
	virtual size_t GetNumChildren() const = 0; // NumChilds?

	//! Returns a pointer to the child item at the given index.
	//! \param index Index of the child slot.
	virtual IImplItem* GetChildAt(size_t const index) const = 0;

	//! Returns a pointer to the parent item.
	virtual IImplItem* GetParent() const = 0;

	//! Returns a bool if the item is a placeholder or not.
	//! Placeholders are items that exist as connections in audio system XML files, but not in the middleware project.
	//! These placeholders are filtered by the middleware model and are not visible in the middleware tree view.
	virtual bool IsPlaceholder() const = 0;

	//! Returns a bool if the item is localized or not.
	virtual bool IsLocalized() const = 0;

	//! Returns a bool if the item is connected or not.
	virtual bool IsConnected() const = 0;

	//! Returns bool if the item is a container or not.
	virtual bool IsContainer() const = 0;
};
} // namespace ACE
