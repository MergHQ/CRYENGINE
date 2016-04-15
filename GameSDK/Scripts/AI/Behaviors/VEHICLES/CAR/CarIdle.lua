local Behavior = CreateAIBehavior("CarIdle", "VehicleIdle",		
{
	Constructor = function(self , entity )
		self.parent:Constructor( entity );
		AI.SetAdjustPath(entity.id,1);
	end,

	TO_CAR_SKID = function( self, entity, sender, data )
		AI.LogEvent(entity:GetName().." CAR_SKID"..data.point.x..","..data.point.y..","..data.point.z);
		entity.AI.vSkidDestination = {};
		CopyVector( entity.AI.vSkidDestination, data.point );
	end,
})
