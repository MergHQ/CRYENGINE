Script.ReloadScript( "Scripts/Entities/Others/RigidBody.lua" );

---------------------------------------------------------

DestroyedVehicle = 
{
	type = "DestroyedVehicle",

	Properties = 
	{
		bInteractLargeObject = 1, 
		bUsable = 1,
		Physics = 
		{
			Mass = 2000,
		},
	},

	PropertiesInstance = 
	{
		bProvideAICover = 1,  
	},

	Editor = 
	{
		IsScalable = false;
	},
}

MakeDerivedEntity(DestroyedVehicle,RigidBody);
AddHeavyObjectProperty(DestroyedVehicle);
MakeAICoverEntity(DestroyedVehicle);
SetupCollisionFiltering(DestroyedVehicle);

function DestroyedVehicle:IsUsable()
	if(self.Properties.bUsable ~= 0) then
		return 1;
	end

	return 0;
end

function DestroyedVehicle:OnReset()
	RigidBody.OnReset(self);
	ApplyCollisionFiltering(self, GetCollisionFiltering(self));
end