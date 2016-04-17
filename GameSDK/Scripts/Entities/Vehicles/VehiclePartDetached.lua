VehiclePartDetached =
{
	Client = {},
	Server = {},

	Properties =
	{
		bPickable = 1,
	},
	
	Editor =
	{
		Icon		= "Item.bmp",
		IconOnTop	= 1,
	},
}

function VehiclePartDetached:IsUsable(user)
	return 1;
end;

function VehiclePartDetached:GetUsableMessage(idx)
	return "@grab_object";
end;

function VehiclePartDetached:OnInit()
	self:OnReset();
end

function VehiclePartDetached:OnPropertyChange()
	self:OnReset();
end

function VehiclePartDetached:OnReset()
end

function VehiclePartDetached:SetObjectModel(model)
end

function VehiclePartDetached:OnShutDown()
end

function VehiclePartDetached:Event_TargetReached(sender)
end

function VehiclePartDetached:Event_Enable(sender)
end

function VehiclePartDetached:Event_Disable(sender)
end

function VehiclePartDetached:GetReturnToPoolWeight(isUrgent)
	local	maxDistance = 100;
	
	local	weight = self:GetTimeSinceLastSeen() * maxDistance;
	
	if (g_localActor) then
		local	distance = self:GetDistance(g_localActor.id);
		
		if (distance > maxDistance) then
			distance = maxDistance;
		end
		
		weight = weight + distance;
	end

	return weight;
end

VehiclePartDetached.FlowEvents =
{
	Inputs =
	{
	},
	
	Outputs =
	{
	},
}
