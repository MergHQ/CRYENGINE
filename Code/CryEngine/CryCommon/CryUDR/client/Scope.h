// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{

		//===================================================================================
		//
		// CScope* classes:
		//
		// These are the workhorses of the whole debug recording system from the game code's point of view.
		// The game code can create instances of such a class on the program stack for adding render-primitives in the according scope.
		// Scopes can get nested by creating more instances on the stack. This will create a whole tree of named nodes in the end.
		// Each node will then eventually have several render-primitives that will be drawn from inside the "UDR Recordings" editor-plugin.
		// Scopes can be re-entered on subsequent frames by simply building a stack with names used prior - this is typically done via
		// the CScope_FixedString class.
		//
		//===================================================================================

		//! Base class for all CScope_* classes
		//! Can only be instantiated via derived classes
		//! Gives access to the underlying IRenderPrimitiveCollection in the tree for adding all kinds of render-primitives to it via its Add*() methods by the game code
		//! These render-primitives will get persisted when saving the live tree do disk
		class CScopeBase
		{
		public:

			const char*                     GetName() const;
			IRenderPrimitiveCollection*     operator->() const;

		protected:

			explicit                        CScopeBase();
			~CScopeBase();

			void                            PushNode(INode& node);

		private:

			CScopeBase(const CScopeBase&) = delete;
			CScopeBase(CScopeBase&&) = delete;
			CScopeBase&                     operator=(const CScopeBase&) = delete;
			CScopeBase&                     operator=(CScopeBase&&) = delete;

			static void*                    operator new(size_t) = delete;
			static void*                    operator new[](size_t) = delete;

			static void*                    operator new(size_t, void*) = delete;
			static void*                    operator new[](size_t, void*) = delete;

			static void                     operator delete(void*) = delete;
			static void                     operator delete[](void*) = delete;

			static void                     operator delete(void*, void*) = delete;
			static void                     operator delete[](void*, void*) = delete;

		private:

			INode*                          m_pNode;
			CRecursiveSyncObjectAutoLock    m_lock;
		};

		inline CScopeBase::CScopeBase()
			: m_pNode(nullptr)
		{}

		inline CScopeBase::~CScopeBase()
		{
			CRY_ASSERT(m_pNode);
			gEnv->pUDR->GetHub().GetNodeStack().PopNode();
		}

		inline void CScopeBase::PushNode(INode& node)
		{
			CRY_ASSERT(!m_pNode);
			m_pNode = &node;
			gEnv->pUDR->GetHub().GetNodeStack().PushNode(node);
		}

		inline const char* CScopeBase::GetName() const
		{
			CRY_ASSERT(m_pNode);
			return m_pNode->GetName();
		}

		inline IRenderPrimitiveCollection* CScopeBase::operator->() const
		{
			CRY_ASSERT(m_pNode);
			return &m_pNode->GetRenderPrimitiveCollection();
		}

		//===================================================================================
		//
		// CScope_FixedString
		//
		// - uses a fixed string to re-enter the particular scope with this name on subsequent calls
		// - creates a scope with this name on first call
		//
		//===================================================================================

		class CScope_FixedString : public CScopeBase
		{
		public:
			explicit CScope_FixedString(const char* szNodeName);
		};

		inline CScope_FixedString::CScope_FixedString(const char* szNodeName)
		{
			PushNode(gEnv->pUDR->GetHub().GetNodeStack().GetTopNode().AddChild_FixedStringIfNotYetExisting(szNodeName));
		}

		//===================================================================================
		//
		// CScope_UniqueStringAutoIncrement
		//
		// - creates a new unique scope upon subsequent calls
		// - the scopes will be named according to given prefix plus an internal number that increments with each call, thus
		//   producing a new scope every time
		//
		//===================================================================================

		class CScope_UniqueStringAutoIncrement : public CScopeBase
		{
		public:
			explicit CScope_UniqueStringAutoIncrement(const char* szNodeNamePrefix);
		};

		inline CScope_UniqueStringAutoIncrement::CScope_UniqueStringAutoIncrement(const char* szNodeNamePrefix)
		{
			PushNode(gEnv->pUDR->GetHub().GetNodeStack().GetTopNode().AddChild_UniqueStringAutoIncrement(szNodeNamePrefix));
		}

		//===================================================================================
		//
		// CScope_UseLastAutoIncrementedUniqueString
		//
		// - continues with a scope that was previously created via CScope_UniqueStringAutoIncrement
		// - the prefix to pass in should be the same for both classes
		//
		//===================================================================================

		class CScope_UseLastAutoIncrementedUniqueString : public CScopeBase
		{
		public:
			explicit CScope_UseLastAutoIncrementedUniqueString(const char* szNodeNamePrefix);
		};

		inline CScope_UseLastAutoIncrementedUniqueString::CScope_UseLastAutoIncrementedUniqueString(const char* szNodeNamePrefix)
		{
			PushNode(gEnv->pUDR->GetHub().GetNodeStack().GetTopNode().AddChild_FindLastAutoIncrementedUniqueNameOrCreateIfNotYetExisting(szNodeNamePrefix));
		}

		//===================================================================================
		//
		// CScope_CurrentSystemFrame
		//
		// - creates a new scope with a name based on the current system frame
		// - if such a scope already exists, it will use that
		//
		//===================================================================================

		class CScope_CurrentSystemFrame : public CScopeBase
		{
		public:
			explicit CScope_CurrentSystemFrame();
		};

		inline CScope_CurrentSystemFrame::CScope_CurrentSystemFrame()
		{
			PushNode(gEnv->pUDR->GetHub().GetNodeStack().GetTopNode().AddChild_CurrentSystemFrame());
		}

	}
}