// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "RuntimeAreas.h"
#include "GameObjects/RuntimeAreaObject.h"
#include <IGameObject.h>

CRuntimeAreaManager::CRuntimeAreaManager()
{
	gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);

	IEntityClassRegistry::SEntityClassDesc runtimeObjectDesc;
	runtimeObjectDesc.sName = "RuntimeAreaObject";
	runtimeObjectDesc.sScriptFile = "";

	static IGameFramework::CGameObjectExtensionCreator<CRuntimeAreaObject> runtimeObjectCreator;
	gEnv->pGameFramework->GetIGameObjectSystem()->RegisterExtension(runtimeObjectDesc.sName, &runtimeObjectCreator, &runtimeObjectDesc);

	FillAudioControls();
}

///////////////////////////////////////////////////////////////////////////
CRuntimeAreaManager::~CRuntimeAreaManager()
{
	gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
}

///////////////////////////////////////////////////////////////////////////
void CRuntimeAreaManager::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
	case ESYSTEM_EVENT_LEVEL_UNLOAD:
		if (gEnv->IsEditor() == false)
			DestroyAreas();
		break;
	case ESYSTEM_EVENT_LEVEL_PRECACHE_END:
		if (gEnv->IsEditor() == false)
			CreateAreas();
		break;
	case ESYSTEM_EVENT_EDITOR_GAME_MODE_CHANGED:
	case ESYSTEM_EVENT_EDITOR_SIMULATION_MODE_CHANGED:
		if (wparam)
			CreateAreas();
		else
			DestroyAreas();
		break;
	}
}

char const* const CRuntimeAreaManager::SXMLTags::sMergedMeshSurfaceTypesRoot = "MergedMeshSurfaceTypes";
char const* const CRuntimeAreaManager::SXMLTags::sMergedMeshSurfaceTag = "SurfaceType";
char const* const CRuntimeAreaManager::SXMLTags::sAudioTag = "Audio";
char const* const CRuntimeAreaManager::SXMLTags::sNameAttribute = "name";
char const* const CRuntimeAreaManager::SXMLTags::sATLTriggerAttribute = "atl_trigger";
char const* const CRuntimeAreaManager::SXMLTags::sATLRtpcAttribute = "atl_rtpc";

///////////////////////////////////////////////////////////////////////////
void CRuntimeAreaManager::FillAudioControls()
{
	CryFixedStringT<1024> sXMLPath(PathUtil::GetGameFolder().c_str());
	sXMLPath.append("/libs/materialeffects/mergedmeshsurfacetypes.xml");

	XmlNodeRef const pXMLRoot(gEnv->pSystem->LoadXmlFromFile(sXMLPath));

	if (pXMLRoot && _stricmp(pXMLRoot->getTag(), SXMLTags::sMergedMeshSurfaceTypesRoot) == 0)
	{
		size_t const nSurfaceCount = static_cast<size_t>(pXMLRoot->getChildCount());

		for (size_t i = 0; i < nSurfaceCount; ++i)
		{
			XmlNodeRef const pSurfaceTypeNode(pXMLRoot->getChild(i));

			if (pSurfaceTypeNode && _stricmp(pSurfaceTypeNode->getTag(), SXMLTags::sMergedMeshSurfaceTag) == 0)
			{
				char const* const sSurfaceName = pSurfaceTypeNode->getAttr(SXMLTags::sNameAttribute);

				if ((sSurfaceName != NULL) && (sSurfaceName[0] != '\0'))
				{
					AudioControlId nTriggerID = INVALID_AUDIO_CONTROL_ID;
					AudioControlId nRtpcID = INVALID_AUDIO_CONTROL_ID;

					XmlNodeRef const pAudioNode(pSurfaceTypeNode->findChild(SXMLTags::sAudioTag));
					if (pAudioNode)
					{
						char const* const sATLTriggerName = pAudioNode->getAttr(SXMLTags::sATLTriggerAttribute);

						if ((sATLTriggerName != NULL) && (sATLTriggerName[0] != '\0'))
						{
							gEnv->pAudioSystem->GetAudioTriggerId(sATLTriggerName, nTriggerID);
						}

						char const* const sATLRtpcName = pAudioNode->getAttr(SXMLTags::sATLRtpcAttribute);

						if ((sATLRtpcName != NULL) && (sATLRtpcName[0] != '\0'))
						{
							gEnv->pAudioSystem->GetAudioRtpcId(sATLRtpcName, nRtpcID);
						}

						if ((nTriggerID != INVALID_AUDIO_CONTROL_ID) && (nRtpcID != INVALID_AUDIO_CONTROL_ID))
						{
							CRuntimeAreaObject::m_audioControls[CCrc32::ComputeLowercase(sSurfaceName)] =
							  CRuntimeAreaObject::SAudioControls(nTriggerID, nRtpcID);
						}
					}
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////
void CRuntimeAreaManager::DestroyAreas()
{
	IEntitySystem* const entity_system = gEnv->pEntitySystem;
	for (size_t i = 0, end = m_areaObjects.size(); i < end; entity_system->RemoveEntity(m_areaObjects[i++], true))
		;
	for (size_t i = 0, end = m_areas.size(); i < end; entity_system->RemoveEntity(m_areas[i++], true))
		;
	m_areaObjects.clear();
	m_areas.clear();
}

///////////////////////////////////////////////////////////////////////////
void CRuntimeAreaManager::CreateAreas()
{
	IMergedMeshesManager* mmrm_manager = gEnv->p3DEngine->GetIMergedMeshesManager();
	mmrm_manager->CalculateDensity();

	DynArray<IMergedMeshesManager::SMeshAreaCluster> clusters;
	mmrm_manager->CompileAreas(clusters, IMergedMeshesManager::CLUSTER_CONVEXHULL_GRAHAMSCAN);

	size_t const nClusterCount = clusters.size();
	m_areas.reserve(nClusterCount);
	m_areaObjects.reserve(nClusterCount);

	std::vector<Vec3> points;
	for (size_t i = 0; i < nClusterCount; ++i)
	{
		char szName[1024];
		cry_sprintf(szName, "RuntimeArea_%03d", (int)i);

		IMergedMeshesManager::SMeshAreaCluster& cluster = clusters[i];
		SEntitySpawnParams areaSpawnParams;
		areaSpawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("AreaShape");
		areaSpawnParams.nFlags = ENTITY_FLAG_VOLUME_SOUND | ENTITY_FLAG_CLIENT_ONLY | ENTITY_FLAG_NO_SAVE;
		areaSpawnParams.sLayerName = "global";
		areaSpawnParams.sName = szName;
		areaSpawnParams.vPosition = cluster.extents.GetCenter() - Vec3(0.0f, 0.0f, cluster.extents.GetSize().z * 0.5f);//??

		IEntityAreaProxy* pAreaProxy = NULL;
		IEntity* pNewAreaEntity = gEnv->pEntitySystem->SpawnEntity(areaSpawnParams);
		if (pNewAreaEntity)
		{
			EntityId const nAreaEntityID = pNewAreaEntity->GetId();

			pAreaProxy = static_cast<IEntityAreaProxy*>(pNewAreaEntity->CreateProxy(ENTITY_PROXY_AREA).get());
			if (pAreaProxy)
			{
				int const nPointCount = cluster.boundary_points.size();
				DynArray<bool> abObstructSound(nPointCount + 2, false);
				points.resize(nPointCount);
				for (size_t j = 0; j < points.size(); ++j)
					points[j] = Vec3(
					  cluster.boundary_points[j].x,
					  cluster.boundary_points[j].y,
					  areaSpawnParams.vPosition.z) - areaSpawnParams.vPosition;

				pAreaProxy->SetPoints(&points[0], &abObstructSound[0], points.size(), cluster.extents.GetSize().z);
				pAreaProxy->SetID(11001100 + i);
				pAreaProxy->SetPriority(100);
				pAreaProxy->SetGroup(110011);

				SEntitySpawnParams areaObjectSpawnParams;
				areaObjectSpawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("RuntimeAreaObject");
				areaObjectSpawnParams.nFlags = ENTITY_FLAG_CLIENT_ONLY | ENTITY_FLAG_NO_SAVE;
				areaObjectSpawnParams.sLayerName = "global";

				IEntity* pNewAreaObjectEntity = gEnv->pEntitySystem->SpawnEntity(areaObjectSpawnParams);

				if (pNewAreaObjectEntity != NULL)
				{
					EntityId const nAreaObjectEntityID = pNewAreaObjectEntity->GetId();

					pAreaProxy->AddEntity(nAreaObjectEntityID);

					m_areaObjects.push_back(nAreaObjectEntityID);
					m_areas.push_back(nAreaEntityID);
				}
				else
				{
					gEnv->pEntitySystem->RemoveEntity(nAreaEntityID, true);
				}
			}
		}
	}
}
