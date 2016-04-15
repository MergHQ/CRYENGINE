local Behavior = CreateAIBehavior("BoatIdle", "VehicleIdle",
{
	Constructor = function( self , entity )
		AI.ChangeParameter(entity.id,AIPARAM_COMBATCLASS,AICombatClasses.BOAT);		
		AI.SetAdjustPath(entity.id,1);
		
		self.parent:Constructor( entity );
	end,
})