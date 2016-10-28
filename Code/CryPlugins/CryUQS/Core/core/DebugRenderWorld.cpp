// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "DebugRenderWorld.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// CDebugRenderWorld::SDebugRenderPrimitiveWithItemAssociation
		//
		//===================================================================================

		CDebugRenderWorld::SDebugRenderPrimitiveWithItemAssociation::SDebugRenderPrimitiveWithItemAssociation()
			: pPrimitive()
			, associatedItemIndex(kItemIndexWithoutAssociation)
		{}

		CDebugRenderWorld::SDebugRenderPrimitiveWithItemAssociation::SDebugRenderPrimitiveWithItemAssociation(const SDebugRenderPrimitiveWithItemAssociation& other)
			: pPrimitive()   // bypass the unique_ptr
			, associatedItemIndex(other.associatedItemIndex)
		{}

		CDebugRenderWorld::SDebugRenderPrimitiveWithItemAssociation::SDebugRenderPrimitiveWithItemAssociation(std::unique_ptr<CDebugRenderPrimitiveBase>& _primitive, size_t _associatedItemIndex)
			: pPrimitive(std::move(_primitive))
			, associatedItemIndex(_associatedItemIndex)
		{}

		CDebugRenderWorld::SDebugRenderPrimitiveWithItemAssociation::SDebugRenderPrimitiveWithItemAssociation(SDebugRenderPrimitiveWithItemAssociation&& other)
			: pPrimitive(std::move(other.pPrimitive))
			, associatedItemIndex(other.associatedItemIndex)
		{}

		CDebugRenderWorld::SDebugRenderPrimitiveWithItemAssociation& CDebugRenderWorld::SDebugRenderPrimitiveWithItemAssociation::operator=(const SDebugRenderPrimitiveWithItemAssociation& other)
		{
			if (this != &other)
			{
				// bypass .pPrimitive due to its unique_ptr
				associatedItemIndex = other.associatedItemIndex;
			}
			return *this;
		}

		void CDebugRenderWorld::SDebugRenderPrimitiveWithItemAssociation::Serialize(Serialization::IArchive& ar)
		{
			ar(pPrimitive, "pPrimitive");
			ar(associatedItemIndex, "associatedItemIndex");
		}

		//===================================================================================
		//
		// CDebugRenderWorld
		//
		//===================================================================================

		CDebugRenderWorld::CDebugRenderWorld()
			: m_indexOfCurrentlyEvaluatedItem(kItemIndexWithoutAssociation)
		{
			// nothing
		}

		void CDebugRenderWorld::DrawSphere(const Vec3& pos, float radius, const ColorF& color) const
		{
			CDebugRenderPrimitive_Sphere::Draw(pos, radius, color, false);
		}

		void CDebugRenderWorld::DrawDirection(const Vec3& pos, float radius, const Vec3& dir, const ColorF& color) const
		{
			CDebugRenderPrimitive_Direction::Draw(pos, radius, dir, color, false);
		}

		void CDebugRenderWorld::DrawLine(const Vec3& pos1, const Vec3& pos2, const ColorF& color) const
		{
			CDebugRenderPrimitive_Line::Draw(pos1, pos2, color, false);
		}

		void CDebugRenderWorld::DrawCone(const Vec3& pos, const Vec3& dir, float baseRadius, float height, const ColorF& color) const
		{
			CDebugRenderPrimitive_Cone::Draw(pos, dir, baseRadius, height, color, false);
		}

		void CDebugRenderWorld::DrawCylinder(const Vec3& pos, const Vec3& dir, float radius, float height, const ColorF& color) const
		{
			CDebugRenderPrimitive_Cylinder::Draw(pos, dir, radius, height, color, false);
		}

		void CDebugRenderWorld::DrawText(const Vec3& pos, float size, const ColorF& color, const char* fmt, ...) const
		{
			va_list ap;
			char text[1024];

			va_start(ap, fmt);
			cry_vsprintf(text, fmt, ap);
			va_end(ap);

			CDebugRenderPrimitive_Text::Draw(pos, size, text, color, false);
		}

		void CDebugRenderWorld::DrawQuat(const Vec3& pos, const Quat& q, float r, const ColorF& color) const
		{
			CDebugRenderPrimitive_Quat::Draw(pos, q, r, color, false);
		}

		void CDebugRenderWorld::DrawAABB(const AABB& aabb, const ColorF& color) const
		{
			CDebugRenderPrimitive_AABB::Draw(aabb, color, false);
		}

		void CDebugRenderWorld::DrawOBB(const OBB& obb, const ColorF& color) const
		{
			CDebugRenderPrimitive_OBB::Draw(obb, color, false);
		}

		void CDebugRenderWorld::AddSphere(const Vec3& pos, float radius, const ColorF& color)
		{
			std::unique_ptr<CDebugRenderPrimitiveBase> p(new CDebugRenderPrimitive_Sphere(pos, radius, color));
			m_primitives.emplace_back(p, m_indexOfCurrentlyEvaluatedItem);
		}

		void CDebugRenderWorld::AddDirection(const Vec3& pos, float radius, const Vec3& dir, const ColorF& color)
		{
			std::unique_ptr<CDebugRenderPrimitiveBase> p(new CDebugRenderPrimitive_Direction(pos, radius, dir, color));
			m_primitives.emplace_back(p, m_indexOfCurrentlyEvaluatedItem);
		}

		void CDebugRenderWorld::AddLine(const Vec3& pos1, const Vec3& pos2, const ColorF& color)
		{
			std::unique_ptr<CDebugRenderPrimitiveBase> p(new CDebugRenderPrimitive_Line(pos1, pos2, color));
			m_primitives.emplace_back(p, m_indexOfCurrentlyEvaluatedItem);
		}

		void CDebugRenderWorld::AddCone(const Vec3& pos, const Vec3& dir, float baseRadius, float height, const ColorF& color)
		{
			std::unique_ptr<CDebugRenderPrimitiveBase> p(new CDebugRenderPrimitive_Cone(pos, dir, baseRadius, height, color));
			m_primitives.emplace_back(p, m_indexOfCurrentlyEvaluatedItem);
		}

		void CDebugRenderWorld::AddCylinder(const Vec3& pos, const Vec3& dir, float radius, float height, const ColorF& color)
		{
			std::unique_ptr<CDebugRenderPrimitiveBase> p(new CDebugRenderPrimitive_Cylinder(pos, dir, radius, height, color));
			m_primitives.emplace_back(p, m_indexOfCurrentlyEvaluatedItem);
		}

		void CDebugRenderWorld::AddText(const Vec3& pos, float size, const ColorF& color, const char* fmt, ...)
		{
			va_list ap;
			char text[1024];

			va_start(ap, fmt);
			cry_vsprintf(text, fmt, ap);
			va_end(ap);

			std::unique_ptr<CDebugRenderPrimitiveBase> p(new CDebugRenderPrimitive_Text(pos, size, text, color));
			m_primitives.emplace_back(p, m_indexOfCurrentlyEvaluatedItem);
		}

		void CDebugRenderWorld::AddQuat(const Vec3& pos, const Quat& q, float r, const ColorF& color)
		{
			std::unique_ptr<CDebugRenderPrimitiveBase> p(new CDebugRenderPrimitive_Quat(pos, q, r, color));
			m_primitives.emplace_back(p, m_indexOfCurrentlyEvaluatedItem);
		}

		void CDebugRenderWorld::AddAABB(const AABB& aabb, const ColorF& color)
		{
			std::unique_ptr<CDebugRenderPrimitiveBase> p(new CDebugRenderPrimitive_AABB(aabb, color));
			m_primitives.emplace_back(p, m_indexOfCurrentlyEvaluatedItem);
		}

		void CDebugRenderWorld::AddOBB(const OBB& obb, const ColorF& color)
		{
			std::unique_ptr<CDebugRenderPrimitiveBase> p(new CDebugRenderPrimitive_OBB(obb, color));
			m_primitives.emplace_back(p, m_indexOfCurrentlyEvaluatedItem);
		}

		void CDebugRenderWorld::AssociateAllUpcomingAddedPrimitivesWithItem(size_t indexOfCurrentlyEvaluatedItem)
		{
			m_indexOfCurrentlyEvaluatedItem = indexOfCurrentlyEvaluatedItem;
		}

		void CDebugRenderWorld::DrawAllAddedPrimitives(size_t indexOfItemCurrentlyBeingFocusedForInspection) const
		{
			for (const SDebugRenderPrimitiveWithItemAssociation& p : m_primitives)
			{
				const bool highlight = (indexOfItemCurrentlyBeingFocusedForInspection != kItemIndexWithoutAssociation && p.associatedItemIndex == indexOfItemCurrentlyBeingFocusedForInspection);
				p.pPrimitive->Draw(highlight);
			}
		}

		size_t CDebugRenderWorld::GetRoughMemoryUsage() const
		{
			size_t memoryUsed = 0;

			for (const SDebugRenderPrimitiveWithItemAssociation& p : m_primitives)
			{
				memoryUsed += p.pPrimitive->GetRoughMemoryUsage();
			}

			return memoryUsed;
		}

		void CDebugRenderWorld::Serialize(Serialization::IArchive& ar)
		{
			ar(m_primitives, "m_primitives");
			ar(m_indexOfCurrentlyEvaluatedItem, "m_indexOfCurrentlyEvaluatedItem");
		}

	}
}
