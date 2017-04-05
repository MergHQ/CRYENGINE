// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "FbxScene.h"
#include "PhysicsProxies.h"
#include "ProxyData.h"

#include <Cry3DEngine/I3DEngine.h>
#include <Cry3DEngine/IIndexedMesh.h>

CProxyData::CProxyData()
	: m_pFbxScene(nullptr)
{}

CProxyData::~CProxyData() {}

void CProxyData::SetScene(const FbxTool::CScene* pFbxScene)
{
	CRY_ASSERT(pFbxScene);

	m_physProxies.clear();

	m_physProxiesNodeMap.clear();
	m_physProxiesNodeMap.resize(pFbxScene->GetNodeCount());

	m_pFbxScene = pFbxScene;
}

SPhysProxies* CProxyData::NewPhysProxies()
{
	m_physProxies.emplace_back(new SPhysProxies());
	signalPhysProxiesCreated(m_physProxies.back().get());
	return m_physProxies.back().get();
}

void CProxyData::AddNodePhysProxies(const FbxTool::SNode* pFbxNode, SPhysProxies* pPhysProxies)
{
	m_physProxiesNodeMap[pFbxNode->id].push_back(pPhysProxies);
}

const SPhysProxies* CProxyData::GetPhysProxiesByIndex(const FbxTool::SNode* pFbxNode, int id) const
{
	return m_physProxiesNodeMap[pFbxNode->id][id];
}

int CProxyData::GetPhysProxiesCount(const FbxTool::SNode* pFbxNode) const
{
	return m_physProxiesNodeMap[pFbxNode->id].size();
}

void CProxyData::RemovePhysProxies(SPhysProxies* pPhysProxies)
{
	if (pPhysProxies)
	{
		signalPhysProxiesAboutToBeRemoved(pPhysProxies);

		for (auto& physGeom : pPhysProxies->proxyGeometries)
		{
			gEnv->pPhysicalWorld->GetGeomManager()->UnregisterGeometry(physGeom);
		}

		auto earseIt = std::remove_if(m_physProxies.begin(), m_physProxies.end(), [pPhysProxies](const std::unique_ptr<SPhysProxies>& other)
		{
			return other.get() == pPhysProxies;
		});
		m_physProxies.erase(earseIt, m_physProxies.end());

		// Remove proxies from node mapping.
		for (auto& nodeProxies : m_physProxiesNodeMap)
		{
			auto earseIt = std::remove(nodeProxies.begin(), nodeProxies.end(), pPhysProxies);
			nodeProxies.erase(earseIt, nodeProxies.end());
		}
	}
}

void CProxyData::RemovePhysGeometry(phys_geometry* pPhysGeom)
{
	if (pPhysGeom)
	{
		signalPhysGeometryAboutToBeRemoved(pPhysGeom);
		gEnv->pPhysicalWorld->GetGeomManager()->UnregisterGeometry(pPhysGeom);

		for (auto& physProxies : m_physProxies)
		{
			auto eraseIt = std::remove_if(physProxies->proxyGeometries.begin(), physProxies->proxyGeometries.end(), [pPhysGeom](phys_geometry* other)
			{
				return other == pPhysGeom;
			});
			if (eraseIt != physProxies->proxyGeometries.end())
			{
				physProxies->proxyGeometries.erase(eraseIt, physProxies->proxyGeometries.end());
			}
		}
	}
}

uint64 CProxyData::HasPhysProxies(const FbxTool::SNode* pFbxNode) const
{
	uint64 isles = 0;
	const SPhysProxies* pProx = 0;
	for (int i = 0; i < GetPhysProxiesCount(pFbxNode); ++i)
	{
		pProx = GetPhysProxiesByIndex(pFbxNode, i);
		isles |= pProx->params.islandMap;
	}
	int n = pProx ? pProx->pSrc->isleTriCount.size() : 1;
	return isles | -(int64)((1ull << n) - 1 == isles);
}

void LogPrintf(const char* szFormat, ...);

void CProxyData::WriteAutoGenProxies(const QString& cgfName)
{	
	std::string fname = cgfName.toStdString();
	_smart_ptr<IStatObj> pStatObj = gEnv->p3DEngine->FindStatObjectByFilename(fname.c_str());
	if (pStatObj)
		pStatObj->Refresh(FRO_GEOMETRY);
	else
		pStatObj = gEnv->p3DEngine->LoadStatObj(fname.c_str(), 0, 0, false);
	if (!pStatObj)
	{
		LogPrintf("Can't find the generated file %s\n", fname.c_str());
		return;
	}
	for (int i = 0; i < pStatObj->GetSubObjectCount(); i++)
		if (pStatObj->GetSubObject(i)->pStatObj)
			pStatObj->GetSubObject(i)->pStatObj->GetIndexedMesh(true);
	int nProxies = 0;
	if ((pStatObj = SaveProxies(pStatObj, m_pFbxScene->GetRootNode(), nProxies)) && nProxies)
	{
		LogPrintf("Adding %d auto-generated physics proxies\n", nProxies);
		pStatObj->SaveToCGF(fname.c_str());
	}
}

IStatObj* CProxyData::SaveProxies(IStatObj* pStatObj, const FbxTool::SNode* pFbxNode, int& nProxies, int slotParent)
{
	if (!m_pFbxScene->IsNodeIncluded(pFbxNode))
	{
		return pStatObj;
	}

	if (!m_physProxiesNodeMap[pFbxNode->id].empty())
	{
		for (SPhysProxies* const pPhysProxies : m_physProxiesNodeMap[pFbxNode->id])
		{
			for (auto m_pProxyGeom : pPhysProxies->proxyGeometries)
			{
				if (!(pStatObj->GetFlags() & STATIC_OBJECT_COMPOUND))
				{
					IStatObj* pMeshObj = pStatObj;
					pStatObj = gEnv->p3DEngine->CreateStatObj();
					pStatObj->FreeIndexedMesh();
					pStatObj->AddSubObject(pMeshObj);
				}
				IStatObj* pProxObj = gEnv->p3DEngine->CreateStatObj();
				static CMesh meshDummy;
				if (!meshDummy.GetVertexCount())
				{
					meshDummy.SetIndexCount(3);
					meshDummy.SetVertexCount(3);
					for (int i = 0; i < 3; i++)
					{
						meshDummy.m_bbox.Add(meshDummy.m_pPositions[i] = Vec3(1, 0, 0).GetRotated(Vec3(1, 1, 1) * (1 / sqrt3), i * gf_PI * 0.5f));
						meshDummy.m_pIndices[i] = i;
						meshDummy.m_pNorms[i] = SMeshNormal(Vec3(1, 1, 1) * (1 / sqrt3));
					}
					meshDummy.m_subsets.resize(1);
					SMeshSubset& ss = meshDummy.m_subsets[0];
					memset(&ss, 0, sizeof(ss));
					ss.nNumIndices = ss.nNumVerts = 3;
					ss.nPhysicalizeType = PHYS_GEOM_TYPE_NONE;
					ss.fRadius = 1;
				}
				pProxObj->GetIndexedMesh(true)->SetMesh(meshDummy);
				pProxObj->Invalidate(false);
				pProxObj->SetPhysGeom(m_pProxyGeom, m_pProxyGeom->nMats);
				m_pProxyGeom->nRefCount++;
				const char* name = slotParent >= 0 ? "$physics_proxy" : "";
				pProxObj->SetGeoName(name);
				IStatObj::SSubObject& subobj = pStatObj->AddSubObject(pProxObj);
				subobj.name = name;
				subobj.nParent = slotParent;
				nProxies++;
			}

			string name(pFbxNode->szName);
			for (slotParent = pStatObj->GetSubObjectCount() - 1; slotParent >= 0 && pStatObj->GetSubObject(slotParent)->name != name; slotParent--)
				;
			for (int i = 0; i < pFbxNode->numChildren; pStatObj = SaveProxies(pStatObj, pFbxNode->ppChildren[i++], nProxies, slotParent))
				;
		}
	}
	else // There is no physics proxy for this node.
	{
		string name(pFbxNode->szName);
		for (slotParent = pStatObj->GetSubObjectCount() - 1; slotParent >= 0 && pStatObj->GetSubObject(slotParent)->name != name; slotParent--)
			;
		for (int i = 0; i < pFbxNode->numChildren; pStatObj = SaveProxies(pStatObj, pFbxNode->ppChildren[i++], nProxies, slotParent))
			;
	}
	return pStatObj;
}
