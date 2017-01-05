// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryMath/Cry_Geo.h>

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// IDebugRenderWorld
		//
		//===================================================================================

		struct IDebugRenderWorld
		{
			virtual        ~IDebugRenderWorld() {}

			virtual void   DrawSphere(const Vec3& pos, float radius, const ColorF& color) const = 0;
			virtual void   DrawDirection(const Vec3& from, const Vec3& to, float coneRadius, float coneHeight, const ColorF& color) const = 0;
			virtual void   DrawLine(const Vec3& pos1, const Vec3& pos2, const ColorF& color) const = 0;
			virtual void   DrawCone(const Vec3& pos, const Vec3& dir, float baseRadius, float height, const ColorF& color) const = 0;
			virtual void   DrawCylinder(const Vec3& pos, const Vec3& dir, float radius, float height, const ColorF& color) const = 0;
			virtual void   DrawText(const Vec3& pos, float size, const ColorF& color, const char* fmt, ...) const PRINTF_PARAMS(5, 6) = 0;
			virtual void   DrawQuat(const Vec3& pos, const Quat& q, float r, const ColorF& color) const = 0;
			virtual void   DrawAABB(const AABB& aabb, const ColorF& color) const = 0;
			virtual void   DrawOBB(const OBB& obb, const ColorF& color) const = 0;

			virtual void   AddSphere(const Vec3& pos, float radius, const ColorF& color) = 0;
			virtual void   AddDirection(const Vec3& from, const Vec3& to, float coneRadius, float coneHeight, const ColorF& color) = 0;
			virtual void   AddLine(const Vec3& pos1, const Vec3& pos2, const ColorF& color) = 0;
			virtual void   AddCone(const Vec3& pos, const Vec3& dir, float baseRadius, float height, const ColorF& color) = 0;
			virtual void   AddCylinder(const Vec3& pos, const Vec3& dir, float radius, float height, const ColorF& color) = 0;
			virtual void   AddText(const Vec3& pos, float size, const ColorF& color, const char* fmt, ...) PRINTF_PARAMS(5, 6) = 0;
			virtual void   AddQuat(const Vec3& pos, const Quat& q, float r, const ColorF& color) = 0;
			virtual void   AddAABB(const AABB& aabb, const ColorF& color) = 0;
			virtual void   AddOBB(const OBB& obb, const ColorF& color) = 0;
		};

	}
}
