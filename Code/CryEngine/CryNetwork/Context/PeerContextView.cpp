// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------
   $Id$
   $DateTime$
   Description:  Peer implementation of a context view
   -------------------------------------------------------------------------
   History:
   - 05/11/2009   : Created by Andrew Catlender
*************************************************************************/

#include "StdAfx.h"
#include "PeerContextView.h"
#include "NetContext.h"

CPeerContextView::CPeerContextView(CNetChannel* pNetChannel, CNetContext* pNetContext)
{
	SetMMM(pNetChannel->GetChannelMMM());
	SContextViewConfiguration config = {
		NULL, // FlushMsgs
		NULL, // ChangeState
		NULL, // ForceNextState
		NULL, // FinishState
		NULL, // BeginUpdateObject
		NULL, // EndUpdateObject
		NULL, // ReconfigureObject
		NULL, // SetAuthority
		NULL, // VoiceData
		NULL, // RemoveStaticEntity
		NULL, // UpdatePhysicsTime
		NULL, // BeginSyncFiles;
		NULL, // BeginSyncFile;
		NULL, // AddFileData;
		NULL, // EndSyncFile;
		NULL, // AllFilesSynced;
		// partial update notification messages
		{
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
#if NUM_ASPECTS > 8
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
#endif//NUM_ASPECTS > 8
#if NUM_ASPECTS > 16
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
#endif//NUM_ASPECTS > 16
		},
		// set aspect profile messages
		{
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
#if NUM_ASPECTS > 8
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
#endif//NUM_ASPECTS > 8
#if NUM_ASPECTS > 16
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
#endif//NUM_ASPECTS > 16
		},
		// update aspect messages
		{
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
#if NUM_ASPECTS > 8
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
#endif//NUM_ASPECTS > 8
#if NUM_ASPECTS > 16
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
#endif//NUM_ASPECTS > 16
		},
#if ENABLE_ASPECT_HASHING
		// hash aspect messages
		{
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
	#if NUM_ASPECTS > 8
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
	#endif//NUM_ASPECTS > 8
	#if NUM_ASPECTS > 16
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
	#endif//NUM_ASPECTS > 16
		},
#endif
		// rmi messages
		{
			NULL, // RMI_ReliableOrdered
			NULL, // RMI_ReliableUnordered
			NULL, // RMI_UnreliableOrdered
			NULL,
			// must be last
			NULL, // RMI_Attachment
		},
	};

	Init(pNetChannel, pNetContext, &config);
}
