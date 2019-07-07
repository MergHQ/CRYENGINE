// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EditableAsset.h"
#include "Asset.h"

#include <CryString/CryPath.h>

CEditableAsset::CEditableAsset(CAsset& asset)
	: m_asset(asset)
{
}

CEditableAsset::CEditableAsset(const CEditableAsset& other)
	: m_asset(other.m_asset)
{
}

CAsset& CEditableAsset::GetAsset()
{
	return m_asset;
}

bool CEditableAsset::WriteToFile()
{
	return m_asset.WriteToFile();
}

void CEditableAsset::SetName(const char* szName)
{
	m_asset.SetName(szName);
}

void CEditableAsset::SetLastModifiedTime(uint64 t)
{
	m_asset.SetLastModifiedTime(t);
}

void CEditableAsset::SetMetadataFile(const char* szFilepath)
{
	m_asset.SetMetadataFile(szFilepath);
}

void CEditableAsset::SetSourceFile(const char* szFilepath)
{
	m_asset.SetSourceFile(szFilepath);
}

void CEditableAsset::SetFiles(const std::vector<string>& filenames)
{
	m_asset.SetFiles(filenames);
}

void CEditableAsset::AddFile(const char* szFilepath)
{
	m_asset.AddFile(szFilepath);
}

void CEditableAsset::SetWorkFiles(const std::vector<string>& filenames)
{
	m_asset.SetWorkFiles(filenames);
}

void CEditableAsset::AddWorkFile(const char* szFilepath)
{
	m_asset.AddWorkFile(szFilepath);
}

void CEditableAsset::SetDetails(const std::vector<std::pair<string, string>>& details)
{
	for (const std::pair<string, string>& detail : details)
	{
		m_asset.SetDetail(detail.first, detail.second);
	}
}

void CEditableAsset::SetDependencies(const std::vector<SAssetDependencyInfo>& dependencies)
{
	m_asset.SetDependencies(dependencies);
}

void CEditableAsset::InvalidateThumbnail()
{
	m_asset.InvalidateThumbnail();
}

void CEditableAsset::SetOpenedInAssetEditor(CAssetEditor* pEditor)
{
	m_asset.OnOpenedInEditor(pEditor);
}

const char* CEditableAsset::GetMetadataFile() const
{
	return m_asset.GetMetadataFile();
}

const char* CEditableAsset::GetFolder() const 
{
	return m_asset.GetFolder();
}

const char* CEditableAsset::GetName() const
{
	return m_asset.GetName();
}
