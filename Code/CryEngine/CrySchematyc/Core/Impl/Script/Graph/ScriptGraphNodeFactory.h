// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Script/IScriptGraph.h>

namespace Schematyc
{
	struct IScriptView;
	struct IScriptGraphNodeCreationMenu;

	struct IScriptGraphNodeCreator
	{
		virtual ~IScriptGraphNodeCreator() {}

		virtual SGUID GetTypeGUID() const = 0;
		virtual IScriptGraphNodePtr CreateNode(const SGUID& guid) = 0;
		virtual void PopulateNodeCreationMenu(IScriptGraphNodeCreationMenu& nodeCreationMenu, const IScriptView& scriptView, const IScriptGraph& graph) = 0;
	};

	DECLARE_SHARED_POINTERS(IScriptGraphNodeCreator)

	class CScriptGraphNodeFactory
	{
	public:

		bool RegisterCreator(const IScriptGraphNodeCreatorPtr& pCreator);
		IScriptGraphNodeCreator* GetCreator(const SGUID& typeGUID);
		IScriptGraphNodePtr CreateNode(const SGUID& typeGUID, const SGUID& guid = SGUID());
		void PopulateNodeCreationMenu(IScriptGraphNodeCreationMenu& nodeCreationMenu, const IScriptView& scriptView, const IScriptGraph& graph);

	private:

		typedef std::unordered_map<SGUID, IScriptGraphNodeCreatorPtr> Creators;

		Creators m_creators;
	};
}