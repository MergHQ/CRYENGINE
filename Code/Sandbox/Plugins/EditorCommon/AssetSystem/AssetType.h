// Copyright 2001-2016 Crytek GmbH. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "AutoRegister.h"
#include "IEditorClassFactory.h"
#include "CryIcon.h"

#include <QVariant>

class CAsset;
class CAssetImportContext;
class CAssetEditor;

class CItemModelAttribute;

class EDITOR_COMMON_API CAssetType : public IClassDesc
{
public:
	CAssetType() {}
	virtual ~CAssetType() {}

	virtual ESystemClassID SystemClassID() override { return ESYSTEM_CLASS_ASSET_TYPE; }

	//! Returns the asset type name. DO NOT CHANGE THIS as this will be serialized to disk and used to identify asset types
	//! Do not use for UI purposes
	virtual char* GetTypeName() const = 0;

	//! Ui friendly name. Use QT_TR_NOOP to enable localization
	virtual char* GetUiTypeName() const = 0;
	

	//! Writes a new file containing a default created asset of this type
	//! This will be used for New assets or SaveAs
	//! Use the asset filename as target filename
	//! Is it assumed an asset can either be imported or created
	virtual bool Create(CAsset* asset) const { CRY_ASSERT(0); /*not implemented*/ return false; }

	//! Can the asset be opened for edit
	virtual bool CanBeEdited() const { return false; }
	//! Opens the asset in the appropriate editor
	//! Usually implemented using CAssetEditor::OpenAssetForEdit
	virtual CAssetEditor* Edit(CAsset* asset) const { CRY_ASSERT(0); /*not implemented*/ return nullptr; }

	virtual bool RenameAsset(CAsset* asset) const { CRY_ASSERT(0); /*not implemented*/ return false; }
	virtual bool DuplicateAsset(CAsset* original, CAsset* duplicate) const { CRY_ASSERT(0); /*not implemented*/ return false; }
	virtual bool MoveAsset(CAsset* original, CAsset* duplicate) const { CRY_ASSERT(0); /*not implemented*/ return false; }

	virtual CryIcon GetIcon() const;
	virtual QWidget* CreatePreviewWidget(CAsset* pAsset, QWidget* pParent = nullptr) const;

	virtual std::vector<CItemModelAttribute*> GetDetails() const { return std::vector<CItemModelAttribute*>(); }
	virtual QVariant GetDetailValue(const CAsset* pAsset, const CItemModelAttribute* pDetail) const { return QVariant(); }


	//! @name Importing
	//! @{

	//! Is it assumed an asset type has one of the two following behaviors
	//! 1) Imported from a source file and can only be created through import
	//! 2) Created within the Sandbox and never has a source file
	//! If returning true, it will be assumed the other import methods are implemented correctly
	//! If returning false, it will be assumed Create() and Edit() are implemented correctly
	virtual bool IsImported() const { return false; }

	// Returns list of file extensions that can be imported for this type.
	// @return Extensions are lower-case and without leading dot.
	virtual std::vector<string> GetImportExtensions() const { return std::vector<string>(); }

	//! Returns a list of other asset types which should be imported before this asset type.
	//! The dependencies are queried for each file individually.
	//! Example: If both a mesh and a material can be imported from the same file, then mesh should depend on
	//! material, since the mesh store a reference to the material. In this example, the mesh asset type would
	//! return the dependency vector { "Material" }.
	virtual std::vector<string> GetImportDependencyTypeNames() const { return std::vector<string>(); }

	//! Returns the data filename that will be written on import. Asset filename is the data filename with appended ".cryasset"
	virtual string GetTargetFilename(const string& sourceFilename) const { CRY_ASSERT(0); /*not implemented*/ return string(); }

	CAsset* DoImport(const string& filePath, CAssetImportContext& context) const;

	//! @}

	//! @name Create instances.
	//! @{

	//! Returns the name of an object class description that is used to create editor objects for this asset type; or nullptr, if no such class exists.
	//! @see CObjectClassDesc
	//! @see CBaseObject
	virtual const char* GetObjectClassName() const { return nullptr; }

	//! @}

protected:

	//! Helper function that parses a string and returns a variant of a type corresponding to \p pAttrib->GetType().
	//! If conversion fails, a default-constructed varient of that type is returned. (see QVariant::value).
	static QVariant GetVariantFromDetail(const string& detailValue, const CItemModelAttribute* pAttrib);

private:

	virtual bool CanImport(const string& filePath, CAssetImportContext& context) const;

	//TODO : better import system that takes into account multi-import and things like this, there needs to be an import task that returns a bunch of assets !
	virtual CAsset* Import(const string& filePath, CAssetImportContext& context) const { CRY_ASSERT(0); /*not implemented*/ return nullptr; }

};

//Helper macro for declaring IClassDesc.
#define DECLARE_ASSET_TYPE_DESC(type)                           \
virtual const char* ClassName() override { return #type; }      \
virtual void* CreateObject() override { return nullptr; } 


#define REGISTER_ASSET_TYPE(type) REGISTER_CLASS_DESC(type)

