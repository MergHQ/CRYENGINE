// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{

		CNodeStack::CNodeStack(CNode& root)
		{
			m_stack.push(&root);
		}

		void CNodeStack::PushNode(INode& nodeToPush)
		{
			m_stack.push(&nodeToPush);
		}

		INode& CNodeStack::GetTopNode()
		{
			CRY_ASSERT(!m_stack.empty());	// the root node of the live tree (we pushed in our ctor) is always on the stack (unless there's a bug involving more Pops than Pushes)
			return *m_stack.top();
		}

		void CNodeStack::PopNode()
		{
			CRY_ASSERT(m_stack.size() > 1);	// careful: the root node of the live tree (we pushed in our ctor) also counts and must not get popped (otherwise there's a bug involving more Pops than Pushes)
			m_stack.pop();
		}

	}
}