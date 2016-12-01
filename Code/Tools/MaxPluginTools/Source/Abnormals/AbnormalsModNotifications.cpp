#include "stdafx.h"
#include "AbnormalsModNotifications.h"
#include "AbnormalsMod.h"

#include "notify.h"
#include "AppDataChunk.h"

bool AbnormalsModNotifications::notificationsRegistered = false;

void AbnormalsModNotifications::RegisterNotifications()
{
	if (!notificationsRegistered)
	{
		RegisterNotification(PostNodesCloned, NULL, NOTIFY_POST_NODES_CLONED);
		notificationsRegistered = true;
	}
}

void AbnormalsModNotifications::UnregisterNotifications()
{
	if (notificationsRegistered)
	{
		UnRegisterNotification(PostNodesCloned, NULL, NOTIFY_POST_NODES_CLONED);
		notificationsRegistered = false;
	}
}

void AbnormalsModNotifications::PostNodesCloned(void* param, NotifyInfo* info)
{
	struct CloneInfo { INodeTab* origNodes; INodeTab* clonedNodes; CloneType cloneType; };
	
	CloneInfo* cloneInfo = (CloneInfo*)info->callParam;

	for (int n = 0; n < cloneInfo->origNodes->Count(); n++)
	{
		INode* orig = (*cloneInfo->origNodes)[n];
		INode* clone = (*cloneInfo->clonedNodes)[n];

		// Copy all relevant appdata to the new nodes.
		AppDataChunk* chunk = orig->GetAppDataChunk(orig->ClassID(), orig->SuperClassID(), ID_ABNORMALS_MOD_CHANNEL_DATA);

		if (chunk != nullptr)
		{
			clone->AddAppDataChunk(clone->ClassID(), clone->SuperClassID(), ID_ABNORMALS_MOD_CHANNEL_DATA, sizeof(int),chunk->data);
		}
	}
}