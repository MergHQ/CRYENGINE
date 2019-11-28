// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryEntitySystem/IEntityBasicTypes.h>
#include "ItemString.h"
struct IMaterial;
struct ICharacterInstance;

// Manager to handle loading team specific player materials and 
// apply them to players + replay entities
class CTeamVisualizationManager 
{
private:
	enum EMaterialIndex
	{
		eMI_AliveFriendly = 0,
		eMI_AliveHostile,
		eMI_DeadFriendly,
		eMI_DeadHostile,

		eMI_Total
	};

	typedef std::vector<uint32> TCrcList;

	struct SModelMaterialSetup
	{
		SModelMaterialSetup();
		~SModelMaterialSetup();
		void SetMaterial(const EMaterialIndex idx, IMaterial* pMaterial);
		void ApplyMaterial(ICharacterInstance* pCharInst, const EMaterialIndex materialIdx) const;

		IMaterial* m_materials[eMI_Total]; 
		TCrcList m_attachments;
	};

	typedef unsigned int TModelNameCRC;  
	typedef std::map<TModelNameCRC, SModelMaterialSetup> TModelPartsMap;

public:
	CTeamVisualizationManager();
	~CTeamVisualizationManager();

	void Init(); 

	void OnPlayerTeamChange(const EntityId playerId) const; 
	void RefreshPlayerTeamMaterial(const EntityId playerId) const;
	void RefreshTeamMaterial( IEntity* pEntity, const bool isAlive, const bool isFriendly ) const;
	void ProcessTeamChangeVisualization(EntityId entityId) const;

private:

	ILINE EMaterialIndex GetMaterialIndex ( const bool isAlive, const bool isFriendly ) const { return (EMaterialIndex)((isAlive?0:2) + (isFriendly?0:1)); }
	void InitTeamVisualizationData(XmlNodeRef xmlNode); 

	TModelPartsMap m_teamVisualizationPartsMap;

};
