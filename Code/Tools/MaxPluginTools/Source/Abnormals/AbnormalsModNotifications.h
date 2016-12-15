#pragma once

class AbnormalsModNotifications
{
public:
	static void RegisterNotifications();
	static void UnregisterNotifications();
private:
	static bool notificationsRegistered;
	
	explicit AbnormalsModNotifications() {}

	static void PostNodesCloned(void* param, NotifyInfo* info);
};