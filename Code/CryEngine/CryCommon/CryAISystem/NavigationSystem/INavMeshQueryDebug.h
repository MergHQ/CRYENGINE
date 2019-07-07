// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#ifdef NAV_MESH_QUERY_DEBUG
#include <CryAISystem/NavigationSystem/INavMeshQuery.h>

#ifdef NAV_MESH_QUERY_DEBUG
#include <CryUDR/InterfaceIncludes.h>
#endif // NAV_MESH_QUERY_DEBUG

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

		//! SBatchData stores the results of a single INavMeshQuery run execution.
		//! This can either be a complete execution from a CNavMeshQueryInstant query
		//! Or just one batch execution from a CNavMeshQueryBatch query.
		struct SBatchData
		{
			//! STriangleData stores required triangle data for debug purposes. Data has to be stored here to not rely on the NavMesh to ask for this data (passing the triangleId). 
			//! NavMesh may be regenerated and Tiles/Triangles may change, invalidating ids and/or data. 
			//! However, storing the data here allows us to debug and render it although the triangle ids may be invalid
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
				, triangleDataArray(INavMeshQueryDebug::GetTriangleData(pMesh, triangleIDArray))
			{
			}

			//! DebugDraws Batch information using UDR
			void DebugDraw() const
			{
				const ColorF trianglesColor = Col_Azure;

				Cry::UDR::CScope_FixedString batchesScope("Batches");
				{
					stack_string batchTimeText;
					batchTimeText.Format("Batch %lu \tSize: %lu \tTime: %2.5f ms", batchNumber, triangleDataArray.size(), elapsedTimeInMs);
					Cry::UDR::CScope_FixedString batchScope(batchTimeText.c_str());
					{
						for (size_t i = 0; i < triangleDataArray.size(); ++i)
						{
							const MNM::INavMeshQueryDebug::SBatchData::STriangleData triangleData = triangleDataArray[i];
							DebugDrawTriangle(triangleData.triangleID, triangleData.v0, triangleData.v1, triangleData.v2, trianglesColor);
						}
					}
				}
			}			
		};

		//! SBatchData stores the data provided when a NavMesh regeneration happen and that invalidates a INavMeshQuery.
		struct SInvalidationData
		{
			const TileID                  tileId;
			const MNM::Tile::STileData    tileData;
			const EInvalidationType       invalidationType;

			SInvalidationData(const TileID tileId_, const MNM::Tile::STileData tileData_, const EInvalidationType invalidationType_)
				: tileId(tileId_)
				, tileData(tileData_)
				, invalidationType(invalidationType_)
			{}

			void DebugDraw() const
			{
				const ColorF invalidTrianglesColor = Col_Red;
				stack_string scopeName;
				switch (invalidationType)
				{
				case INavMeshQueryDebug::EInvalidationType::NavMeshRegeneration:
					scopeName = "NavMesh Regeneration";
					break;
				case INavMeshQueryDebug::EInvalidationType::NavMeshAnnotationChanged:
					scopeName = "Annotation Changed";
					break;
				default:
					CRY_ASSERT(false, "New enum value has been added to MNM::NavMeshQueryDebug::InvalidationType: but it's not being handled in this switch statement");
				}

				Cry::UDR::CScope_FixedString invalidationsScope(scopeName.c_str());
				{
					Cry::UDR::CScope_UniqueStringAutoIncrement invalidationsScope("Invalidation ");

					for (size_t i = 0; i < tileData.trianglesCount; ++i)
					{
						const TileTriangleIndex triangleIdx(static_cast<TileTriangleIndex>(i));
						const MNM::TriangleID triangleId = ComputeTriangleID(tileId, triangleIdx);

						MNM::vector3_t v0;
						MNM::vector3_t v1;
						MNM::vector3_t v2;

						if (INavMeshQueryDebug::GetVertices(tileId, tileData, triangleId, v0, v1, v2))
						{
							DebugDrawTriangle(triangleId, v0, v1, v2, invalidTrianglesColor);
						}
					}
				}
			}
		};

		typedef DynArray<SBatchData> BatchDataArray;
		typedef DynArray<SInvalidationData> InvalidationDataArray;

		//! SQueryDebugData stores all required information to debug a INavMeshQuery (1:1 relationship)
		struct SQueryDebugData
		{
			CTimeValue             timeAtStart;
			size_t                 trianglesCount;
			float                  elapsedTimeTotalInMs;
			float                  elapsedTimeRunningInMs;
			BatchDataArray         batchHistory;
			InvalidationDataArray  invalidationsHistory;

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

	private:
		static void DebugDrawTriangle(const MNM::TriangleID triangleID, const MNM::vector3_t v0_, const MNM::vector3_t v1_, const MNM::vector3_t v2_, const ColorF& triangleFillColor)
		{
			stack_string triangleIdText;
			triangleIdText.Format("Id: %u", triangleID);
			Cry::UDR::CScope_FixedString triangleIdScope(triangleIdText.c_str());

			const Vec3 v0 = v0_.GetVec3();
			const Vec3 v1 = v1_.GetVec3();
			const Vec3 v2 = v2_.GetVec3();

			const ColorF triangleEdgeColor = Col_White;
			triangleIdScope->AddLine(v0, v1, triangleEdgeColor);
			triangleIdScope->AddLine(v1, v2, triangleEdgeColor);
			triangleIdScope->AddLine(v2, v0, triangleEdgeColor);
			triangleIdScope->AddTriangle(v0, v1, v2, triangleFillColor);

			const ColorF textColor = Col_White;
			const float  textSize = 1.25f;

			triangleIdScope->AddText((v0 + v1 + v2) / 3.f, textSize, textColor, "Id: %u", triangleID);
		}

		static bool GetVertices(const MNM::TileID tileId, const MNM::Tile::STileData& tileData, const MNM::TriangleID triangleId, MNM::vector3_t& v0, MNM::vector3_t& v1, MNM::vector3_t& v2)
		{
			if (tileId.IsValid())
			{
				if (triangleId.IsValid())
				{
					const uint16 triangleIdx = MNM::ComputeTriangleIndex(triangleId);

					v0 = MNM::Tile::Util::GetVertexPosWorldFixed(tileData, triangleIdx, 0);
					v1 = MNM::Tile::Util::GetVertexPosWorldFixed(tileData, triangleIdx, 1);
					v2 = MNM::Tile::Util::GetVertexPosWorldFixed(tileData, triangleIdx, 2);

					return true;
				}
			}

			return false;
		}

		//! Fill an array of STriangleData by asking the NavMesh with a TriangleId.
		//! This function should only run while the TriangleIds are still valid, otherwise the behavior is undefined. For this reason it will run in the ctor, that is executed right after processing the NavMesh Triangles.
		static MNM::INavMeshQueryDebug::SBatchData::TriangleDataArray GetTriangleData(const MNM::INavMesh* pMesh, const TriangleIDArray& triangleIDArray)
		{
			MNM::INavMeshQueryDebug::SBatchData::TriangleDataArray triangleDataArray;
			if (!pMesh)
			{
				return triangleDataArray;
			}

			triangleDataArray.reserve(triangleIDArray.size());
			for (const MNM::TriangleID triangleId : triangleIDArray)
			{
				MNM::vector3_t v0;
				MNM::vector3_t v1;
				MNM::vector3_t v2;

				MNM::Tile::STileData tileData;
				const MNM::TileID tileId = ComputeTileID(triangleId);
				pMesh->GetTileData(tileId, tileData);

				if (GetVertices(tileId, tileData, triangleId, v0, v1, v2))
				{
					triangleDataArray.emplace_back(MNM::INavMeshQueryDebug::SBatchData::STriangleData(triangleId, v0, v1, v2));
				}
				else
				{
					CRY_ASSERT(false, "Can't find the TileId from an existing NavMesh TriangleId. This shouldn't happen in NavMesh wasn't regenerated between the generation phase and this function.");
				}
			}
			return triangleDataArray;
		}
	};
}
#endif // NAV_MESH_QUERY_DEBUG