// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "TextureConversionDialog.h"
#include "ComboBoxDelegate.h"
#include "ImporterUtil.h"
#include <IEditor.h>
#include <CryCore/ToolsHelpers/SettingsManagerHelpers.h>
#include <CryCore/ToolsHelpers/ResourceCompilerHelper.h>
#include <CrySystem/File/IFileChangeMonitor.h>
#include <CrySystem/File/LineStreamBuffer.h>

#include <QtUtil.h>

#include <CryString/CryPath.h>

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QHeaderView>

#include <QDir>
#include <QFileInfo>

#include <functional>

#include <CrySystem/IProjectManager.h>

extern void LogPrintf(const char* szFormat, ...);

void        TifConversionListener(const char* szWhat);
typedef void (* Listener)(const char*);
const char* ConvertToTIF(const string& filename, Listener listener, const wchar_t* szWorkingDirectory);

// ====================================================================================================
// CTextureMonitor implementation.
// ====================================================================================================

CTextureMonitor::CTextureMonitor(CTextureManager* pTextureManager, const string& subject)
	: m_pTextureManager(pTextureManager)
	, m_subject(subject)
{
	// Strip engine path.
	string dir = subject;
	string enginePath = PathUtil::ToUnixPath(PathUtil::AddSlash(PathUtil::GetEnginePath()));
	if (dir.length() > enginePath.length() && strnicmp(dir.c_str(), enginePath.c_str(), enginePath.length()) == 0)
	{
		dir = dir.substr(enginePath.length(), dir.length() - enginePath.length());
	}

	GetIEditor()->GetFileMonitor()->RegisterListener(this, PathUtil::MakeGamePath(dir).c_str());
}

CTextureMonitor::~CTextureMonitor()
{
	GetIEditor()->GetFileMonitor()->UnregisterListener(this);
}

CTextureManager::CTextureManager(const CTextureManager& other)
	: m_sourceDirectory(other.m_sourceDirectory)
{
	m_textures.reserve(other.m_textures.size());
	for (auto& tex : other.m_textures)
	{
		m_textures.emplace_back(new STexture(*tex));
	}
}

CTextureManager& CTextureManager::operator=(const CTextureManager& other)
{
	if (this != &other)
	{
		m_sourceDirectory = other.m_sourceDirectory;

		m_textures.clear();
		m_textures.reserve(other.m_textures.size());
		for (auto& tex : other.m_textures)
		{
			m_textures.emplace_back(new STexture(*tex));
		}
	}
	return *this;
}

void CTextureMonitor::OnFileChange(const char* szFilename, EChangeType eType)
{
	if (!stricmp(PathUtil::GetExt(szFilename), "dds"))
	{
		return;
	}

	const string tifFilename = PathUtil::ReplaceExtension(szFilename, "tif");

	string gameDir = gEnv->pSystem->GetIProjectManager()->GetCurrentAssetDirectoryAbsolute();

	if (!FileExists(PathUtil::Make(gameDir, tifFilename))) // XXX
	{
		ConvertToTIF(szFilename, TifConversionListener, QString(gameDir.c_str()).toStdWString().c_str());

		const string targetPath = QtUtil::ToString(QDir(QtUtil::ToQString(m_subject)).relativeFilePath(szFilename));

		// Apply RC settings, if possible.
		CTextureManager::TextureHandle tex = m_pTextureManager->GetTextureFromTargetPath(targetPath);
		if (tex)
		{
			const string rcSettings = m_pTextureManager->GetRcSettings(tex);
			if (!rcSettings.empty())
			{

				string rcArguments;
				rcArguments += " /refresh=1";
				rcArguments += " /savesettings=\"";
				rcArguments += rcSettings;
				rcArguments += "\"";

				// CResourceCompilerHelper::InvokeResourceCompiler(tifFilename, tifFilename, false, true);
				CResourceCompilerHelper::CallResourceCompiler(
				  tifFilename,
				  rcArguments.c_str(),
				  nullptr, // Listener
				  false,   // May show window
				  CResourceCompilerHelper::eRcExePath_editor,
				  false,  // silent
				  true);  // no user dialog
			}
		}
	}

}

// ====================================================================================================
// CTextureManager implementation.
// ====================================================================================================

CTextureManager::CTextureManager()
{}

CTextureManager::~CTextureManager()
{}

string CTextureManager::GetSourceDirectory() const
{
	return m_sourceDirectory;
}

void CTextureManager::SetSourceDirectory(const string& workingDirectory)
{
	m_sourceDirectory = workingDirectory;
}

bool CTextureManager::AddTexture(const string& sourceFilePath, const string& rcSettings)
{
	auto texIt = std::find_if(m_textures.begin(), m_textures.end(), [sourceFilePath](const std::unique_ptr<STexture>& pTex)
	{
		return !pTex->sourceFilePath.compareNoCase(sourceFilePath);
	});
	if (texIt != m_textures.end())
	{
		// Filename already exists.
		return false;
	}

	std::unique_ptr<STexture> texture(new STexture());

	texture->targetFilePath = TranslateFilePath(sourceFilePath);
	texture->sourceFilePath = sourceFilePath;
	texture->rcSettings = rcSettings;

	m_textures.push_back(std::move(texture));

	return true;
}

void CTextureManager::SetRcSettings(TextureHandle tex, const string& rcSettings)
{
	tex->rcSettings = rcSettings;
}

int CTextureManager::GetTextureCount() const
{
	return m_textures.size();
}

CTextureManager::TextureHandle CTextureManager::GetTextureFromIndex(int index)
{
	CRY_ASSERT(index >= 0 && index < m_textures.size());
	return m_textures[index].get();
}

CTextureManager::TextureHandle CTextureManager::GetTextureFromSourcePath(const string& sourceFilePath)
{
	auto texIt = std::find_if(m_textures.begin(), m_textures.end(), [sourceFilePath](const std::unique_ptr<STexture>& pTex)
	{
		return !pTex->sourceFilePath.compareNoCase(sourceFilePath);
	});

	if (texIt == m_textures.end())
	{
		return nullptr;
	}

	return texIt->get();
}

CTextureManager::TextureHandle CTextureManager::GetTextureFromTargetPath(const string& targetFilePath)
{
	auto texIt = std::find_if(m_textures.begin(), m_textures.end(), [targetFilePath](const std::unique_ptr<STexture>& pTex)
	{
		return !pTex->targetFilePath.compareNoCase(targetFilePath);
	});

	if (texIt == m_textures.end())
	{
		return nullptr;
	}

	return texIt->get();
}

string CTextureManager::GetTargetPath(TextureHandle tex) const
{
	return tex->targetFilePath;
}

string CTextureManager::GetSourcePath(TextureHandle tex) const
{
	return tex->sourceFilePath;
}

string CTextureManager::GetLoadPath(TextureHandle tex) const
{
	if (tex->targetFilePath == tex->sourceFilePath)
	{
		return m_sourceDirectory + "/" + tex->targetFilePath;
	}
	else
	{
		return tex->sourceFilePath;
	}
}

string CTextureManager::GetRcSettings(TextureHandle tex) const
{
	return tex->rcSettings;
}

void CTextureManager::ClearTextures()
{
	m_textures.clear();
}

CTextureManager::STexture::STexture()
{}

string CTextureManager::TranslateFilePath(const string& sourceTexturePath) const
{
	const QString absSourceDirectoryPath = QFileInfo(QtUtil::ToQString(m_sourceDirectory)).canonicalFilePath();
	const QString absSourceTexturePath = QFileInfo(QtUtil::ToQString(sourceTexturePath)).canonicalFilePath();

	if (absSourceTexturePath.startsWith(absSourceDirectoryPath))
	{
		return QtUtil::ToString(QDir(absSourceDirectoryPath).relativeFilePath(absSourceTexturePath));
	}
	else
	{
		return PathUtil::GetFile(sourceTexturePath);
	}
}

// ==================================================

// ==================================================

CTextureModel::CTextureModel(CTextureManager* pTextureManager, QObject* pParent)
	: QAbstractItemModel(pParent), m_pTextureManager(pTextureManager)
{
	// CTextureManager::FileChangeCallback func = std::bind(&CTextureModel::OnTexturesChanged, this); XXX
	// m_pTextureManager->AddFileChangeCallback(func);
}

CTextureManager::TextureHandle CTextureModel::GetTexture(const QModelIndex& index)
{
	CRY_ASSERT(index.isValid());
	return (CTextureManager::TextureHandle)index.internalPointer();
}

CTextureManager* CTextureModel::GetTextureManager()
{
	return m_pTextureManager;
}

QModelIndex CTextureModel::index(int row, int col, const QModelIndex& parent) const
{
	if (!parent.isValid())
	{
		CRY_ASSERT(row >= 0 && row < m_pTextureManager->GetTextureCount());
		CRY_ASSERT(col >= 0 && col < eColumn_COUNT);
		return createIndex(row, col, m_pTextureManager->GetTextureFromIndex(row));
	}
	return QModelIndex();
}

QModelIndex CTextureModel::parent(const QModelIndex& index) const
{
	return QModelIndex();
}

int CTextureModel::rowCount(const QModelIndex& index) const
{
	if (!index.isValid())
	{
		return m_pTextureManager->GetTextureCount();
	}
	return 0;
}

int CTextureModel::columnCount(const QModelIndex& index) const
{
	return eColumn_COUNT;
}

const char* GetDisplayNameOfSettings(CTextureModel::ERcSettings settings);

QVariant CTextureModel::data(const QModelIndex& index, int role) const
{
	CRY_ASSERT(index.isValid());

	CTextureManager::TextureHandle tex = (CTextureManager::TextureHandle)index.internalPointer();

	if (index.column() == eColumn_RcSettings)
	{
		const int setting = GetSettings(m_pTextureManager->GetRcSettings(tex));
		if (role == Qt::DisplayRole)
		{
			return QString(GetDisplayNameOfSettings((ERcSettings)setting));
		}
		else if (role == Qt::EditRole)
		{
			return setting;
		}
	}
	else if (role == Qt::DisplayRole)
	{
		switch (index.column())
		{
		case eColumn_Filename:
			return QtUtil::ToQString(m_pTextureManager->GetSourcePath(tex));
		default:
			CRY_ASSERT(0 && "invalid index column");
		}
		;
	}
	else if (role == Qt::ToolTipRole && index.column() == eColumn_Filename)
	{
		const QString source = QtUtil::ToQString(m_pTextureManager->GetSourcePath(tex));
		const QString target = QtUtil::ToQString(m_pTextureManager->GetTargetPath(tex));
		return tr("Source file path: %1;\nTarget file path:%2").arg(source).arg(target);
	}

	return QVariant();
}

bool CTextureModel::setData(const QModelIndex& modelIndex, const QVariant& value, int role)
{
	if (modelIndex.column() == eColumn_RcSettings && role == Qt::EditRole && value.canConvert<int>())
	{
		CTextureManager::TextureHandle tex = (CTextureManager::TextureHandle)modelIndex.internalPointer();
		const string rcSetting = GetSettingsRcString((ERcSettings)value.toInt());
		m_pTextureManager->SetRcSettings(tex, rcSetting);
	}
	return false;
}

Qt::ItemFlags CTextureModel::flags(const QModelIndex& modelIndex) const
{
	if (modelIndex.column() == eColumn_RcSettings)
	{
		return Qt::ItemIsEditable | QAbstractItemModel::flags(modelIndex);
	}
	else
	{
		return QAbstractItemModel::flags(modelIndex);
	}
}

string CTextureModel::GetSettingsRcString(ERcSettings rcSettings) const
{
	switch (rcSettings)
	{
	case eRcSettings_Diffuse:
		return "/preset=Albedo";
	case eRcSettings_Bumpmap:
		return "/preset=Normals";
	case eRcSettings_Specular:
		return "/preset=Reflectance";
	default:
		CRY_ASSERT(0 && "Unkown rc settings");
		return "/preset=Albedo";
	}
}

int CTextureModel::GetSettings(const string& rcString) const
{
	if (rcString == GetSettingsRcString(eRcSettings_Bumpmap))
	{
		return eRcSettings_Bumpmap;
	}
	else if (rcString == GetSettingsRcString(eRcSettings_Specular))
	{
		return eRcSettings_Specular;
	}
	else
	{
		return eRcSettings_Diffuse;
	}
}

const char* GetDisplayNameOfSettings(CTextureModel::ERcSettings settings)
{
	switch(settings)
	{
	case CTextureModel::eRcSettings_Diffuse:
		return "Diffuse";
	case CTextureModel::eRcSettings_Bumpmap:
		return "Bumpmap";
	case CTextureModel::eRcSettings_Specular:
		return "Specular";
	default:
		assert(0 && "unkown rc setting");
		return "<unknown rc setting>";
	}
}

QVariant CTextureModel::headerData(int column, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole)
	{
		switch (column)
		{
		case eColumn_Filename:
			return QString("Texture");
		case eColumn_RcSettings:
			return QString("Type");
		default:
			CRY_ASSERT(0 && "unkown column");
			return QVariant();
		}
		;
	}
	return QVariant();
}

void CTextureModel::OnTexturesChanged()
{
	beginResetModel();
	endResetModel();
}

CTextureView::CTextureView(QWidget* pParent)
	: QAdvancedTreeView(QAdvancedTreeView::None, pParent)
	, m_pTextureModel(nullptr)
{
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, &QTreeView::customContextMenuRequested, this, &CTextureView::CreateContextMenu);
}

void CTextureView::SetModel(CTextureModel* pTextureModel)
{
	m_pTextureModel = pTextureModel;
	setModel(m_pTextureModel);

	m_pComboBoxDelegate.reset(new CComboBoxDelegate(this));
	m_pComboBoxDelegate->SetFillEditorFunction([this](QMenuComboBox* pEditor)
	{
		for (int i = 0; i < CTextureModel::eRcSettings_COUNT; ++i)
		{
		  pEditor->AddItem(GetDisplayNameOfSettings((CTextureModel::ERcSettings)i));
		}
	});
	setItemDelegateForColumn(CTextureModel::eColumn_RcSettings, m_pComboBoxDelegate.get());

	header()->resizeSection(0, 300);
}

void CTextureView::CreateContextMenu(const QPoint& point)
{
	const QPersistentModelIndex index = indexAt(point);
	if (!index.isValid())
	{
		return;
	}

	CTextureManager::TextureHandle tex = m_pTextureModel->GetTexture(index);

	QMenu* const pMenu = new QMenu(this);
	pMenu->addAction(tr("Convert to TIF"));
	// TODO: connect

	{
		QAction* pAction = pMenu->addAction(tr("Call RC"));
		connect(pAction, &QAction::triggered, [this, tex]()
		{
			const CTextureManager* const pTextureManager = m_pTextureModel->GetTextureManager();
			const string workingDir = pTextureManager->GetSourceDirectory();
			const string filename = PathUtil::Make(workingDir, PathUtil::ReplaceExtension(tex->targetFilePath, "tif"));

			CResourceCompilerHelper::InvokeResourceCompiler(filename, filename, true, true);
		});
	}

	const QPoint popupLocation = point + QPoint(1, 1); // Otherwise double-right-click immediately executes first option
	pMenu->popup(viewport()->mapToGlobal(popupLocation));
}

#include "DialogCommon.h"

static const char* GetPresetFromChannelType(FbxTool::EMaterialChannelType channelType)
{
	// TODO: The preset names are loaded from rc.ini. Since this file might be modified by
	// a user, the preset names should not be hard coded. Possibly use preset aliases for this.

	switch (channelType)
	{
	case FbxTool::eMaterialChannelType_Diffuse:
		return "Albedo";
	case FbxTool::eMaterialChannelType_Bump:
		return "Normals";
	case FbxTool::eMaterialChannelType_Specular:
		return "Reflectance";
	default:
		return nullptr;
	}
}

void Fill(const MeshImporter::CSceneManager& sceneManager, CTextureManager& texManager)
{
	const FbxTool::CScene* const pScene = sceneManager.GetScene();

	texManager.SetSourceDirectory(QtUtil::ToString(QFileInfo(sceneManager.GetImportFile()->GetOriginalFilePath()).dir().path()));
	texManager.ClearTextures();

	for (int i = 0; i < pScene->GetMaterialCount(); ++i)
	{
		const FbxTool::SMaterial* const pMaterial = pScene->GetMaterialByIndex(i);

		for (int j = 0; j < FbxTool::eMaterialChannelType_COUNT; ++j)
		{
			const FbxTool::SMaterialChannel& channel = pMaterial->m_materialChannels[j];
			if (channel.m_bHasTexture)
			{
				string rcArguments;
				const char* szPreset = GetPresetFromChannelType(FbxTool::EMaterialChannelType(j));
				if (szPreset)
				{
					rcArguments = string("/preset=") + szPreset;
				}

				texManager.AddTexture(channel.m_textureFilename, rcArguments);
			}
		}
	}
}
