// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{

		void CRenderPrimitiveCollection::AddSphere(const Vec3& pos, float radius, const ColorF& color)
		{
			m_prims.emplace_back(new CRenderPrimitive_Sphere(pos, radius, color));
		}

		void CRenderPrimitiveCollection::AddLine(const Vec3& pos1, const Vec3& pos2, const ColorF& color)
		{
			m_prims.emplace_back(new CRenderPrimitive_Line(pos1, pos2, color));
		}

		void CRenderPrimitiveCollection::AddTriangle(const Vec3& vtx1, const Vec3& vtx2, const Vec3& vtx3, const ColorF& color)
		{
			m_prims.emplace_back(new CRenderPrimitive_Triangle(vtx1, vtx2, vtx3, color));
		}

		void CRenderPrimitiveCollection::AddText(const Vec3& pos, float size, const ColorF& color, const char* szFormat, ...)
		{
			stack_string text;
			va_list ap;

			va_start(ap, szFormat);
			text.FormatV(szFormat, ap);
			va_end(ap);

			m_prims.emplace_back(new CRenderPrimitive_Text(pos, size, text.c_str(), color));
		}

		void CRenderPrimitiveCollection::AddArrow(const Vec3& from, const Vec3& to, float coneRadius, float coneHeight, const ColorF& color)
		{
			m_prims.emplace_back(new CRenderPrimitive_Arrow(from, to, coneRadius, coneHeight, color));
		}

		void CRenderPrimitiveCollection::AddAxes(const Vec3& pos, const Matrix33& axes)
		{
			m_prims.emplace_back(new CRenderPrimitive_Line(pos, pos + axes.GetColumn0(), Col_Red));
			m_prims.emplace_back(new CRenderPrimitive_Line(pos, pos + axes.GetColumn1(), Col_Green));
			m_prims.emplace_back(new CRenderPrimitive_Line(pos, pos + axes.GetColumn2(), Col_Blue));
		}

		void CRenderPrimitiveCollection::AddAABB(const AABB& aabb, const ColorF& color)
		{
			m_prims.emplace_back(new CRenderPrimitive_AABB(aabb, color));
		}

		void CRenderPrimitiveCollection::AddOBB(const OBB& obb, const Vec3& pos, const ColorF& color)
		{
			m_prims.emplace_back(new CRenderPrimitive_OBB(obb, pos, color));
		}

		void CRenderPrimitiveCollection::AddCone(const Vec3& pos, const Vec3& dir, const float radius, const float height, const ColorF& color)
		{
			const Vec3 coneEnd = pos + dir.GetNormalizedSafe() * height;
			m_prims.emplace_back(new CRenderPrimitive_Arrow(pos, coneEnd, radius, height, color));
		}

		void CRenderPrimitiveCollection::AddCylinder(const Vec3& pos, const Vec3& dir, const float radius, const float height, const ColorF& color)
		{
			m_prims.emplace_back(new CRenderPrimitive_Cylinder(pos, dir, radius, height, color));
		}

		void CRenderPrimitiveCollection::Draw() const
		{
			for (const std::unique_ptr<CRenderPrimitiveBase>& pPrim : m_prims)
			{
				pPrim->Draw();
			}
		}

		void CRenderPrimitiveCollection::GatherStatistics(size_t& outNumRenderPrimitivesInTotalSoFar, size_t& outRoughMemoryUsageOfRenderPrimitivesInTotalSoFar) const
		{
			outNumRenderPrimitivesInTotalSoFar += m_prims.size();

			for (const std::unique_ptr<CRenderPrimitiveBase>& pPrim : m_prims)
			{
				outRoughMemoryUsageOfRenderPrimitivesInTotalSoFar += pPrim->GetRoughMemoryUsage();
			}
		}

		template<typename T>
		struct SUniquePtrSerializer
		{
			std::unique_ptr<T>& ptr;

			SUniquePtrSerializer(std::unique_ptr<T>& ptr_) : ptr(ptr_) {}

			void Serialize(Serialization::IArchive& ar)
			{
				bool bPtrExists = (ptr != nullptr);
				if (ar.isInput())
				{
					if (ar(bPtrExists, "exists"))
					{
						if (bPtrExists)
						{
							ptr.reset(new T);
							ar(*ptr, "ptr");
						}
						else
						{
							ptr.reset();
						}
					}
				}
				else
				{
					ar(bPtrExists, "exists");
					if (bPtrExists)
					{
						ar(*ptr, "ptr");
					}
				}
			}
		};

		// TODO #1: investigate: why do we need a custom Serialize() function for std::unique_ptr<CNode>, but not for std::unique_ptr<CRenderPrimitiveBase> ???
		//          info: if we have no such Serialize() function, then only the root CNode in the tree will be seriazed (all its child nodes will appear as empty <Element> xml tags with no content)
		// TODO #2: is TODO #1 still valid after the refactorization of having moved the render-primitives from CNode into a custom class that CNode has an instance of now? (christianw, 2018-01-29)
		bool Serialize(Serialization::IArchive& ar, std::unique_ptr<CNode>& pNode, const char* szName, const char* szLabel)
		{
			SUniquePtrSerializer<CNode> ser(pNode);
			return ar(ser, szName, szLabel);
		}

		void CRenderPrimitiveCollection::Serialize(Serialization::IArchive& ar)
		{
			ar(m_prims, "m_renderPrimitives");	// the name "m_renderPrimitives" is for backwards compatibility (all prims were previously stored in the owning CNode class)
		}

	}
}