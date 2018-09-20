// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#ifdef NAV_MESH_QUERY_DEBUG
#include <CryAISystem/NavigationSystem/INavMeshQuery.h>

namespace MNM
{
	struct INavMesh;

	struct INavMeshQueryDebug
	{
		enum class EInvalidationType
		{
			NavMeshRegeneration,
			NavMeshAnnotationChanged
		};

		struct SBatchData
		{
			//! Store required triangle data for debug purposes. Data has to be stored here to not rely on the NavMesh to ask for this data (passing the triangleId). NavMesh may be regenerated and Tiles/Triangles may change, invalidating ids and/or data. However, if we store the data here, although the triangle ids may not be valid anymore, we will still have all the data (ie: vertices) to properly debug them.
			struct STriangleData
			{
				const TriangleID triangleID;
				const vector3_t  v0;
				const vector3_t  v1;
				const vector3_t  v2;

				STriangleData(const TriangleID triangleID_, const vector3_t v0_, const vector3_t v1_, const vector3_t v2_)
					: triangleID(triangleID_)
					, v0(v0_)
					, v1(v1_)
					, v2(v2_)
				{
				}
			};

			typedef size_t NavMeshQueryDebugBatchId;
			typedef DynArray<STriangleData> TriangleDataArray;

			const NavMeshQueryDebugBatchId batchNumber;
			const size_t                   batchSize;
			const TriangleDataArray        triangleDataArray;
			const float                    elapsedTimeInMs;

			SBatchData(const NavMeshQueryDebugBatchId batchNumber_,
				const size_t batchSize_,
				const float elapsedTimeInMs_,
				const INavMesh* pMesh,
				const TriangleIDArray& triangleIDArray)
				: batchNumber(batchNumber_)
				, batchSize(batchSize_)
				, elapsedTimeInMs(elapsedTimeInMs_)
				, triangleDataArray(GetTriangleData(pMesh, triangleIDArray))
			{
			}

		private:
			//! Fill an array of STriangleData by asking the NavMesh with a TriangleId.
			//! This function should only run while the TriangleIds are still valid, otherwise the behavior is undefined. For this reason it will run in the ctor, that is executed right after processing the NavMesh Triangles.
			INavMeshQueryDebug::SBatchData::TriangleDataArray GetTriangleData(const INavMesh* pMesh, const TriangleIDArray& triangleIDArray) const
			{
				INavMeshQueryDebug::SBatchData::TriangleDataArray triangleDataArray;
				if (!pMesh)
				{
					return triangleDataArray;
				}

				triangleDataArray.reserve(triangleIDArray.size());
				for (const TriangleID triangleID : triangleIDArray)
				{
					vector3_t v0;
					vector3_t v1;
					vector3_t v2;

					if (GetVertices(pMesh, triangleID, v0, v1, v2))
					{
						triangleDataArray.emplace_back(INavMeshQueryDebug::SBatchData::STriangleData(triangleID, v0, v1, v2));
					}
					else
					{
						CRY_ASSERT_MESSAGE(false, "Can't find the TileId from an existing NavMesh TriangleId. This shouldn't happen in NavMesh wasn't regenerated between the generation phase and this function.");
					}
				}
				return triangleDataArray;
			}

			bool GetVertices(const INavMesh* pMesh, const TriangleID triangleID, vector3_t& v0, vector3_t& v1, vector3_t& v2) const
			{
				if (const TileID tileId = ComputeTileID(triangleID))
				{
					Tile::STileData tileData;
					pMesh->GetTileData(tileId, tileData);

					const uint16 triangleIdx = ComputeTriangleIndex(triangleID);

					v0 = Tile::Util::GetVertexPosWorldFixed(tileData, triangleIdx, 0);
					v1 = Tile::Util::GetVertexPosWorldFixed(tileData, triangleIdx, 1);
					v2 = Tile::Util::GetVertexPosWorldFixed(tileData, triangleIdx, 2);

					return true;
				}

				return false;
			}
		};

		struct SInvalidationData
		{
			const TileID                  tileID;
			const CTimeValue              timeAtInvalidation;
			const EInvalidationType       invalidationType;

			SInvalidationData(const TileID tileID_, const CTimeValue& timeOnInvalidation_, const EInvalidationType invalidationType_)
				: tileID(tileID_)
				, timeAtInvalidation(timeOnInvalidation_)
				, invalidationType(invalidationType_)
			{}
		};

		typedef DynArray<SBatchData> BatchDataArray;
		typedef DynArray<SInvalidationData> InvalidationDataArray;

		struct SQueryDebugData
		{
			CTimeValue             timeAtStart;
			size_t                 trianglesCount;
			float                  elapsedTimeTotalInMs;
			float                  elapsedTimeRunningInMs;
			BatchDataArray        batchHistory;
			InvalidationDataArray invalidationsHistory;

			SQueryDebugData()
				: timeAtStart(0.f)
				, trianglesCount(0)
				, elapsedTimeTotalInMs(0.f)
				, elapsedTimeRunningInMs(0.f)
			{}
		};

		virtual const INavMeshQuery&   GetQuery() const = 0;
		virtual const SQueryDebugData& GetDebugData() const = 0;
		virtual bool                   AddBatchToHistory(const SBatchData& queryBatch) = 0;
		virtual bool                   AddInvalidationToHistory(const SInvalidationData& queryInvalidation) = 0;
	protected:
		~INavMeshQueryDebug() {}
	};
}
#endif // NAV_MESH_QUERY_DEBUG