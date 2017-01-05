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

			//===================================================================================
			//
			// SDebugRenderPrimitiveWithItemAssociation
			//
			// - holds the render-primitive plus some extra information related to a potential item it may be associated with:
			//   (1) it's not associated with an item at all (.associatedItemIndex == kItemIndexWithoutAssociation, .bIsPartOfTheItem == false); this would be the case for when a generator adds some primitves
			//   (2) it's associated with an item, but not part of the item itself (.associatedItemIndex != kItemIndexWithoutAssociation, .bIsPartOfTheItem == false); this would be the case for when an evaluator adds its custom primitives while evaluating the item
			//   (3) it's associated with an item, and it's part of the item itself (.associatedItemIndex != kItemIndexWithoutAssociation, .bIsPartOfTheItem == true); this would be the case for when all items have just been generated and being exposed to the render-world all at once
			//
			//===================================================================================

			struct SDebugRenderPrimitiveWithItemAssociation
			{
				explicit                                            SDebugRenderPrimitiveWithItemAssociation();  // default ctor required for yasli serialization
				                                                    SDebugRenderPrimitiveWithItemAssociation(const SDebugRenderPrimitiveWithItemAssociation& other);  // copy-ctor (that is not explicit) required for yasli serialization of STL containers
				explicit                                            SDebugRenderPrimitiveWithItemAssociation(std::unique_ptr<CDebugRenderPrimitiveBase>& _primitive, size_t _associatedItemIndex, bool _bIsPartOfTheItem);
				explicit                                            SDebugRenderPrimitiveWithItemAssociation(SDebugRenderPrimitiveWithItemAssociation&& other);
				SDebugRenderPrimitiveWithItemAssociation&           operator=(const SDebugRenderPrimitiveWithItemAssociation& other);   // required for yasli serialization

				void                                                Serialize(Serialization::IArchive& ar);

				std::unique_ptr<CDebugRenderPrimitiveBase>          pPrimitive;
				size_t                                              associatedItemIndex;  // is kItemIndexWithoutAssociation if no item is associated with the render primitive (which is the case when adding primitives outside the evaluation phases)
				bool                                                bIsPartOfTheItem;     // true if the render primitive is one of many primitives to make the item look as it is; false if the primitive is just additional information that was added by e. g. an evaluator
			};

		public:
			explicit                                                CDebugRenderWorld();

			virtual void                                            DrawSphere(const Vec3& pos, float radius, const ColorF& color) const override;
			virtual void                                            DrawDirection(const Vec3& from, const Vec3& to, float coneRadius, float coneHeight, const ColorF& color) const override;
			virtual void                                            DrawLine(const Vec3& pos1, const Vec3& pos2, const ColorF& color) const override;
			virtual void                                            DrawCone(const Vec3& pos, const Vec3& dir, float baseRadius, float height, const ColorF& color) const override;
			virtual void                                            DrawCylinder(const Vec3& pos, const Vec3& dir, float radius, float height, const ColorF& color) const override;
			virtual void                                            DrawText(const Vec3& pos, float size, const ColorF& color, const char* fmt, ...) const override PRINTF_PARAMS(5, 6);
			virtual void                                            DrawQuat(const Vec3& pos, const Quat& q, float r, const ColorF& color) const override;
			virtual void                                            DrawAABB(const AABB& aabb, const ColorF& color) const override;
			virtual void                                            DrawOBB(const OBB& obb, const ColorF& color) const override;

			virtual void                                            AddSphere(const Vec3& pos, float radius, const ColorF& color) override;
			virtual void                                            AddDirection(const Vec3& from, const Vec3& to, float coneRadius, float coneHeight, const ColorF& color) override;
			virtual void                                            AddLine(const Vec3& pos1, const Vec3& pos2, const ColorF& color) override;
			virtual void                                            AddCone(const Vec3& pos, const Vec3& dir, float baseRadius, float height, const ColorF& color) override;
			virtual void                                            AddCylinder(const Vec3& pos, const Vec3& dir, float radius, float height, const ColorF& color) override;
			virtual void                                            AddText(const Vec3& pos, float size, const ColorF& color, const char* fmt, ...) override;
			virtual void                                            AddQuat(const Vec3& pos, const Quat& q, float r, const ColorF& color) override;
			virtual void                                            AddAABB(const AABB& aabb, const ColorF& color) override;
			virtual void                                            AddOBB(const OBB& obb, const ColorF& color) override;

			// These 3 methods deal with whether primitives will be associated with an item and whether they will be even part of what an item will look like.
			// They basically change the inner "state" of the CDebugRenderWorld in terms of how newly added primitives will be treated.
			void                                                    ItemConstructionBegin();
			void                                                    AssociateAllUpcomingAddedPrimitivesWithItem(size_t indexOfCurrentlyEvaluatedItem);
			void                                                    ItemConstructionEnd();

			void                                                    DrawAllAddedPrimitivesAssociatedWithItem(size_t indexOfItemToDrawPrimitivesOf, const ColorF& colorToOverridePrimitivesWithIfPartOfTheItem, bool bHighlight) const;
			void                                                    DrawAllAddedPrimitivesWithNoItemAssociation() const;

			size_t                                                  GetRoughMemoryUsage() const;

			void                                                    Serialize(Serialization::IArchive& ar);

		private:
			                                                        UQS_NON_COPYABLE(CDebugRenderWorld);

		private:
			std::vector<SDebugRenderPrimitiveWithItemAssociation>   m_primitives;
			size_t                                                  m_indexOfCurrentlyEvaluatedItem;    // used for associating render-primitives when being added via the Add*() methods with the current item that is being evaluated by the ongoing query
			bool                                                    m_bIsItemConstructionInProgress;    // while true, it means that all render-primitives that get added are part of what the item makes it look as it is
		};

	}
}
