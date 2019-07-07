// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SurfaceType.h"

#include "IEditorImpl.h"
#include "Material/MaterialManager.h"
#include "Terrain/TerrainManager.h"
#include "Vegetation/VegetationMap.h"
#include "Vegetation/VegetationObject.h"

#pragma push_macro("GetObject")
#undef GetObject

const int CSurfaceType::ms_maxSurfaceTypeIdCount = static_cast<int>(LayerIdConstants::e_layerIdUndefined);

CSurfaceType::CSurfaceType()
{
	m_detailScale[0] = 1.0f;
	m_detailScale[1] = 1.0f;
	m_projAxis = ESFT_Z;
	m_surfaceTypeID = LayerIdConstants::e_layerIdUndefined;
}

void CSurfaceType::Serialize(CXmlArchive& xmlAr)
{
	Serialize(xmlAr.root, xmlAr.bLoading);
}

void CSurfaceType::Serialize(XmlNodeRef xmlRootNode, bool boLoading)
{
	XmlNodeRef sfType = xmlRootNode;

	if (boLoading)
	{
		sfType->getAttr("Name", m_name);
		sfType->getAttr("SurfaceTypeID", m_surfaceTypeID);
		sfType->getAttr("DetailTexture", m_detailTexture);
		sfType->getAttr("DetailScaleX", m_detailScale[0]);
		sfType->getAttr("DetailScaleY", m_detailScale[1]);
		sfType->getAttr("DetailMaterial", m_material);
		sfType->getAttr("ProjectAxis", m_projAxis);
		sfType->getAttr("Bumpmap", m_bumpmap);

		if (!m_detailTexture.IsEmpty() && m_material.IsEmpty())
		{
			// For backward compatibility create a material for this detail texture.
			CMaterial* pMtl = GetIEditorImpl()->GetMaterialManager()->CreateMaterial(PathUtil::RemoveExtension(m_detailTexture.GetString()), XmlNodeRef(), 0);
			pMtl->AddRef();
			pMtl->SetShaderName("Terrain.Layer");
			pMtl->GetShaderResources().m_Textures[EFTT_DIFFUSE].m_Name = m_detailTexture;
			pMtl->Update();
			m_material = pMtl->GetName();
		}
	}
	else
	{
		sfType->setAttr("Name", m_name);
		sfType->setAttr("SurfaceTypeID", m_surfaceTypeID);
		sfType->setAttr("DetailTexture", m_detailTexture);
		sfType->setAttr("DetailScaleX", m_detailScale[0]);
		sfType->setAttr("DetailScaleY", m_detailScale[1]);
		sfType->setAttr("DetailMaterial", m_material);
		sfType->setAttr("ProjectAxis", m_projAxis);
		sfType->setAttr("Bumpmap", m_bumpmap);

		switch (m_projAxis)
		{
		case 0:
			sfType->setAttr("ProjAxis", "X");
			break;
		case 1:
			sfType->setAttr("ProjAxis", "Y");
			break;
		case 2:
		default:
			sfType->setAttr("ProjAxis", "Z");
		}

		SaveVegetationIds(sfType);
	}
}

void CSurfaceType::SaveVegetationIds(XmlNodeRef& node)
{
	CVegetationMap* pVegMap = GetIEditorImpl()->GetVegetationMap();
	if (!pVegMap)
	{
		return;
	}

	// Go through all vegetation groups, and see who uses us.
	bool bExport = false;
	if (node && node->getParent() && node->getParent()->getTag())
	{
		string pTag = node->getParent()->getTag();
		bExport = (pTag == "SurfaceTypes");
	}

	if (bExport)
	{
		DynArray<IStatInstGroup*> statInstGroupTable;

		if (ITerrain* pTerrain = GetIEditorImpl()->Get3DEngine()->GetITerrain())
		{
			uint32 nObjTypeMask = 1 << eERType_Vegetation | 1 << eERType_MergedMesh;

			pTerrain->GetStatObjAndMatTables(NULL, NULL, &statInstGroupTable, nObjTypeMask);

			for (int i = 0; i < statInstGroupTable.size(); ++i)
			{
				IStatInstGroup* p = statInstGroupTable[i];
				for (int j = 0; j < pVegMap->GetObjectCount(); ++j)
				{
					CVegetationObject* pObject = pVegMap->GetObject(j);
					if (pObject->GetId() == p->nID)
					{
						if (pObject->IsUsedOnTerrainLayer(m_name))
						{
							XmlNodeRef nodeVeg = node->newChild("VegetationGroup");
							nodeVeg->setAttr("GUID", pObject->GetGUID());
							nodeVeg->setAttr("Id", pObject->GetId());
							break;
						}
					}
				}
			}
		}
	}
	else
	{
		for (int i = 0; i < pVegMap->GetObjectCount(); i++)
		{
			CVegetationObject* pObject = pVegMap->GetObject(i);
			if (pObject->IsUsedOnTerrainLayer(m_name))
			{
				XmlNodeRef nodeVeg = node->newChild("VegetationGroup");
				nodeVeg->setAttr("Id", pObject->GetId());
				nodeVeg->setAttr("GUID", pObject->GetGUID());
			}
		}
	}
}

void CSurfaceType::AssignUnusedSurfaceTypeID()
{
	// Inefficient function to find a free unused surface type id.

	const CTerrainManager* pManager = GetIEditorImpl()->GetTerrainManager();

	const int idCount = pManager->GetSurfaceTypeCount();

	std::vector<int> ids;
	ids.reserve(idCount);

	for (int i = 0; i < idCount; i++)
	{
		CSurfaceType* pSrfType = pManager->GetSurfaceTypePtr(i);
		ids.push_back(pSrfType->GetSurfaceTypeID());
	}
	std::sort(ids.begin(), ids.end());

	int nID = LayerIdConstants::e_layerIdUndefined;
	for (int i = 0; i < idCount; i++)
	{
		int j;
		for (j = 0; j < idCount; j++)
		{
			if (i == ids[j])
				break;
		}
		if (j == idCount)
		{
			nID = i;
			break;
		}
	}
	if (nID >= LayerIdConstants::e_layerIdUndefined)
	{
		nID = idCount < LayerIdConstants::e_layerIdUndefined ? idCount : LayerIdConstants::e_layerIdUndefined;
	}

	m_surfaceTypeID = nID;
}

#pragma pop_macro("GetObject")