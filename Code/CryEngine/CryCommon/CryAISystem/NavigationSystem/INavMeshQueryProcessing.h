// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAISystem/NavigationSystem/INavMeshQueryManager.h>
#include <CryAISystem/NavigationSystem/NavigationIdTypes.h>
#include <CryAISystem/NavigationSystem/INavMeshQueryFilter.h>

//! INavMeshQueryProcessing is a structure used to process triangles.
//! Mainly used in NavMeshQueries to apply a post-process operation on triangles after retrieving them from the NavMesh
//! but can also be used independently.
struct INavMeshQueryProcessing
{
	enum class EResult
	{
		Continue, // Continue processing with next set of triangles
		Stop,     // Processing is done and should be stopped
	};

	//! Clears the triangles that require processing
	virtual void    Clear() = 0;

	//! Returns the count of the triangles that require processing
	virtual size_t  Size() const = 0;

	//! Adds triangle to to be processed
	//! Triangle is not processed until Process() is called
	virtual void    AddTriangle(const MNM::TriangleID triangleId) = 0;

	//! Adds triangles array to to be processed
	//! Triangles are not processed until Process() is called
	virtual void    AddTriangleArray(const DynArray<MNM::TriangleID>& triangleIdArray) = 0;

	//! Executes the query processing against the queued triangles
	virtual EResult Process() = 0;
};