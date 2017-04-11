// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MetadataCompiler.h"
#include "Metadata/Metadata.h"
#include "Metadata/MetadataHelpers.h"
#include "FileUtil.h"
#include "StringHelpers.h"
#include "CGF/CGFLoader.h"

namespace MetadataCompiler_Private
{
using namespace AssetManager;

static string GetMetadataFilename(const string& filename)
{
	return string().Format("%s.%s", filename.c_str(), GetAssetExtension());
}

class CAsset
{
public:
	CAsset(IResourceCompiler* pRC) : m_pRC(pRC) {}

	virtual bool New()
	{
		m_xml = m_pRC->CreateXml(GetAssetExtension());
		return m_xml != nullptr;
	}

	virtual bool Read(const string& filename)
	{
		m_xml = IsXml(filename) ? m_pRC->LoadXml(filename) : nullptr;
		return m_xml != nullptr;
	}

	virtual bool Save(const string& filename)
	{
		return m_xml->saveToFile(filename);
	}

	//! Returns true if successful, otherwise returns false.
	virtual bool SetMetadata(const SAssetMetadata& metadata)
	{
		return AssetManager::WriteMetadata(m_xml, metadata);
	}

	//! Returns false if metadata does not exist or an error occurs, otherwise returns true.
	virtual bool GetMetadata(SAssetMetadata& metadata)
	{
		return AssetManager::ReadMetadata(m_xml, metadata);
	}

	//! Returns true if successful or metadata does not exist, otherwise returns false.
	virtual bool DeleteMetadata()
	{
		return AssetManager::DeleteMetadata(m_xml);
	}

private:
	static bool IsXml(const string& filename)
	{
		typedef std::unique_ptr<FILE, int (*)(FILE*)> file_ptr;

		file_ptr file(fopen(filename.c_str(), "rb"), fclose);

		if (!file.get())
			return false;

		static const size_t bufferSizeElements = 2048;
		char buffer[bufferSizeElements];
		while (size_t elementsRead = fread(buffer, sizeof(char), bufferSizeElements, file.get()))
		{
			for (size_t i = 0; i < elementsRead; ++i)
			{
				const int c = buffer[i];
				switch (buffer[i])
				{
				case '<':
					return true;
				case ' ':
					continue;
				case '\r':
					continue;
				case '\n':
					continue;
				case '\t':
					continue;
				default:
					return false;
				}
			}
		}
		;
		return false;
	}
private:
	IResourceCompiler* m_pRC;
	XmlNodeRef m_xml;
};

bool SaveAsset(CAsset* pAsset, const ConvertContext& context)
{
	const string sourceFile = context.GetSourcePath();
	const string targetFile = GetMetadataFilename(PathHelpers::Join(context.GetOutputFolder(), context.sourceFileNameOnly));
	if (!pAsset->Save(targetFile))
	{
		return false;
	}
	context.pRC->AddInputOutputFilePair(sourceFile, targetFile);
	return true;
}

std::vector<string> FindCgfAssetFiles(const string& filename)
{
	std::vector<string> files;

	// Find lods:
	// name.cgf
	// name_lod1.cgf
	// name_lod2.cgf
	// ...

	files.emplace_back(filename);

	const char* const szExt = PathUtil::GetExt(filename.c_str());
	const string name = PathUtil::RemoveExtension(filename.c_str());

	CRY_ASSERT(MAX_STATOBJ_LODS_NUM < 10);
	for (int i = 1; i < MAX_STATOBJ_LODS_NUM; ++i)
	{
		const string lodFilename = name + "_lod" + ('0' + i) + "." + szExt;
		if (!FileUtil::FileExists(lodFilename))
		{
			continue;
		}
		files.emplace_back(lodFilename);
	}

	return files;
}

std::vector<string> FindDdsAssetFiles(const string& filename)
{
	std::vector<string> files;

	// Find mips:
	// name.dds
	// name.dds.a
	// name.dds.1
	// name.dds.1a
	// ...

	int i = 0;
	for (string color = filename, alpha = color + ".a";
		FileUtil::FileExists(color);
		color = string().Format("%s.%i", filename.c_str(), ++i), alpha = color + "a")
	{
		files.emplace_back(color);

		if (FileUtil::FileExists(alpha))
		{
			files.emplace_back(alpha);
		}
	}
	return files;
}

bool IsCgfLod(const string& cgf)
{
	const char* const szExt = PathUtil::GetExt(cgf.c_str());
	const string filename(string(cgf).MakeLower());
	const char* const szLod = strstr(filename.c_str(), "_lod");
	return szLod && (szLod != filename.begin()) && FileUtil::FileExists(string(filename.begin(), szLod) + "." + szExt);
}

//! Removes the read-only attribute and deletes the file. Returns true if successful or file does not exist, otherwise returns false.
bool DeleteFileIfExists(const string filename)
{
	SetFileAttributesA(filename.c_str(), FILE_ATTRIBUTE_ARCHIVE);
	return (DeleteFileA(filename.c_str()) != 0) || (GetLastError() == ERROR_FILE_NOT_FOUND);
}

}

class CMetadataCompiler::CAssetFactory
{
public:
	CAssetFactory(IResourceCompiler* pRc, const IConfig* pConfig)
		: m_asset(pRc)
	{
	}

	MetadataCompiler_Private::CAsset* ReadAsset(const string& sourceFile)
	{
		using namespace MetadataCompiler_Private;

		const string metadataFilename = GetMetadataFilename(sourceFile);

		if (!FileUtil::FileExists(metadataFilename))
		{
			if (m_asset.New())
			{
				return &m_asset;
			}
		}
		else if (m_asset.Read(metadataFilename))
		{
			return &m_asset;
		}

		RCLogError("Can't read asset '%s'", sourceFile.c_str());
		return nullptr;
	}
private:
	MetadataCompiler_Private::CAsset m_asset;
};

CMetadataCompiler::CMetadataCompiler(IResourceCompiler* pRc)
	: m_pResourceCompiler(pRc)
{
}

CMetadataCompiler::~CMetadataCompiler()
{
}

void CMetadataCompiler::BeginProcessing(const IConfig* pConfig)
{
	m_pAssetFactory.reset(new CAssetFactory(m_pResourceCompiler, pConfig));
}

void CMetadataCompiler::EndProcessing()
{
}

bool CMetadataCompiler::Process()
{
	assert(m_pAssetFactory);

	using namespace MetadataCompiler_Private;

	if (m_CC.config->GetAsBool("stripMetadata", false, true))
	{
		return DeleteFileIfExists(GetMetadataFilename(m_CC.GetSourcePath()));
	}

	CAsset* pAsset = m_pAssetFactory->ReadAsset(m_CC.GetSourcePath());
	if (!pAsset)
	{
		return false;
	}

	SAssetMetadata metadata; 
	pAsset->GetMetadata(metadata);

	std::vector<string> files;

	const char* szExt = PathUtil::GetExt(m_CC.GetSourcePath().c_str());
	if (stricmp(szExt, "dds") == 0)
	{
		files = FindDdsAssetFiles(m_CC.GetSourcePath());
	}
	else if (!stricmp(szExt, "cgf") || !stricmp(szExt, "cga") || !stricmp(szExt, "skin"))
	{
		// Do not create cryassets for lod files.
		if (IsCgfLod(m_CC.GetSourcePath()))
		{
			return true;
		}

		files = FindCgfAssetFiles(m_CC.GetSourcePath());
	}
	else
	{
		files.emplace_back(m_CC.GetSourcePath());
	}

	InitMetadata(metadata, m_CC.config, "", files);

	return pAsset->SetMetadata(metadata) && SaveAsset(pAsset, m_CC);
}

const char* CMetadataCompiler::GetExt(int index) const
{
	using namespace MetadataCompiler_Private;

	switch (index)
	{
	case 0:
		return GetAssetExtension();
	default:
		return nullptr;
	}
}

namespace AssetManager
{
	std::vector<std::pair<string, string>> CollectCgfDetails(const CContentCGF& cgf)
	{
		size_t triangleCount = 0;
		size_t vertexCount = 0;

		for (int i = 0, n = cgf.GetNodeCount(); i < n; ++i)
		{
			const CNodeCGF* const pNode = cgf.GetNode(i);
			if (!pNode)
			{
				continue;
			}

			const CMesh* const pMesh = pNode->pMesh;
			if (pMesh)
			{
				triangleCount += std::max(pMesh->GetFaceCount(), pMesh->GetIndexCount() / 3); // Compiled meshes do not have faces.
				vertexCount += pMesh->GetVertexCount();
			}
		}

		std::vector<std::pair<string, string>> details;
		details.emplace_back("materialCount", string().Format("%i", cgf.GetMaterialCount()));
		details.emplace_back("triangleCount", string().Format("%i", triangleCount));
		details.emplace_back("vertexCount", string().Format("%i", vertexCount));

		const CSkinningInfo* pSkin = cgf.GetSkinningInfo();
		if (pSkin && !pSkin->m_arrBoneEntities.empty())
		{
			details.emplace_back("bonesCount", string().Format("%i", pSkin->m_arrBoneEntities.size()));
		}

		return details;
	}

	std::vector<std::pair<string, string>> CollectMetadataDetails(const char* szFilename)
	{
		CChunkFile chunkFile;
		CLoaderCGF cgfLoader;
		std::unique_ptr<CContentCGF> pCGF(cgfLoader.LoadCGF(szFilename, chunkFile, nullptr));
		if (!pCGF)
		{
			return{};
		}

		return CollectCgfDetails(*pCGF);
	}
}

