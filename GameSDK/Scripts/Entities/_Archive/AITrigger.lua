-- 10/12/04 by Kirill
-- CXP specific entity, to notify AI system of mission events, later this functionality will be 
-- implemented with mission flow system.

AITrigger = {
  type = "AITrigger",
	Properties = {
			modifyerName = "bridge",
			fAlarmRadius = 50,
	},
	
	Editor={
		--Model="Editor/Objects/T.cgf",
		Model="Editor/Objects/box.cgf",
	},
		
}

-------------------------------------------------------
function AITrigger:OnInit()
	self:OnReset();
	
end

-------------------------------------------------------
function AITrigger:OnReset()
	
--	AI:RegisterWithAI(self.id, self.Properties.Action);
--	self:RegisterWithAI(self.Properties.Action);
end

-------------------------------------------------------
-------------------------------------------------------
-- Input events
-------------------------------------------------------

function AITrigger:Event_Event( sender )
	AI.Event(1, self.Properties.modifyerName); 
	BroadcastEvent( self,"Event" );
	AI.Commander:OnBridgeDestroyed();
end

-------------------------------------------------------

function AITrigger:Event_Alarm( sender )
	System.Log( "Alarm!" );
	AI.SoundEvent(self:GetWorldPos(), self.Properties.fAlarmRadius, AISOUND_GENERIC, g_localActor.id);
	BroadcastEvent( self,"Alarm" );
end


-------------------------------------------------------

-------------------------------------------------------
-- Output events
-------------------------------------------------------
--function AIObject:Event_OnTouch(sender)
--	BroadcastEvent( self,"OnTouch" );
--end
-------------------------------------------------------


AITrigger.Events =
{
	Inputs =
	{
		Alarm = { AITrigger.Event_Alarm, "bool" },
		Event = { AITrigger.Event_Event, "bool" },
	},
	Outputs =
	{
		Alarm = "bool",
		Event = "bool",
	},
}