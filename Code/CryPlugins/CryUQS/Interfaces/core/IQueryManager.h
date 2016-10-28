// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// IQueryManager
		//
		// - starts queries, updates them over time and allows to prematurely cancel a running query
		// - once a query finishes, it will notify the caller through the callback that was provided by the initial client::SQueryRequest
		// - for each query, several item-monitors can be installed, and they all will run at least until the end of that query (but may get carried over to the next one in case of hierarchical queries)
		//
		//===================================================================================

		struct IQueryManager
		{
			virtual                       ~IQueryManager() {}
			virtual CQueryID              StartQuery(const client::SQueryRequest& request, shared::IUqsString& errorMessage) = 0;
			virtual void                  CancelQuery(const CQueryID& idOfQueryToCancel) = 0;
			virtual void                  AddItemMonitorToQuery(const CQueryID& queryID, client::ItemMonitorUniquePtr&& pItemMonitorToInstall) = 0;
		};

	}
}
