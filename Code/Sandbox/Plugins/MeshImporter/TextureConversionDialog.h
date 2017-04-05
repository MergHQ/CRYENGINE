// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryString/CryString.h>
#include <CrySystem/File/IFileChangeMonitor.h>
#include <QAdvancedTreeView.h>

#include <QAbstractItemModel>
#include <QTreeView>

#include <vector>

class CComboBoxDelegate;
bool FileExists(const string& path);

class CTextureManager
{
private:
	struct STexture
	{
		string sourceFilePath;  // Texture file path read from source file.
		string targetFilePath;  // New file path of this texture (relative). Can be appended to target directory.
		string rcSettings;

		STexture();
	};
public:
	typedef STexture* TextureHandle;

	CTextureManager();
	CTextureManager(const CTextureManager& other);
	~CTextureManager();

	CTextureManager& operator=(const CTextureManager& other);

	string        GetSourceDirectory() const;

	void          SetSourceDirectory(const string& workingDirectory);

	void          ClearTextures();
	bool          AddTexture(const string& originalFilePath, const string& rcSettings);

	void          SetRcSettings(TextureHandle tex, const string& rcSettings);

	int           GetTextureCount() const;
	TextureHandle GetTextureFromIndex(int index);
	TextureHandle GetTextureFromSourcePath(const string& name);
	TextureHandle GetTextureFromTargetPath(const string& name);

	string        GetTargetPath(TextureHandle tex) const;
	string        GetSourcePath(TextureHandle tex) const;
	string        GetRcSettings(TextureHandle tex) const;

	string        GetLoadPath(TextureHandle tex) const;
private:
	string        TranslateFilePath(const string& sourcePath) const;

	string m_sourceDirectory; // Directory of source FBX file.
	std::vector<std::unique_ptr<STexture>> m_textures;
};

class CTextureMonitor : public IFileChangeListener
{
public:
	CTextureMonitor(CTextureManager* pTextureManager, const string& dir);
	~CTextureMonitor();

	// IFileChangeListener implementation.
	virtual void OnFileChange(const char* szFilename, EChangeType eType) override;
private:
	CTextureManager* m_pTextureManager;
	string           m_subject;
};

namespace MeshImporter {
class CSceneManager;
}

void Fill(const MeshImporter::CSceneManager& sceneManager, CTextureManager& texManager);

// Binds CTextureManager to Qt's Model/View concept.

class CTextureModel : public QAbstractItemModel
{
public:
	enum EColumn
	{
		eColumn_Filename,
		eColumn_RcSettings,
		eColumn_COUNT
	};

	enum ERcSettings
	{
		eRcSettings_Diffuse,
		eRcSettings_Bumpmap,
		eRcSettings_Specular,
		eRcSettings_COUNT
	};
public:
	CTextureModel(CTextureManager* pTextureManager, QObject* pParent = nullptr);

	CTextureManager::TextureHandle GetTexture(const QModelIndex& index);

	CTextureManager*               GetTextureManager();

	// QAbstractItemModel implementation.
	QModelIndex   index(int row, int column, const QModelIndex& parent) const override;
	QModelIndex   parent(const QModelIndex& index) const override;
	int           rowCount(const QModelIndex& index) const override;
	int           columnCount(const QModelIndex& index) const override;
	QVariant      data(const QModelIndex& index, int role) const override;
	bool          setData(const QModelIndex& index, const QVariant& value, int role) override;
	QVariant      headerData(int column, Qt::Orientation orientation, int role) const override;
	Qt::ItemFlags flags(const QModelIndex& modelIndex) const override;

	void          Reset()
	{
		// TODO: Delete me.
		beginResetModel();
		endResetModel();
	}

	void   OnTexturesChanged();
private:
	string GetSettingsRcString(ERcSettings rcSettings) const;
	int    GetSettings(const string& rcString) const;

	CTextureManager* m_pTextureManager;
};

class CTextureView : public QAdvancedTreeView
{
public:
	CTextureView(QWidget* pParent = nullptr);

	void SetModel(CTextureModel* pTextureModel);
private:
	void CreateContextMenu(const QPoint& point);

	CTextureModel*                     m_pTextureModel;
	std::unique_ptr<CComboBoxDelegate> m_pComboBoxDelegate;
};
