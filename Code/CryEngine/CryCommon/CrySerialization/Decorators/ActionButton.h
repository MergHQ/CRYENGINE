// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/IArchive.h>
#include <CryCore/functor.h>

namespace Serialization
{

struct IActionButton;

DECLARE_SHARED_POINTERS(IActionButton)

struct IActionButton
{
	virtual ~IActionButton() {}

	virtual void             Callback() const = 0;
	virtual const char*      Icon() const = 0;
	virtual IActionButtonPtr Clone() const = 0;
};

typedef Functor0 FunctorActionButtonCallback;

struct FunctorActionButton : public IActionButton
{
	FunctorActionButtonCallback callback;
	string                      icon;

	explicit FunctorActionButton(const FunctorActionButtonCallback& callback, const char* icon = "")
		: callback(callback)
		, icon(icon)
	{
	}

	// IActionButton

	virtual void Callback() const override
	{
		if (callback)
		{
			callback();
		}
	}

	virtual const char* Icon() const override
	{
		return icon.c_str();
	}

	virtual IActionButtonPtr Clone() const override
	{
		return IActionButtonPtr(new FunctorActionButton(callback, icon.c_str()));
	}

	// ~IActionButton
};

inline bool Serialize(Serialization::IArchive& ar, FunctorActionButton& button, const char* name, const char* label)
{
	if (ar.isEdit())
		return ar(Serialization::SStruct::forEdit(static_cast<Serialization::IActionButton&>(button)), name, label);
	else
		return false;
}

inline FunctorActionButton ActionButton(const FunctorActionButtonCallback& callback, const char* icon = "")
{
	return FunctorActionButton(callback, icon);
}

}
