// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct CryGUID;

struct IObjectLayer
{
	//! Get layer name.
	virtual const string& GetName() const = 0;

	//! Get layer name including its parent names.
	virtual string GetFullName() const = 0;

	//! Get GUID assigned to this layer.
	virtual const CryGUID& GetGUID() const = 0;

	virtual ~IObjectLayer() {}
	virtual void SetModified(bool isModified = true) = 0;
	virtual void SetVisible(bool isVisible, bool bRecursive = true) = 0;
	virtual void SetFrozen(bool isFrozen, bool bRecursive = true) = 0;
	virtual bool IsVisible(bool bRecursive = true) const = 0;
	virtual bool IsFrozen(bool bRecursive = true) const = 0;

	virtual IObjectLayer* GetParentIObjectLayer() const = 0;

	//! Returns the filepath of this layer. The path may not exist if the level has not been saved yet.
	virtual string GetLayerFilepath() const = 0;

	//! Number of nested layers.
	virtual int  GetChildCount() const = 0;

	//! Specifies if the layer is folder layer.
	virtual bool IsFolder() const = 0;

	virtual IObjectLayer* GetChildIObjectLayer(int index) const = 0;

	//! Return the list of file that comprise the layer (not including .lyr file)
	//! Paths to files are relative to the corresponding level folder.
	virtual const std::vector<string>& GetFiles() const = 0; 
};
