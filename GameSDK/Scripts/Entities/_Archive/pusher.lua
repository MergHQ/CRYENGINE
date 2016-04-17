--created by Kirill	
--
--	this entity has to be attached to a trigger
--	it will add an impulse to a vehicle the player is in when triggering the trigger
--	if player is not in the vehicle - does nothing
--	Impulse is added in negative Y axis direction

Pusher = {
	type = "Pusher",
	Properties = {	
		fImpulse = 10,
		bEnabled = 1,
		bOnce = 0,
		},
	Editor={
		Model="Objects/Editor/T.cgf",
	},
}


function Pusher:OnInit()

	self:OnReset();
	
end

-----------------------------------------------------------------------------
function Pusher:OnReset( )

	self.isEnabled = self.Properties.bEnabled;
	
end



-----------------------------------------------------------------------------
function Pusher:Event_Push( trigger,areaId )

	if( self.isEnabled == 0 ) then return end
local player = trigger.Who;
	if( not player ) then return end
local theVehicle = player.theVehicle;	
	if( not theVehicle ) then return end
	if( not theVehicle.driverT ) then return end	
	if( theVehicle.driverT.entity ~= player ) then return end

	theVehicle:AddImpulseObj( self:GetDirectionVector(), self.Properties.fImpulse);
	
	if(self.Properties.bOnce == 1) then
		self.isEnabled = 0;
	end	
--System.Log("\001  pushing!!! ");
end


-----------------------------------------------------------------------------
function Pusher:OnShutDown()

end



----------------------------------------------------------------------------------------------------------------------------
--
function Pusher:Event_Enable( params )

	self.isEnabled = 1;

end

----------------------------------------------------------------------------------------------------------------------------
--
function Pusher:Event_Disable( params )

	self.isEnabled = 0;

end

Pusher.FlowEvents =
{
	Inputs =
	{
		Disable = { Pusher.Event_Disable, "bool" },
		Enable = { Pusher.Event_Enable, "bool" },
		Push = { Pusher.Event_Push, "bool" },
	},
	Outputs =
	{
		Disable = "bool",
		Enable = "bool",
		Push = "bool",
	},
}
