// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   CloudsManager.h
//  Version:     v1.00
//  Created:     15/2/2005 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __CloudsManager_h__
#define __CloudsManager_h__
#pragma once

class CCloudRenderNode;

typedef std::vector<SCloudParticle*> CloudParticles;

struct SCloudQuadTree
{
	CloudParticles  m_particles;
	AABB            m_bounds;
	SCloudQuadTree* m_pQuads[4];
	int             m_level;
	SCloudQuadTree(int level = 0)
	{
		m_pQuads[0] = 0;
		m_pQuads[1] = 0;
		m_pQuads[2] = 0;
		m_pQuads[3] = 0;
		m_level = level;
	}
	~SCloudQuadTree()
	{
		delete m_pQuads[0];
		delete m_pQuads[1];
		delete m_pQuads[2];
		delete m_pQuads[3];
		m_pQuads[0] = m_pQuads[1] = m_pQuads[2] = m_pQuads[3] = 0;
	}
	void Init(const AABB& bounds, const CloudParticles& particles, int maxlevel = 2);
	bool CheckIntersection(const Vec3& p1, const Vec3& p2);
};

//////////////////////////////////////////////////////////////////////////
// SCloudDescription contains cached representation of the cloud description file.
//////////////////////////////////////////////////////////////////////////
struct SCloudDescription : public _reference_target_t, public Cry3DEngineBase
{
	string                      filename;
	int                         m_textureRows;
	int                         m_textureCols;
	int                         m_numSprites;

	AABB                        m_bounds;
	Vec3                        m_offset;

	_smart_ptr<IMaterial>       m_pMaterial;
	std::vector<SCloudParticle> m_particles;
	SCloudQuadTree*             m_pCloudTree;

	SCloudDescription()
	{
		m_textureRows = 0;
		m_textureCols = 0;
		m_numSprites = 0;
		m_pCloudTree = 0;
	};
	~SCloudDescription();
};

//////////////////////////////////////////////////////////////////////////
// CloudsManager is used to manage cloud descriptions loaded from the files.
// When cloud file is once loaded it caches its content and next time the same
// cloud file is request, Clients will get the cached content.
//////////////////////////////////////////////////////////////////////////
class CCloudsManager : public Cry3DEngineBase
{
public:
	CCloudsManager() {};
	~CCloudsManager() {};

	// Loads cloud file and returns cloud description.
	// If cloud was already loaded cached instance is returned.
	// Reference count of the cloud description is incremented,Client must call Release on returned pointer to free cloud description.
	SCloudDescription* LoadCloud(const char* sFilename);

	// Used to parse xml node and create a cloud description from it.
	void ParseCloudFromXml(XmlNodeRef node, SCloudDescription* pCloud);

	void AddCloudRenderNode(CCloudRenderNode* pNode);
	void RemoveCloudRenderNode(CCloudRenderNode* pNode);
	bool CheckIntersectClouds(const Vec3& p1, const Vec3& p2);
	void MoveClouds();

private:
	friend struct SCloudDescription;
	void Register(SCloudDescription* desc);
	void Unregister(SCloudDescription* desc);

private:
	typedef std::map<string, SCloudDescription*, stl::less_stricmp<string>> CloudsMaps;

	CloudsMaps                     m_cloudsMap;
	std::vector<CCloudRenderNode*> m_cloudNodes;
};

#endif // __CloudsManager_h__
