// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{

		class CRenderPrimitiveCollection final : public IRenderPrimitiveCollection
		{
		public:

			// IRenderPrimitiveCollection
			virtual void                                          AddSphere(const Vec3& pos, float radius, const ColorF& color) override;
			virtual void                                          AddLine(const Vec3& pos1, const Vec3& pos2, const ColorF& color) override;
			virtual void                                          AddTriangle(const Vec3& vtx1, const Vec3& vtx2, const Vec3& vtx3, const ColorF& color) override;
			virtual void                                          AddText(const Vec3& pos, float size, const ColorF& color, const char* szFormat, ...) override PRINTF_PARAMS(5, 6);
			virtual void                                          AddArrow(const Vec3& from, const Vec3& to, float coneRadius, float coneHeight, const ColorF& color) override;
			virtual void                                          AddAxes(const Vec3& pos, const Matrix33& axes) override;
			virtual void                                          AddAABB(const AABB& aabb, const ColorF& color) override;
			virtual void                                          AddOBB(const OBB& obb, const Vec3& pos, const ColorF& color) override;
			virtual void                                          AddCone(const Vec3& pos, const Vec3& dir, const float radius, const float height, const ColorF& color) override;
			virtual void                                          AddCylinder(const Vec3& pos, const Vec3& dir, const float radius, const float height, const ColorF& color) override;
			// ~IRenderPrimitiveCollection

			void                                                  Draw() const;
			void                                                  GatherStatistics(size_t& outNumRenderPrimitivesInTotalSoFar, size_t& outRoughMemoryUsageOfRenderPrimitivesInTotalSoFar) const;
			void                                                  Serialize(Serialization::IArchive& ar);

		private:

			std::vector<std::unique_ptr<CRenderPrimitiveBase>>    m_prims;
		};

	}
}