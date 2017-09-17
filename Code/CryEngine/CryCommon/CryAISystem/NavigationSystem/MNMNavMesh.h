#pragma once

#include "MNMTile.h"

namespace MNM
{

//! Structure used as output in navmesh raycast queries
struct SRayHitOutput
{
	Vec3 position;
	Vec3 normal2D;
	float distance;
};

namespace NavMesh
{

//! Mesh parameters.
struct SParams
{
	Vec3 originWorld;   //! Origin position of the NavMesh in the world coordinate space.
};

//! Filter functor for INavMesh::QueryTriangles.
struct IQueryTrianglesFilter
{
	virtual ~IQueryTrianglesFilter() {}

	enum class EResult : uint8
	{
		Rejected = 0,    //!< Triangle is rejected.
		Accepted = 1     //!< Triangle is accepted.
	};

	//! Called for each tested triangle.
	//! /param triangleId Id of tested triangle.
	//! /return Result of the filter.
	virtual EResult Check(TriangleID triangleId) = 0;

	// #MNM_TODO pavloi 2016.08.12: calling virtual function for each considered triangle is not nice.
	// Consider changing this interface into one, which accepts an array of triangles, something like:
	// virtual ResultBitmask_uint64 Check(const TriangleID triangleIds[64], const size_t count) = 0;
	// It will accept array with up to 64 triangles and return uint64 bitmask with filter results.
};

//! Returns the tile grid coordinate offset of the neighbor tile.
//! \param side Side of the tile on which neighbor tile is. Also, \see MNM::Tile::SLink::side.
//! \return Pointer to int[3] array with the (x,y,z) grid coordinate offset.
// #MNM_TODO pavloi 2016.08.15: represent side as an enum?
inline const int* GetNeighbourTileOffset(const size_t side)
{
	// #MNM_TODO pavloi 2016.08.15: is there a better solution than putting a static array into the inline function inside header file?
	static const int NeighbourOffset_MeshGrid[14][3] =
	{
		{ 1,  0,  0  },
		{ 1,  0,  1  },
		{ 1,  0,  -1 },

		{ 0,  1,  0  },
		{ 0,  1,  1  },
		{ 0,  1,  -1 },

		{ 0,  0,  1  },

		{ -1, 0,  0  },
		{ -1, 0,  -1 },
		{ -1, 0,  1  },

		{ 0,  -1, 0  },
		{ 0,  -1, -1 },
		{ 0,  -1, 1  },

		{ 0,  0,  -1 },
	};

	return NeighbourOffset_MeshGrid[side];
}

//! Returns the neighbor tile grid coordinate given the current tile gird coordinate and the neighbor side.
//! \param tileCoord Tile coordinate of the current tile.
//! \param side Side of the tile on which neighbor tile is. Also, \see MNM::Tile::SLink::side.
//! \return Tile grid coordinate of the neighbor tile.
// #MNM_TODO pavloi 2016.08.15: see comment about MNM::Tile::STileData::tileGridCoord about grid coord type
// #MNM_TODO pavloi 2016.08.15: produces negative coordinates - can be done better?
inline vector3_t GetNeighbourTileCoord(const vector3_t& tileCoord, const size_t side)
{
	const int* pOffset = GetNeighbourTileOffset(side);
	const vector3_t offset = vector3_t(real_t(pOffset[0]), real_t(pOffset[1]), real_t(pOffset[2]));
	return tileCoord + offset;
}

} // namespace NavMesh

//! Provides an access to the NavMesh data
struct INavMesh
{
	virtual ~INavMesh() {}

	//! Fills a NavMesh::SParams structure with a mesh parameters.
	virtual void GetMeshParams(NavMesh::SParams& outParams) const = 0;

	//! Finds a TileID by the tile's grid coordinate.
	//! \return Valid TileID, if such tile exists. Otherwise, return MNM::Constants::InvalidTileID.
	virtual TileID FindTileIDByTileGridCoord(const vector3_t& tileGridCoord) const = 0;

	//! Queries NavMesh triangles inside a bounding box.
	//! \param queryAabbWorld Bounding box for the query in a world coordinate space.
	//! \param pOptionalFilter Pointer to the optional triangle filter. If provided, then it's called for each triangleId.
	//! \param maxTrianglesCount Size of output buffer for result triangleId's.
	//! \param pOutTriangles Array buffer for result triangleId's.
	//! \return Count of found triangleId's.
	virtual size_t QueryTriangles(const aabb_t& queryAabbWorld, NavMesh::IQueryTrianglesFilter* pOptionalFilter, const size_t maxTrianglesCount, TriangleID* pOutTriangles) const = 0;

	//! Finds a single triangle closest to the point. /see QueryTriangles() for a way to get candidate triangles.
	//! \param queryPosWorld Query point in a world coordinate space.
	//! \param pCandidateTriangles Array of triangleID to search.
	//! \param candidateTrianglesCount Size of pCandidateTriangles array.
	//! \param pOutClosestPosWorld Optional pointer for a return value of a snapped to the triangle point in the world coordinate space.
	//! \param pOutClosestDistanceSq Optional pointer for a return value of squared distance between query point and a snapped to the triangle point.
	//! \return ID of a found closest triangle.
	virtual TriangleID FindClosestTriangle(const vector3_t& queryPosWorld, const TriangleID* pCandidateTriangles, const size_t candidateTrianglesCount, vector3_t* pOutClosestPosWorld, float* pOutClosestDistanceSq) const = 0;

	//! Fills a STileData structure for a read-only tile data access
	//! \param tileId Id of tile
	//! \param outTileData STileData structure to fill
	//! \return true if tile is found and outTileData is filled.
	virtual bool GetTileData(const TileID tileId, Tile::STileData& outTileData) const = 0;
};

} // namespace MNM
