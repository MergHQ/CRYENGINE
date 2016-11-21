// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// CDebugRenderWorld
		//
		//===================================================================================

		class CDebugRenderWorld : public IDebugRenderWorld
		{
		public:
			static const size_t                                     kItemIndexWithoutAssociation = (size_t)-1;

		private:
			struct SDebugRenderPrimitiveWithItemAssociation
			{
				explicit                                            SDebugRenderPrimitiveWithItemAssociation();  // default ctor required for yasli serialization
				                                                    SDebugRenderPrimitiveWithItemAssociation(const SDebugRenderPrimitiveWithItemAssociation& other);  // copy-ctor (that is not explicit) required for yasli serialization of STL containers
				explicit                                            SDebugRenderPrimitiveWithItemAssociation(std::unique_ptr<CDebugRenderPrimitiveBase>& _primitive, size_t _associatedItemIndex);
				explicit                                            SDebugRenderPrimitiveWithItemAssociation(SDebugRenderPrimitiveWithItemAssociation&& other);
				SDebugRenderPrimitiveWithItemAssociation&           operator=(const SDebugRenderPrimitiveWithItemAssociation& other);   // required for yasli serialization

				void                                                Serialize(Serialization::IArchive& ar);

				std::unique_ptr<CDebugRenderPrimitiveBase>          pPrimitive;
				size_t                                              associatedItemIndex;  // is kItemIndexWithoutAssociation if no item is associated with the render primitive (which is the case when adding primitives outside the evaluation phases)
			};

		public:
			explicit                                                CDebugRenderWorld();

			virtual void                                            DrawSphere(const Vec3& pos, float radius, const ColorF& color) const override;
			virtual void                                            DrawDirection(const Vec3& pos, float radius, const Vec3& dir, const ColorF& color) const override;
			virtual void                                            DrawLine(const Vec3& pos1, const Vec3& pos2, const ColorF& color) const override;
			virtual void                                            DrawCone(const Vec3& pos, const Vec3& dir, float baseRadius, float height, const ColorF& color) const override;
			virtual void                                            DrawCylinder(const Vec3& pos, const Vec3& dir, float radius, float height, const ColorF& color) const override;
			virtual void                                            DrawText(const Vec3& pos, float size, const ColorF& color, const char* fmt, ...) const override PRINTF_PARAMS(5, 6);
			virtual void                                            DrawQuat(const Vec3& pos, const Quat& q, float r, const ColorF& color) const override;
			virtual void                                            DrawAABB(const AABB& aabb, const ColorF& color) const override;
			virtual void                                            DrawOBB(const OBB& obb, const ColorF& color) const override;

			virtual void                                            AddSphere(const Vec3& pos, float radius, const ColorF& color) override;
			virtual void                                            AddDirection(const Vec3& pos, float radius, const Vec3& dir, const ColorF& color) override;
			virtual void                                            AddLine(const Vec3& pos1, const Vec3& pos2, const ColorF& color) override;
			virtual void                                            AddCone(const Vec3& pos, const Vec3& dir, float baseRadius, float height, const ColorF& color) override;
			virtual void                                            AddCylinder(const Vec3& pos, const Vec3& dir, float radius, float height, const ColorF& color) override;
			virtual void                                            AddText(const Vec3& pos, float size, const ColorF& color, const char* fmt, ...) override;
			virtual void                                            AddQuat(const Vec3& pos, const Quat& q, float r, const ColorF& color) override;
			virtual void                                            AddAABB(const AABB& aabb, const ColorF& color) override;
			virtual void                                            AddOBB(const OBB& obb, const ColorF& color) override;

			void                                                    AssociateAllUpcomingAddedPrimitivesWithItem(size_t indexOfCurrentlyEvaluatedItem);
			void                                                    DrawAllAddedPrimitives(size_t indexOfItemCurrentlyBeingFocusedForInspection) const;
			size_t                                                  GetRoughMemoryUsage() const;

			void                                                    Serialize(Serialization::IArchive& ar);

		private:
			                                                        UQS_NON_COPYABLE(CDebugRenderWorld);

		private:
			std::vector<SDebugRenderPrimitiveWithItemAssociation>   m_primitives;
			size_t                                                  m_indexOfCurrentlyEvaluatedItem;    // used for associating render-primitives when being added via the Add*() methods with the current item that is being evaluated by the ongoing query
		};

	}
}
