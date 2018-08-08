// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{

		class CTree;

		class CNode final : public INode
		{
		public:

			struct SStatistics
			{
				size_t                               numChildren = 0;
				size_t                               numRenderPrimitives = 0;
				size_t                               roughMemoryUsageOfRenderPrimitives = 0;
			};

		public:

			explicit                                 CNode();               // default ctor required for yasli serialization
			explicit                                 CNode(const char* szNodeName, const CTree& owningTree);
			CNode&                                   operator = (CNode && other);

			// INode
			virtual INode&                           AddChild_FixedStringIfNotYetExisting(const char* szChildNodeName) override;
			virtual INode&                           AddChild_UniqueStringAutoIncrement(const char* szChildNodeNamePrefix) override;
			virtual INode&                           AddChild_FindLastAutoIncrementedUniqueNameOrCreateIfNotYetExisting(const char* szChildNodeNamePrefix) override;
			virtual INode&                           AddChild_CurrentSystemFrame() override;
			virtual IRenderPrimitiveCollection&      GetRenderPrimitiveCollection() override;
			virtual const char*                      GetName() const override;
			virtual const INode*                     GetParent() const override;
			virtual size_t                           GetChildCount() const override;
			virtual const INode&                     GetChild(size_t index) const override;
			virtual void                             DrawRenderPrimitives(bool recursivelyDrawChildrenAsWell) const override;
			// ~INode

			void                                     Serialize(Serialization::IArchive& ar);
			void                                     RemoveChildren();
			void                                     GatherStatisticsRecursively(SStatistics& outStats) const;
			void                                     PrintHierarchyToConsoleRecursively(int indentLevel) const;
			void                                     PrintStatisticsToConsole(const char* szMessagePrefix) const;

		private:

			CNode(const CNode &) = delete;
			CNode(CNode &&) = delete;
			CNode& operator = (const CNode&) = delete;

			CNode*                                   FindChildNode(const char* szChildNodeName) const;
			CNode&                                   AddChildNode(const char* szChildNodeName);
			void                                     RemoveChild(size_t index);
			void                                     SetOwningTreeInChildrenRecursively();
			void                                     SetParentInChildrenRecursively();
			void                                     BuildFastLookupTable();

		private:

			const CTree*                             m_pOwningTree;
			const CNode*                             m_pParent;
			string                                   m_name;
			CRenderPrimitiveCollection               m_renderPrimitives;
			std::vector <std::unique_ptr<CNode>>     m_children;
			std::map <string, size_t>                m_childrenByName;   // for fast lookup in FindChildNode(); key = child name, value = index into m_children[]
			std::map <string, int>                   m_uniqueNamesCache;    // for keeping track of when a unique name is to be generated using an integer-suffix that will auto-increment; key = name prefix, value = last auto-incremented integer value
		};

	}
}
