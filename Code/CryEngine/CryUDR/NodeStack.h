// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{

		class CNodeStack final : public INodeStack
		{
		public:

			explicit              CNodeStack(CNode& root);

			// INodeStack         
			virtual void          PushNode(INode& nodeToPush) override;
			virtual INode&        GetTopNode() override;
			virtual void          PopNode() override;
			// ~INodeStack

		private:

			std::stack<INode*>    m_stack;
		};

	}
}